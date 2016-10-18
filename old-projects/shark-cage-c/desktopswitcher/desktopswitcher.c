#include <Windows.h>
#include "cage_lib.h"


BOOL WINAPI switch_desktop(HDESK desk);
HANDLE log = NULL;

int main() {
	init_logging(&log, "desktopswitcher");

	log_window_stations_and_desktops();

	HDESK desk = OpenDesktopA("mydesk", DF_ALLOWOTHERACCOUNTHOOK, TRUE, GENERIC_ALL);
	if (desk == NULL) {
		DWORD err = GetLastError();
		LOG("opendesktop failed. er=%d\r\n",err);
		add_logging_syserror(log, err);
	}
	
	switch_desktop(desk);

	CloseHandle(log);
	return EXIT_SUCCESS;
}


BOOL WINAPI switch_desktop(HDESK desk) {

	HWINSTA winsta = OpenWindowStation("WinSta0", FALSE, MAXIMUM_ALLOWED);
	if (winsta == NULL) {
		DWORD err = GetLastError();
		LOG("openwindowstation failed. er=%d\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	if (!SetProcessWindowStation(winsta))  {
		DWORD err = GetLastError();
		LOG("setprocesswindowstation failed. er=%d\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	/*
	if (!SetThreadDesktop(desk)) {
		DWORD err = GetLastError();
		LOG("settrheaddesktop failed. er=%d\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}*/
	if (!SwitchDesktop(desk)) {
		DWORD err = GetLastError();
		LOG("switchdesktop failed. er=%d\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}


	return TRUE;
}