///
/// \file helper.c
/// \author Richard Heininger - richmont12@gmail.com
/// unsharked - helper functions and modules
/// function descriptions are in the header file
///

#include <time.h>
#include <strsafe.h>

//#include "stdafx.h"
#include "cage_lib.h"


char** alloc_list(int entries, int length) {
	char **list = malloc(sizeof(char*)*entries);
	for (int a = 0; a < entries; a++) {
		list[a] = malloc(sizeof(char)*length);
		for (int b = 0; b < length; b++) {
			list[a][b] = '\0';
		}
	}
	return list;
}

void free_list(int entries, char** list) {
	for (int a = 0; a < entries; a++) {
		free(list[a]);
	}
	free(list);
}

// for desc. see header file
BOOL CALLBACK enum_station_callback(LPSTR station, LPARAM  list) {
	station = (char*)station;
	char** l = (char**)list;
	if (l == NULL) {
		return FALSE;
	}
	// copy station string to list where is the next free entry
	for (int i = 0; i < MAX_STATION_ENTRIES; i++) {
		// found free entry
		if (l[i][0] == '\0') {
			// copy string station to this place!
			sprintf_s(l[i], sizeof(char)*MAX_STATION_NAME, station);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CALLBACK enum_desktop_callback(LPTSTR desktop, LPARAM list) {
	desktop = (char*)desktop;
	char** l = (char**)list;
	if (l == NULL) {
		return FALSE;
	}
	// copy station string to list where is the next free entry
	for (int i = 0; i < MAX_DESKTOP_ENTRIES; i++) {
		// found free entry
		if (l[i][0] == '\0') {
			// copy string station to this place!
			sprintf_s(l[i], sizeof(char)*MAX_DESKTOP_NAME, desktop);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL init_logging(LPSTR log_file_name, PHANDLE loghandle){
	SYSTEMTIME sys_time;
	GetSystemTime(&sys_time);
	HANDLE log;
	char logfile[256];
	sprintf_s(logfile, 255, "%02d%02d%04d_%02d%02d%02d_%s\0", sys_time.wDay, sys_time.wMonth, sys_time.wYear, sys_time.wHour, sys_time.wMinute, sys_time.wSecond, log_file_name);

	log = CreateFileA(logfile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	*loghandle = log;

	if (log == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	return TRUE;
}

BOOL add_logging_line(HANDLE loghandle, LPCSTR logmsg){
	DWORD bytesWritten = 0;
	if (!WriteFile(loghandle, logmsg, lstrlenA(logmsg), &bytesWritten, NULL)) {
		return FALSE;
	}
	return TRUE;
}

BOOL add_logging_syserror(HANDLE loghandle, DWORD error_code){
	LPVOID error_buffer;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&error_buffer,
		0, NULL);
	if (!add_logging_line(loghandle, error_buffer)) {
		LocalFree(error_buffer);
		return FALSE;
	}
	LocalFree(error_buffer);
	return TRUE;
}

BOOL get_permissions_as_string(DWORD permissions, char *buffer, DWORD buffer_length) {
	for (unsigned i = 0; i < buffer_length; i++) {
		buffer[i] = '\0';
	}
	DWORD count = 0;
	// standart rights for all objects
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DELETE, "DELETE ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, READ_CONTROL, "READ_CONTROL ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, SYNCHRONIZE, "SYNCHRONIZE ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, WRITE_DAC, "WRITE_DAC ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, WRITE_OWNER, "WRITE_OWNER ");
	// desktop specific access rights
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_CREATEMENU, "DESKTOP_CREATEMENU ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_CREATEWINDOW, "DESKTOP_CREATEWINDOW ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_ENUMERATE, "DESKTOP_ENUMERATE ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_HOOKCONTROL, "DESKTOP_HOOKCONTROL ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_JOURNALPLAYBACK, "DESKTOP_JOURNALPLAYBACK ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_JOURNALRECORD, "DESKTOP_JOURNALRECORD ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_READOBJECTS, "DESKTOP_READOBJECTS ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_SWITCHDESKTOP, "DESKTOP_SWITCHDESKTOP ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, DESKTOP_WRITEOBJECTS, "DESKTOP_WRITEOBJECTS ");


	StringCbCatA(&buffer[count] - 1, sizeof('\0\0'), "\0\0");
	return TRUE;
}