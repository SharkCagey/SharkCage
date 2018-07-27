#include "stdafx.h"
#include "ValidateBinary.h"

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <bcrypt.h>
#include <vector>
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

	switch (status)
	{
	case ERROR_SUCCESS:
		// sucessfully verified the signature
		break;

	case TRUST_E_NOSIGNATURE:
		DWORD last_error;
		last_error = ::GetLastError();
		if (TRUST_E_NOSIGNATURE == last_error ||
			TRUST_E_SUBJECT_FORM_UNKNOWN == last_error ||
			TRUST_E_PROVIDER_UNKNOWN == last_error)
		{
			// file not signed, maybe interesting to switch automatically to check the hash
		}
		else
		{
			// unkown error occured
		}

		break;

	default:
		break;
	}

	WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

	return status == ERROR_SUCCESS;
}

void hexString(unsigned char const* bytes, std::size_t bytesLength, char* dest)
{
	static char const lookup[] = "0123456789ABCDEF";

	for (std::size_t i = 0; i != bytesLength; ++i)
	{
		dest[2 * i] = lookup[bytes[i] >> 4];
		dest[2 * i + 1] = lookup[bytes[i] & 0xF];
	}
}

bool ValidateBinary::ValidateHash(const std::wstring &app_path, const std::wstring &app_hash)
{
	//open file
	std::ifstream infile(app_path, std::ios::binary);
	std::vector<char> buffer;


	//get length of file
	infile.seekg(0, infile.end);
	size_t length = (size_t) infile.tellg();
	infile.seekg(0, infile.beg);

	char dest[2 * 64 + 1];

	//read file
	if (length > 0) {
		buffer.resize(length);
		infile.read(&buffer[0], length);
	}

	BCRYPT_ALG_HANDLE algorithm = nullptr;
	BCRYPT_HASH_HANDLE hash_handle = nullptr;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DWORD data = 0, cb_hash = 0, cb_hash_object = 0;
	PBYTE hash_object = nullptr;
	PUCHAR hash = nullptr;
	std::string test;

	//open an algorithm handle
	if (!NT_SUCCESS(status = ::BCryptOpenAlgorithmProvider(
		&algorithm,
		BCRYPT_SHA512_ALGORITHM,
		nullptr,
		0)))
	{
		wprintf(L"**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
		goto Cleanup;
	}

	//calculate the size of the buffer to hold the hash object
	if (!NT_SUCCESS(status = ::BCryptGetProperty(
		algorithm,
		BCRYPT_OBJECT_LENGTH,
		(PBYTE)&cb_hash_object,
		sizeof(DWORD),
		&data,
		0)))
	{
		wprintf(L"**** Error 0x%x returned by BCryptGetProperty\n", status);
		goto Cleanup;
	}

	//allocate the hash object on the heap
	hash_object = (PBYTE)HeapAlloc(::GetProcessHeap(), 0, cb_hash_object);
	if (!hash_object)
	{
		wprintf(L"**** memory allocation failed\n");
		goto Cleanup;
	}

	//calculate the length of the hash
	if (!NT_SUCCESS(status = ::BCryptGetProperty(
		algorithm,
		BCRYPT_HASH_LENGTH,
		(PBYTE)&cb_hash,
		sizeof(DWORD),
		&data,
		0)))
	{
		wprintf(L"**** Error 0x%x returned by ::BCryptGetProperty\n", status);
		goto Cleanup;
	}

	//allocate the hash buffer on the heap
	hash = (PBYTE)HeapAlloc(::GetProcessHeap(), 0, cb_hash);
	if (!hash)
	{
		wprintf(L"**** memory allocation failed\n");
		goto Cleanup;
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
		wprintf(L"**** Error 0x%x returned by BCryptCreateHash\n", status);
		goto Cleanup;
	}


	//hash some data
	if (!NT_SUCCESS(status = ::BCryptHashData(
		hash_handle,
		reinterpret_cast<PUCHAR>(buffer.data()),
		buffer.size(),
		0)))
	{
		wprintf(L"**** Error 0x%x returned by BCryptHashData\n", status);
		goto Cleanup;
	}

	//close the hash
	if (!NT_SUCCESS(status = ::BCryptFinishHash(
		hash_handle,
		hash,
		cb_hash,
		0)))
	{
		wprintf(L"**** Error 0x%x returned by BCryptFinishHash\n", status);
		goto Cleanup;
	}

	hexString(hash, cb_hash, dest);
	dest[2 * cb_hash] = 0;

	// TODO compare hashes

Cleanup:
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

	return true;
}
