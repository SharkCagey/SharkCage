///
/// \file cage_lib_token.c
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage library - token module
/// all the dirty little helpers ;)
///
#include <Windows.h>
#include <WtsApi32.h>
#include <stdio.h>
#include <AclAPI.h>
#include <Sddl.h>
#include <LM.h>
#include <strsafe.h>
#include <Psapi.h>


#include "cage_lib.h"

BOOL get_privileged_process(LUID luid, PDWORD privileged_pid_out) {

	DWORD procs[1024];
	ZeroMemory(procs, sizeof(procs));
	DWORD procs_length;
	if (!EnumProcesses(procs, sizeof(procs), &procs_length)) {
		DWORD err = GetLastError();
		LOG("failed to enumerate processes.\r\n");
		add_logging_syserror(log, err);
		return FALSE;
	}

	DWORD procs_count = procs_length / sizeof(DWORD);
	//LOG("we have %d running processes.\r\n", procs_count);
	for (unsigned int b = 0; b < procs_count; b++) {
		//LOG("checking process number %d with pid %d.\r\n", b, procs[b]);
		HANDLE proc_tmp = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procs[b]);
		if (proc_tmp != NULL) {
			HANDLE tmp_token = NULL;
			if (OpenProcessToken(proc_tmp, TOKEN_ALL_ACCESS, &tmp_token)) {
				DWORD buffer_length = 0;
				GetTokenInformation(tmp_token, TokenPrivileges, NULL, buffer_length, &buffer_length);
				LPVOID buffer = malloc(buffer_length);
				ZeroMemory(buffer, buffer_length);

				if (GetTokenInformation(tmp_token, TokenPrivileges, buffer, buffer_length, &buffer_length)) {
					PTOKEN_PRIVILEGES info = (PTOKEN_PRIVILEGES)buffer;
					for (unsigned i = 0; i < info->PrivilegeCount; i++) {
						if (info->Privileges[i].Luid.HighPart == luid.HighPart && info->Privileges[i].Luid.LowPart == luid.LowPart) {
							//LOG("\tfound privileg in token %x from process with pid %d  (%3d/%3d)\r\n", tmp_token, procs[b], b, procs_count);
							*privileged_pid_out = procs[b];
							break;
						}
					}					
				}
				else {
					//LOG("failed to get token information from pid %d\r\n", procs[b]);
				}
				free(buffer);
			}
			CloseHandle(tmp_token);
		}
		else {
			//DWORD err = GetLastError();
			//LOG("failed to open process with pid %d. err=%d\r\n", procs[b], err);
			//add_logging_syserror(log, err);
			//return FALSE;
		}
		CloseHandle(proc_tmp);
	}
	if (*privileged_pid_out != 4 && *privileged_pid_out != 0)
		return TRUE;
	else
		return FALSE;
}


BOOL get_sid(char *name, PSID *sid) {

	PSID group_sid = NULL;
	DWORD group_sid_cb = 0;
	LPTSTR group_ref_domain = NULL;
	DWORD group_ref_dom_cb = 0;
	SID_NAME_USE group_sid_type = 0;

	LookupAccountName(NULL, name, group_sid, &group_sid_cb, group_ref_domain, &group_ref_dom_cb, &group_sid_type);
	group_sid = malloc(group_sid_cb);
	if (group_sid == NULL) {
		return FALSE;
	}
	group_ref_domain = malloc(sizeof(char)*group_ref_dom_cb);
	if (group_ref_domain == NULL) {
		free(group_sid);
		return FALSE;
	}

	if (!LookupAccountNameA(NULL, name, group_sid, &group_sid_cb, group_ref_domain, &group_ref_dom_cb, &group_sid_type)) {
		DWORD err = GetLastError();
		LOG("failed to get PID from GROUP name %s. errorcode=%u\r\n", name, err);
		add_logging_syserror(log, err);
		free(group_sid);
		free(group_ref_domain);
		return FALSE;
	}

	free(group_ref_domain);
	*sid = group_sid;
	return TRUE;
}





