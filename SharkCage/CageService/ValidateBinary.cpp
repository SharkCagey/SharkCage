#include "stdafx.h"
#include "ValidateBinary.h"

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>

#pragma comment (lib, "wintrust")

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

bool ValidateBinary::ValidateHash(const std::wstring & app_path)
{
	return false;
}
