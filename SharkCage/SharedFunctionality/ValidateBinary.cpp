#include "stdafx.h"
#include "ValidateBinary.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <fstream>

#pragma comment(lib, "wintrust")
#pragma comment(lib, "bcrypt.lib")

bool ValidateBinary::ValidateCertificate(const std::wstring &app_path)
{
	WINTRUST_FILE_INFO file_data = { 0 };
	file_data.cbStruct = sizeof(WINTRUST_FILE_INFO);
	file_data.pcwszFilePath = app_path.c_str();
	file_data.hFile = nullptr;
	file_data.pgKnownSubject = nullptr;

	WINTRUST_DATA win_trust_data = { 0 };
	win_trust_data.cbStruct = sizeof(win_trust_data);
	win_trust_data.pPolicyCallbackData = nullptr;
	win_trust_data.pSIPClientData = nullptr;
	win_trust_data.dwUIChoice = WTD_UI_NONE;
	win_trust_data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
	win_trust_data.dwUnionChoice = WTD_CHOICE_FILE;
	win_trust_data.dwStateAction = WTD_STATEACTION_IGNORE;
	win_trust_data.hWVTStateData = nullptr;
	win_trust_data.pwszURLReference = nullptr;
	win_trust_data.dwUIContext = 0;
	win_trust_data.pFile = &file_data;

	GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	LONG status = ::WinVerifyTrust(
		nullptr,
		&WVTPolicyGUID,
		&win_trust_data);

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
	if (length < 1)
	{
		return false;
	}

	buffer.resize(length);
	infile.read(&buffer[0], length);

	BCRYPT_ALG_HANDLE algorithm = nullptr;
	//open an algorithm handle
	if (!BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(
		&algorithm,
		BCRYPT_SHA512_ALGORITHM,
		nullptr,
		0)))
	{
		return false;
	}

	DWORD cb_hash_object = 0, data = 0;
	//calculate the size of the buffer to hold the hash object
	if (!BCRYPT_SUCCESS(::BCryptGetProperty(
		algorithm,
		BCRYPT_OBJECT_LENGTH,
		reinterpret_cast<PUCHAR>(&cb_hash_object),
		sizeof(DWORD),
		&data,
		0)))
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
		return false;
	}

	DWORD cb_hash = 0;
	//calculate the length of the hash
	if (!BCRYPT_SUCCESS(::BCryptGetProperty(
		algorithm,
		BCRYPT_HASH_LENGTH,
		reinterpret_cast<PUCHAR>(&cb_hash),
		sizeof(DWORD),
		&data,
		0)))
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
		return false;
	}

	//allocate the hash object on the heap
	PUCHAR hash_object = static_cast<PUCHAR>(::HeapAlloc(::GetProcessHeap(), 0, cb_hash_object));
	if (!hash_object)
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
		return false;
	}

	BCRYPT_HASH_HANDLE hash_handle = nullptr;
	//create a hash
	if (!BCRYPT_SUCCESS(::BCryptCreateHash(
		algorithm,
		&hash_handle,
		hash_object,
		cb_hash_object,
		0,
		0,
		0)))
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
		::HeapFree(::GetProcessHeap(), 0, hash_object);
		return false;
	}

	//hash data
	if (!BCRYPT_SUCCESS(::BCryptHashData(
		hash_handle,
		reinterpret_cast<PUCHAR>(buffer.data()),
		buffer.size(),
		0)))
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
		::HeapFree(::GetProcessHeap(), 0, hash_object);
		::BCryptDestroyHash(hash_handle);
		return false;
	}

	//allocate the hash buffer on the heap
	PUCHAR hash = reinterpret_cast<PUCHAR>(::HeapAlloc(::GetProcessHeap(), 0, cb_hash));
	if (!hash)
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
		::HeapFree(::GetProcessHeap(), 0, hash_object);
		::BCryptDestroyHash(hash_handle);
		return false;
	}

	//close the hash
	if (!BCRYPT_SUCCESS(::BCryptFinishHash(
		hash_handle,
		hash,
		cb_hash,
		0)))
	{
		::BCryptCloseAlgorithmProvider(algorithm, 0);
		::HeapFree(::GetProcessHeap(), 0, hash_object);
		::BCryptDestroyHash(hash_handle);
		::HeapFree(::GetProcessHeap(), 0, hash);
		return false;
	}

	std::vector<char> current_hash = BytesToHexString(hash, cb_hash);
	std::string current_hash_str(current_hash.begin(), current_hash.end());

	::BCryptCloseAlgorithmProvider(algorithm, 0);
	::HeapFree(::GetProcessHeap(), 0, hash_object);
	::BCryptDestroyHash(hash_handle);
	::HeapFree(::GetProcessHeap(), 0, hash);

	return CompareHashes(app_hash, current_hash_str);
}

std::vector<char> ValidateBinary::BytesToHexString(unsigned char const *bytes, std::size_t length)
{
	std::vector<char> file_hash;

	if (bytes == nullptr)
	{
		return file_hash;
	}

	static char const lookup[] = "0123456789ABCDEF";

	for (std::size_t i = 0; i < length; ++i)
	{
		file_hash.push_back(lookup[bytes[i] >> 4]);
		file_hash.push_back(lookup[bytes[i] & 0xF]);
	}

	return file_hash;
}

bool ValidateBinary::CompareHashes(const std::wstring &hash_1, const std::string &hash_2)
{
	return std::equal(hash_1.begin(), hash_1.end(), hash_2.begin(), hash_2.end());
}
