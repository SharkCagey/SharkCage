// Shark Cage.cpp : Entry point for the program.

#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include "ProcessHandler.h"


int main(int argc, char* argv[]) {
    // Save the current desktop
    HDESK currentDesktop = GetThreadDesktop(GetCurrentThreadId());

    // Create new desktop and switch to it
	LPTSTR desktopName = L"New Desktop";
	HDESK newDesktop = CreateDesktop(desktopName, NULL, NULL, 0, GENERIC_ALL, NULL);
	SwitchDesktop(newDesktop);

	Sleep(500); // Wait to set up desktop

	// Create new process on the new desktop
	ProcessHandler pc;
	pc.createProcess(desktopName);

	Sleep(5000);

    // Return the old desktop and close the new one
    SwitchDesktop(currentDesktop);
	CloseDesktop(newDesktop);
    return 0;
}
