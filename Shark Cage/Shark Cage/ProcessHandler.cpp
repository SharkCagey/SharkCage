
#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include "ProcessHandler.h"


ProcessHandler::ProcessHandler()
{
}


ProcessHandler::~ProcessHandler()
{
}

void ProcessHandler::createProcess() {
	// Create Process
	LPTSTR szCmdline = _tcsdup(TEXT("C:\\Windows\\System32\\calc.exe"));
	LPTSTR param = L"";	// This does not work
	STARTUPINFO si = { sizeof si };
	PROCESS_INFORMATION pi;

	bool success = CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	if (success)
		std::cout << "success \n";
	else
		std::cout << "failure \n";
}