BOOL get_token_functions(void) {
	ZwCreateToken = (PVOID)GetProcAddress(GetModuleHandle("ntdll.dll"), "ZwCreateToken");
	if (ZwCreateToken == NULL) {
		DWORD err = GetLastError();
		LOG("failed to get ZwCreateToken function Pointer. errorcode=%u\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	else {
		//LOG("successfull got ZwCreateToken function Pointers.\r\n");
	}
	/*
	RtlNtStatusToDosError = (PVOID)GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlNtStatusToDosError");
	if (RtlNtStatusToDosError == NULL) {
		DWORD err = GetLastError();
		LOG("failed to get RtlNtStatusToDosError function Pointer. errorcode=%u\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}

	else {
		//LOG("successfull got RtlNtStatusToDosError function Pointers.\r\n");
		return TRUE;
	}*/
	return TRUE;
}

BOOL create_process_on_desk(LPSTR process, LPSTR desk_name, HANDLE token) {



	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	//LOG("strlen(desk_name)=%d  desk_name=%s\r\n ", strlen(desk_name),desk_name);
	char *desk = malloc(strlen(desk_name)+1);
	if (desk == NULL) {
		return FALSE;
	}
	ZeroMemory(desk, strlen(desk_name)+1);
	//strcpy_s(&desk, strlen(desk_name), desk_name);
	for (unsigned i = 0; i < strlen(desk_name); i++) {
		desk[i] = desk_name[i];
		//LOG("i:%d desk[i]=%c desk_name[i]=%c \r\n",i,desk[i],desk_name[i]);
	}
	desk[strlen(desk_name)] = '\0';
	//LOG("strlen(desk)=%d  desk=%s \r\n", strlen(desk), desk);
	si.lpDesktop = desk;

	//si.dwFlags = STARTF_USESHOWWINDOW;
	//si.wShowWindow = SW_SHOWNORMAL;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcessAsUserA(token, process, "", NULL, NULL, TRUE, CREATE_NEW_CONSOLE | INHERIT_CALLER_PRIORITY, NULL, NULL, &si, &pi)) {
		DWORD err = GetLastError();
		LOG("failed to create process %s on desk %s with token %x. errorcode=%d\r\n", process, desk, token, err);
		add_logging_syserror(log, err);
		free(desk);
		return FALSE;
	}
	else {
		LOG("successfull created %s on desk %s.\r\n",process,desk);
		free(desk);
		return TRUE;
	}
}


