///
/// \file cage_lib.h
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage lib - main module
///
#ifndef __cage_lib_h__
#define __cage_lib_h__



#include <Windows.h>
#include <WtsApi32.h>
#include <stdio.h>
#include <AclAPI.h>
#include <Sddl.h>
#include <strsafe.h>

#include "protocol.h"
#include "cage_lib_token.h"
#include "cage_lib_desk.h"

// mail slot stuff
#define MAILSLOT_NAME_MANAGER	TEXT("\\\\.\\mailslot\\shark_cage_manager")
#define MAILSLOT_NAME_SERVICE	TEXT("\\\\.\\mailslot\\shark_cage_service")
#define MAILSLOT_NAME_LABELLER	TEXT("\\\\.\\mailslot\\shark_cage_labeller")
BOOL write_to_slot(PHANDLE slot, char *message);
BOOL read_from_slot(PHANDLE slot, char **buffer);
BOOL open_mailslot_in(PHANDLE slot, LPCSTR slotname);
BOOL open_mailslot_out(PHANDLE slot, LPCSTR slotname);




// logging helper
#define LOGGING
#ifdef LOGGING
extern HANDLE log;
#define LOG(msg, ...)	{ \
	char line[1024]; \
	sprintf_s(line, 1024, msg, __VA_ARGS__); \
	add_logging_line(log, line); \
}
BOOL add_logging_line(HANDLE loghandle, LPCSTR logmsg);
BOOL add_logging_syserror(HANDLE loghandle, DWORD error_code);
BOOL init_logging(PHANDLE log, char *prefix);
//HANDLE log;
#else
#define LOG(msg, ...)
#define add_logging_syserror(a,b)
#define init_logging(a,b)
#endif


// helper functions
// allocates memory for a 2d char array
char** alloc_list(int entries, int length);
// frees the allocated memory from a 2d array
void free_list(int entries, char** list);




// station and desktop stuff
#define DESK_LOGON				"WinSta0\\Winlogon\0"
#define DESK_DEFAULT			"WinSta0\\Default\0"
#define MAX_STATION_ENTRIES		8		// maximum amount of station names that the buffer is able to hold
#define MAX_STATION_NAME		256		// maximum length of an station name in the buffer
#define MAX_DESKTOP_ENTRIES		8		// maximum amount of desktop names that the buffer is able to hold
#define MAX_DESKTOP_NAME		256		// maximum length of an desktop name in the buffer
#define MAX_WINDOW_ENTRIES		256
#define MAX_WINDOW_NAME			160
// callback to get window station names on this machine
BOOL CALLBACK enum_station_callback(LPSTR station, LPARAM  list);
// callback to geht desktop names associated with specified window station
BOOL CALLBACK enum_desktop_callback(LPTSTR desktop, LPARAM list);
// callback to get all windows on a desktop
BOOL CALLBACK enum_window_callback(HWND   hwnd, LPARAM lParam);


#define MAX_APPLICATIONS	10






// token info stuff
BOOL read_and_log_user_from_token(HANDLE token);
BOOL read_and_log_group_from_token(HANDLE token);
BOOL read_and_log_privileges_from_token(HANDLE token);
BOOL read_and_log_owner_from_token(HANDLE token);
BOOL read_and_log_primary_group_from_token(HANDLE token);
BOOL read_and_log_default_acl_from_token(HANDLE token);
BOOL read_and_log_token_source_from_token(HANDLE token);
BOOL read_and_log_token_type_from_token(HANDLE token);
BOOL read_and_log_token_impersonation_level_from_token(HANDLE token);
BOOL read_and_log_token_statistics_from_token(HANDLE token);
BOOL read_and_log_token_restricted_token_from_token(HANDLE token);
BOOL read_and_log_session_id_from_token(HANDLE token);
BOOL read_and_log_groups_and_privileges_from_token(HANDLE token);
// ----------------------
BOOL read_and_log_token_sandbox_inert_from_token(HANDLE token);
BOOL read_and_log_token_origin_from_token(HANDLE token);
BOOL read_and_log_token_elevation_type_from_token(HANDLE token);
BOOL read_and_log_token_linked_token_from_token(HANDLE token);
BOOL read_and_log_token_elevation_from_token(HANDLE token);
BOOL read_and_log_token_has_restrictions_from_token(HANDLE token);
// ----------------------
BOOL log_token(HANDLE token);
BOOL log_acl(PACL acl);
BOOL read_and_log_object(HANDLE h);
BOOL get_permissions_as_string(DWORD permissions, char *buffer, DWORD buffer_length);
#define GET_STRING_FROM_RIGHTS(buf, count, permissions, right, right_s)		if ((permissions & right)==right) { \
	StringCbCatA(&buf[count], sizeof(right_s)+1, right_s); \
	count += sizeof(right_s)-1; \
}

// ---------------------------
//svoid log_privileges_from_token(HANDLE token);


#endif