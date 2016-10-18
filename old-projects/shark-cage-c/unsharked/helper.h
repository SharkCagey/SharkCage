///
/// \file helper.h
/// \author Richard Heininger - richmont12@gmail.com
/// unsharked - helper functions and modules header
///
#ifndef __helper_h__
#define __helper_h__

#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>


#define MAX_STATION_ENTRIES		8		// maximum amount of station names that the buffer is able to hold
#define MAX_STATION_NAME		256		// maximum length of an station name in the buffer
#define MAX_DESKTOP_ENTRIES		8		// maximum amount of desktop names that the buffer is able to hold
#define MAX_DESKTOP_NAME		256		// maximum length of an desktop name in the buffer




// access control https://msdn.microsoft.com/en-us/library/windows/desktop/ms682124(v=vs.85).aspx
//#define DF_ALLOWOTHERACCOUNTHOOK		 (0x0001)


// allocates memory for a 2d char array
char** alloc_list(int entries, int length);
// frees the allocated memory from a 2d array
void free_list(int entries, char** list);

// saves given acces permissions in a human readable format to the given buffer
BOOL get_permissions_as_string(DWORD permissions, char *buffer, DWORD buffer_length);



// callback to get window station names on this machine
BOOL CALLBACK enum_station_callback(LPSTR station, LPARAM  list);
// callback to geht desktop names associated with specified window station
BOOL CALLBACK enum_desktop_callback(LPTSTR desktop, LPARAM list);


// inits the logging, according to the parameters
BOOL init_logging(LPSTR logfile, PHANDLE loghandle);
// saves a logmessage to the logfile
BOOL add_logging_line(HANDLE loghandle, LPCSTR logmsg);
// adds the string value for the given error code to the log
BOOL add_logging_syserror(HANDLE loghandle, DWORD error_code);

// adds a formatted message to the logfile, max 1023 chars long
#define LOG(msg, ...)	{ \
	char line[1024]; \
	sprintf_s(line, 1024, _T(msg), __VA_ARGS__); \
	add_logging_line(logger, line); \
}

#define GET_STRING_FROM_RIGHTS(buf, count, permissions, right, right_s)		if ((permissions & right)==right) { \
	StringCbCatA(&buf[count], sizeof(right_s)+1, right_s); \
	count += sizeof(right_s)-1; \
}






#endif