BOOL create_manager_token(HANDLE token_in, DWORD session_id, LUID luid_to_enable, PHANDLE token_out) {	
	HANDLE token_o = NULL;
	LPSECURITY_ATTRIBUTES sec_attr = NULL;
	// TokenImpersonation to TokenPrimary 
	if (!DuplicateTokenEx(token_in, GENERIC_ALL | TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, sec_attr, SecurityImpersonation, TokenPrimary, &token_o)) {
		DWORD err = GetLastError();
		LOG("failed to duplicate token. errorcode=%d\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	// enable given privilege
	TOKEN_PRIVILEGES token_priv;
	ZeroMemory(&token_priv, sizeof(TOKEN_PRIVILEGES));
	token_priv.PrivilegeCount = 1;
	token_priv.Privileges[0].Luid = luid_to_enable;
	token_priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	TOKEN_PRIVILEGES token_priv_old;
	DWORD token_priv_old_length = 0;
	ZeroMemory(&token_priv_old, sizeof(TOKEN_PRIVILEGES));
	token_priv_old = token_priv;

	// 1 is disabled(but enabled_by_default) 2 is enabled, 3 is enabled and enabled by default
	if (!AdjustTokenPrivileges(token_o, FALSE, &token_priv, sizeof(TOKEN_PRIVILEGES), &token_priv_old, &token_priv_old_length)) {
		DWORD err = GetLastError();
		LOG("failed to AdjustTokenPrivileges for given privileg. errorcode=%d\r\n", err);
		add_logging_syserror(log, err);
		CloseHandle(token_o);
		return FALSE;
	}

	// change session to input session
	DWORD bla = session_id;
	if (!SetTokenInformation(token_o, TokenSessionId, &bla, sizeof(DWORD))) {
		DWORD err = GetLastError();
		LOG("failed to set new session %d in token. errorcode=%d\r\n", session_id, err);
		add_logging_syserror(log, err);
		CloseHandle(token_o);
		return FALSE;
	}
	*token_out = token_o;
	return TRUE;
}


BOOL get_token_with_privileg(LUID privilege, PHANDLE token_out) {	
	DWORD pid_with_priv = 4;


	if (!get_privileged_process(privilege, &pid_with_priv)) {
		LOG("failed to get privileged process id.\r\n");
		return FALSE;
	}
#if 0
	DWORD procs[1024];
	ZeroMemory(procs, sizeof(procs));
	DWORD procs_length;
	if (!EnumProcesses(procs, sizeof(procs), &procs_length)) {
		DWORD err = GetLastError();
		LOG("failed to enumerate processes.\r\n");
		add_logging_syserror(log, err);
		return FALSE;
	}

	DWORD procs_count = procs_length / sizeof(DWORD);
	LOG("we have %d running processes.\r\n", procs_count);
	for (unsigned int b = 0; b < procs_count; b++) {
		//LOG("checking process number %d with pid %d.\r\n", b, procs[b]);
		HANDLE proc_tmp = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procs[b]);
		if (proc_tmp != NULL) {
			HANDLE tmp_token = NULL;
			if (OpenProcessToken(proc_tmp, TOKEN_ALL_ACCESS, &tmp_token)) {
				DWORD buffer_length = 0;
				GetTokenInformation(tmp_token, TokenPrivileges, NULL, buffer_length, &buffer_length);
				LPVOID buffer = malloc(buffer_length);
				ZeroMemory(buffer, buffer_length);

				if (GetTokenInformation(tmp_token, TokenPrivileges, buffer, buffer_length, &buffer_length)) {
					PTOKEN_PRIVILEGES info = (PTOKEN_PRIVILEGES)buffer;
					for (unsigned i = 0; i < info->PrivilegeCount; i++) {
						if (info->Privileges[i].Luid.HighPart == luid.HighPart && info->Privileges[i].Luid.LowPart == luid.LowPart) {
							LOG("\tfound privileg in token %x from process with pid %d  (%3d/%3d)\r\n", tmp_token, procs[b], b, procs_count);
							pid_with_priv = procs[b];	
							break;
						}
					}
				}
				else {
					//LOG("failed to get token information from pid %d\r\n", procs[b]);
				}
				free(buffer);
			}
			CloseHandle(tmp_token);
		}
		else {
			DWORD err = GetLastError();
			//LOG("failed to open process with pid %d. err=%d\r\n", procs[b], err);
			//add_logging_syserror(log, err);
		}
		CloseHandle(proc_tmp);
	}
#endif
	HANDLE process_with_priv = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid_with_priv);
	if (process_with_priv == NULL) {
		DWORD err = GetLastError();
		LOG("failed to open process with pid %d.\r\n", pid_with_priv);
		add_logging_syserror(log, err);
		return FALSE;
	}
	HANDLE priv_token = NULL;
	if (!OpenProcessToken(process_with_priv, TOKEN_ALL_ACCESS, &priv_token)) {
		DWORD err = GetLastError();
		LOG("failed to open process token %x.\r\n", process_with_priv);
		add_logging_syserror(log, err);
		CloseHandle(process_with_priv);
		return FALSE;
	}
	*token_out = priv_token;
	return TRUE;

}

BOOL get_active_session(PDWORD session_id_active) {
	LOG("searching for active session id.\r\n");
	PWTS_SESSION_INFO session_info = NULL;
	DWORD session_count = 0;
	//int session_id_active = 0;
	if (!WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &session_info, &session_count)) {
		LOG("failed to enumerate sessions.\r\n");
		return FALSE;
	}
	BOOL goon = TRUE;
	int session_size = sizeof(WTS_SESSION_INFO);
	for (DWORD i = 0; i < session_count; i++) {
		WTS_SESSION_INFOA si = session_info[i];
		LOG("\tfound session %u station %s state %d \r\n", si.SessionId, si.pWinStationName, si.State);
		if (si.State == WTSActive && goon) {
			*session_id_active = si.SessionId;
			LOG("\tchosen %u (%d) \r\n", si.SessionId, si.SessionId);			
			goon = FALSE;
		}
	}
	WTSFreeMemory(session_info);
	return TRUE;
}
