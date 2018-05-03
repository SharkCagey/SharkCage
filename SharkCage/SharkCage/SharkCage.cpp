// Shark Cage.cpp : Entry point for the program.

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include "ProcessHandler.h"

// Start as LocalSystem
int main()
{
	// Create CageService
	// ProcessHandler pc;
	std::wstring desktop_name = L"cage_manager_desktop";
	std::wstring appName = L"CageManager.exe";
	DWORD process_id = ::GetCurrentProcessId();
	DWORD session_id;

	if (::ProcessIdToSessionId(process_id, &session_id))
	{
		// CageService starts CageManager
		//pc.startCageManager(appName, desktopName, sessionId);
	}
	else
	{
		std::cout << "Unable to get session ID\n";
		return -1;
	}

	return 0;
}


void StartProcessInNewDesktop()
{
	// Save the current desktop
	HDESK current_desktop = ::GetThreadDesktop(::GetCurrentThreadId());

	// Create new desktop and switch to it
	std::wstring desktop_name = L"cage_desktop";
	HDESK new_desktop = ::CreateDesktop(desktop_name.c_str(), NULL, NULL, 0, GENERIC_ALL, NULL);

	if (!new_desktop)
	{
		std::cout << "Could not create new desktop" << std::endl;
		return;
	}

	if (::SwitchDesktop(new_desktop) == 0)
	{
		std::cout << "Could not switch to new desktop" << std::endl;
		::CloseDesktop(new_desktop);
		return;
	}

	// FIXME is this even necessary?
	Sleep(500); // Wait to set up desktop

	// Create new process on the new desktop
	ProcessHandler pc;
	pc.CreateProcess(desktop_name);

	// FIXME instead of sleeping save the process handle and make it clossable with a call to the shark cage applicaton?
	Sleep(5000);

	// Return the old desktop and close the new one
	::SwitchDesktop(current_desktop);
	::CloseDesktop(new_desktop);
}