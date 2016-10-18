///
/// \file cage_lib_desk.h
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage library - desktop module
/// all the dirty little helpers ;)
///
#ifndef __cage_lib_desk_h__
#define __cage_lib_desk_h__

#include "cage_lib.h"
// struct to store application infos
typedef struct _APPLICATION_INFO{
	DWORD id;
char name[256];
char window_class_name[80];
}APPLICATION_INFO, *PAPPLICATION_INFO;

BOOL start_app(APPLICATION_INFO appinfo, PSID sid_group, LPSTR mydesk);
BOOL is_process_on_desk(DWORD proc_id, LPSTR desk_name);
BOOL clean_desktop();
BOOL log_window_stations_and_desktops(void);
BOOL create_cage_desk(PSID group_sid, HDESK *desk_out);
BOOL create_caged_token(PSID group_sid, PHANDLE token_out);
BOOL generate_and_add_group_id(char **group_name_out);
BOOL delete_group(LPSTR group_name);
BOOL delete_group_w(WCHAR * group_name);
BOOL delete_all_groups(WCHAR *prefix);

// function and structs taken from windows internals
/*
NTSTATUS (WINAPI *NtQueryInformationProcess)(
	_In_      HANDLE           ProcessHandle,
	_In_      int			   ProcessInformationClass,
	_Out_     PVOID            ProcessInformation,
	_In_      ULONG            ProcessInformationLength,
	_Out_opt_ PULONG           ReturnLength
	);
	*/
#if 0
typedef struct _PEB_LDR_DATA {
	BYTE       Reserved1[8];
	PVOID      Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;
typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE           Reserved1[16];
	PVOID          Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;
typedef struct _PEB {
	BYTE                          Reserved1[2];
	BYTE                          BeingDebugged;
	BYTE                          Reserved2[1];
	PVOID                         Reserved3[2];
	PPEB_LDR_DATA                 Ldr;
	PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
	BYTE                          Reserved4[104];
	PVOID                         Reserved5[52];
	LPVOID PostProcessInitRoutine;
	BYTE                          Reserved6[128];
	PVOID                         Reserved7[1];
	ULONG                         SessionId;
} PEB, *PPEB;
typedef struct _PROCESS_BASIC_INFORMATION {
	PVOID Reserved1;
	PPEB PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;
#endif
#endif