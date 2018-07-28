#include "stdafx.h"
#include "ValidateBinary.h"

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <fstream>

#pragma comment(lib, "wintrust")
#pragma comment(lib, "bcrypt.lib")

#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

bool ValidateBinary::ValidateCertificate(const std::wstring &app_path)
{
	WINTRUST_FILE_INFO FileData = { 0 };
	FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
	FileData.pcwszFilePath = app_path.c_str();
	FileData.hFile = nullptr;
	FileData.pgKnownSubject = nullptr;

	WINTRUST_DATA WinTrustData = { 0 };
	WinTrustData.cbStruct = sizeof(WinTrustData);
	WinTrustData.pPolicyCallbackData = nullptr;
	WinTrustData.pSIPClientData = nullptr;
	WinTrustData.dwUIChoice = WTD_UI_NONE;
	WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
	WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
	WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
	WinTrustData.hWVTStateData = nullptr;
	WinTrustData.pwszURLReference = nullptr;
	WinTrustData.dwUIContext = 0;
	WinTrustData.pFile = &FileData;

	GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	LONG status = ::WinVerifyTrust(
		nullptr,
		&WVTPolicyGUID,
		&WinTrustData);

	WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

	return status == ERROR_SUCCESS;
}

bool ValidateBinary::ValidateHash(const std::wstring &app_path, const std::wstring &app_hash)
{
	//open file
	std::ifstream infile(app_path, std::ios::binary);
	std::vector<char> buffer;

	//get length of file
	infile.seekg(0, infile.end);
	size_t length = static_cast<size_t>(infile.tellg());
	infile.seekg(0, infile.beg);

	//read file
	if (length < 1) {
		return false;
	}

	buffer.resize(length);
	infile.read(&buffer[0], length);

	BCRYPT_ALG_HANDLE algorithm = nullptr;
	BCRYPT_HASH_HANDLE hash_handle = nullptr;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DWORD data = 0, cb_hash = 0, cb_hash_object = 0;
	PBYTE hash_object = nullptr;
	PUCHAR hash = nullptr;

	//open an algorithm handle
	if (!NT_SUCCESS(status = ::BCryptOpenAlgorithmProvider(
		&algorithm,
		BCRYPT_SHA512_ALGORITHM,
		nullptr,
		0)))
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	//calculate the size of the buffer to hold the hash object
	if (!NT_SUCCESS(status = ::BCryptGetProperty(
		algorithm,
		BCRYPT_OBJECT_LENGTH,
		reinterpret_cast<PBYTE>(&cb_hash_object),
		sizeof(DWORD),
		&data,
		0)))
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	//allocate the hash object on the heap
	hash_object = reinterpret_cast<PBYTE>(::HeapAlloc(::GetProcessHeap(), 0, cb_hash_object));
	if (!hash_object)
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	//calculate the length of the hash
	if (!NT_SUCCESS(status = ::BCryptGetProperty(
		algorithm,
		BCRYPT_HASH_LENGTH,
		reinterpret_cast<PBYTE>(&cb_hash),
		sizeof(DWORD),
		&data,
		0)))
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	//allocate the hash buffer on the heap
	hash = reinterpret_cast<PBYTE>(::HeapAlloc(::GetProcessHeap(), 0, cb_hash));
	if (!hash)
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	//create a hash
	if (!NT_SUCCESS(status = ::BCryptCreateHash(
		algorithm,
		&hash_handle,
		hash_object,
		cb_hash_object,
		0,
		0,
		0)))
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	//hash data
	if (!NT_SUCCESS(status = ::BCryptHashData(
		hash_handle,
		reinterpret_cast<PUCHAR>(buffer.data()),
		buffer.size(),
		0)))
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	//close the hash
	if (!NT_SUCCESS(status = ::BCryptFinishHash(
		hash_handle,
		hash,
		cb_hash,
		0)))
	{
		CleanupHash(algorithm, hash_handle, hash_object, hash);
		return false;
	}

	std::vector<char> current_hash = BytesToHexString(hash, cb_hash);

	CleanupHash(algorithm, hash_handle, hash_object, hash);

	return CompareHashes(app_hash, current_hash);
}

std::vector<char> ValidateBinary::BytesToHexString(unsigned char const *bytes, std::size_t length)
{
	std::vector<char> file_hash;

	static char const lookup[] = "0123456789ABCDEF";

	for (std::size_t i = 0; i != length; ++i)
	{
		file_hash.push_back(lookup[bytes[i] >> 4]);
		file_hash.push_back(lookup[bytes[i] & 0xF]);
	}

	return file_hash;
}

bool ValidateBinary::CompareHashes(const std::wstring &hash_1, const std::vector<char> &hash_2)
{
	std::string str_hash_2(hash_2.begin(), hash_2.end());

	if (hash_1.size() < str_hash_2.size())
	{
		return false;
	}

	return std::equal(str_hash_2.begin(), str_hash_2.end(), hash_1.begin());
}

void ValidateBinary::CleanupHash(
	const BCRYPT_ALG_HANDLE &algorithm,
	const BCRYPT_HASH_HANDLE &hash_handle,
	const PBYTE &hash_object,
	const PUCHAR &hash)
{
	if (algorithm)
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
	}

	if (hash_handle)
	{
		::BCryptDestroyHash(hash_handle);
	}

	if (hash_object)
	{
		::HeapFree(GetProcessHeap(), 0, hash_object);
	}

	if (hash)
	{
		::HeapFree(GetProcessHeap(), 0, hash);
	}
}
