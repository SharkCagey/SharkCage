
#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include "ProcessHandler.h"


ProcessHandler::ProcessHandler()
{
}


ProcessHandler::~ProcessHandler() {
}

void ProcessHandler::createProcess(LPTSTR desktopName/*SECURITY_DESCRIPTOR *sd*/) {
    // Create process in the windowstation of the user

	// Create Process
	LPTSTR szCmdline = _tcsdup(TEXT("C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe"));
	STARTUPINFO si = { sizeof si };
	si.lpDesktop = desktopName;
	PROCESS_INFORMATION pi;
	/*
	SECURITY_ATTRIBUTES sa = {
		sizeof SECURITY_ATTRIBUTES,
		sd,
		FALSE // Does not inherit the handle
	};*/

	bool success = CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	// Wait until child process exits.
	//WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	//CloseHandle(pi.hProcess);
	//CloseHandle(pi.hThread);
}

void ProcessHandler::startCageManager(LPCTSTR appName, LPTSTR desktopName, DWORD sessionId) {
    PHANDLE tokenHandle = nullptr; // Handle for the created access token
    HANDLE hServiceToken = nullptr;
    PHANDLE hUserSessionToken = nullptr;

    STARTUPINFO si = { sizeof si };
    si.lpDesktop = desktopName;
    PROCESS_INFORMATION pi;

    if (ImpersonateSelf(SecurityImpersonation)) {
        std::cout << "ImpersonateSelf was successful\n";
        if (OpenThreadToken(GetCurrentThread, TOKEN_ALL_ACCESS, false, tokenHandle)) {
            std::cout << "OpenThreadToken was successful\n";
            if (DuplicateTokenEx(hServiceToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, hUserSessionToken)) {
                std::cout << "DuplicateTokenEx was successful\n";
                if (SetTokenInformation(hUserSessionToken, TokenSessionId, &sessionId, sizeof DWORD)) {
                    std::cout << "SetTokenInformation was successful\n";
                    if (CreateProcessAsUser(hUserSessionToken,
                        appName,
                        NULL,
                        NULL,  // <- Process Attributes
                        NULL,  // Thread Attributes
                        false, // Inheritaion flags
                        0,     // Creation flags
                        NULL,  // Environment
                        NULL,  // Current directory
                        &si,   // Startup Info
                        &pi)) {
                        std::cout << "CreateProcess (" << appName << ") was succesful\n";
                        CloseHandle(&si);
                        CloseHandle(&pi);
                    }
                    else {
                        std::cout << "CreateProcess (" << appName << ") failed\n";
                    }
                }
                else {
                    std::cout << "SetTokenInformation failed\n";
                }
            }
            else
                std::cout << "DuplicateTokenEx failed\n";
        }
        else
            std::cout << "OpenThreadToken failed\n";
    }
    else
        std::cout << "ImpersonateSelf failed\n";
}
