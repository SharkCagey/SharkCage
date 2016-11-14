// ProccessInSaveDesktop.cpp: define el punto de entrada de la aplicación de consola.
//


#include "stdafx.h"
#include "Windows.h"
#include "stdio.h"
#include "Aclapi.h"
#include "tchar.h"
#include "Sddl.h"

int main()
{
	DWORD dwRes;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea[1];
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
		SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	SECURITY_ATTRIBUTES sa;

	HDESK newDesktop = NULL;

	

	//ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));
	
	// Create a well-known SID for the Everyone group.
	
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
		0,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID))
	{
		_tprintf(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup;
	}
	

	LPCTSTR cadena1 = (LPCTSTR)"S-1-5-21-2391956980-2875960972-1841609823-1001";
	//LPCTSTR cadena2 = (LPCTSTR)"desktop-inijdd6\raquel";

	ConvertStringSidToSid(cadena1, &pEveryoneSID);

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone read access to the key.
	ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));
	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)pEveryoneSID;


	/*
	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdminSID))
	{
		_tprintf(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup;
	}


	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to
	// the key.
	//ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)pAdminSID; //HERE WE MUST SET THE GROUP SID
	*/
												 // Create a new ACL that contains the new ACEs.
	dwRes = SetEntriesInAcl(1, ea, NULL, &pACL);
	if (ERROR_SUCCESS != dwRes)
	{
		_tprintf(_T("SetEntriesInAcl Error %u\n"), GetLastError());
		goto Cleanup;
	}

	// Initialize a security descriptor.  
	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (NULL == pSD)
	{
		_tprintf(_T("LocalAlloc Error %u\n"), GetLastError());
		goto Cleanup;
	}

	if (!InitializeSecurityDescriptor(pSD,
		SECURITY_DESCRIPTOR_REVISION))
	{
		_tprintf(_T("InitializeSecurityDescriptor Error %u\n"),
			GetLastError());
		goto Cleanup;
	}

	// Add the ACL to the security descriptor. 
	if (!SetSecurityDescriptorDacl(pSD,
		TRUE,     // bDaclPresent flag   
		pACL,
		FALSE))   // not a default DACL 
	{
		_tprintf(_T("SetSecurityDescriptorDacl Error %u\n"),
			GetLastError());
		goto Cleanup;
	}

	// Initialize a security attributes structure.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	// Use the security attributes to set the security descriptor 
	// when you create a desktop.
	newDesktop = CreateDesktop(TEXT("Test"), NULL, NULL, NULL, GENERIC_ALL, &sa);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//SAVE THE OLD DESKTOP. This is in order to come back to our desktop.
	HDESK oldDesktop = GetThreadDesktop(GetCurrentThreadId());

	if (newDesktop != NULL) {

		//Switch to the new desktop.
		SwitchDesktop(newDesktop);

		//The path where is the application that we are going to start in the new desktop.
		LPTSTR path = _tcsdup(TEXT("C:\\Program Files\\Notepad++\\notepad++.exe"));

		//We need in order to create the process.
		STARTUPINFO info = { sizeof(info) };
		PROCESS_INFORMATION processInfo;

		//The desktop's name where we are going to start the application. In this case, our new desktop.
		LPTSTR desktop = _tcsdup(TEXT("DesktopName"));
		info.lpDesktop = desktop;

		char* dominio = "dekstop-inijdd6";

		//Create the process.
		if (!CreateProcess(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
		{
			WaitForSingleObject(processInfo.hProcess, INFINITE);
			printf("proceso no creado");

		}
		else {
			//Handle error here.
			printf("error %d",GetLastError());
		}


		//In order to see the desktop.
		Sleep(5000);

		//SWITCH TO THE OLD DESKTOP. This is in order to come back to our desktop.
		SwitchDesktop(oldDesktop);

	}
	else {
		//Handle error here.
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Cleanup:

	if (pEveryoneSID)
		FreeSid(pEveryoneSID);
	if (pAdminSID)
		FreeSid(pAdminSID);
	if (pACL)
		LocalFree(pACL);
	if (pSD)
		LocalFree(pSD);

	return 0;

}

