
#include "stdafx.h"
#include "windows.h"

#include <string>
#include <iostream>
#include <vector>

#include "ProcessHandler.h"


void ProcessHandler::CreateProcess(const std::wstring &desktop_name /*, SECURITY_DESCRIPTOR sd */)
{
	// Create process in the window-station of the user

	// Create Process
	STARTUPINFO si = { 0 };
	std::vector<wchar_t> desktop_name_buf(desktop_name.begin(), desktop_name.end());
	desktop_name_buf.push_back(0);
	si.lpDesktop = desktop_name_buf.data();
	PROCESS_INFORMATION pi = { 0 };

	//SECURITY_ATTRIBUTES sa = {
	//	sizeof SECURITY_ATTRIBUTES,
	//	sd,
	//	FALSE // Does not inherit the handle
	//};

	bool success = ::CreateProcess(
		NULL, L"C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe",
		NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi
	);

	if (success)
	{
		::CloseHandle(&si);
		::CloseHandle(&pi);
	}
}
