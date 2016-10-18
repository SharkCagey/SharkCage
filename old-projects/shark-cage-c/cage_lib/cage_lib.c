///
/// \file cage_lib.c
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage library - main module
/// all the dirty little helpers ;)
///


#include "cage_lib.h"

#ifdef LOGGING
BOOL add_logging_line(HANDLE loghandle, LPCSTR logmsg){
	DWORD bytesWritten = 0;
	SYSTEMTIME sys_time;
	GetSystemTime(&sys_time);
	char msg[1024];
	sprintf_s(msg, 1024, "[%02d%02d%02d%03d] %s", sys_time.wHour, sys_time.wMinute, sys_time.wSecond, sys_time.wMilliseconds, logmsg);
	if (!WriteFile(loghandle, msg, lstrlenA(msg), &bytesWritten, NULL)) {
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
BOOL init_logging(PHANDLE log, char *prefix) {
	SYSTEMTIME sys_time;
	GetSystemTime(&sys_time);
	char logfile[256];
	sprintf_s(logfile, 255, "c:\\log\\%02d%02d%04d_%02d%02d%02d_%s_%s.txt\0", sys_time.wDay, sys_time.wMonth, sys_time.wYear, sys_time.wHour, sys_time.wMinute, sys_time.wSecond, prefix, "log");
	*log = CreateFileA(logfile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (*log == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	return TRUE;
}
#endif



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

BOOL CALLBACK enum_window_callback(_In_ HWND   hwnd, _In_ LPARAM lParam) {
	char **buf = (char**)lParam;
	char class_name[80];
	char title[80];
	GetClassNameA(hwnd, class_name, sizeof(class_name));
	GetWindowText(hwnd, title, sizeof(title));
	for (int i = 0; i < MAX_WINDOW_ENTRIES; i++) {
		// found free entry
		if (buf[i][0] == '\0') {
			// copy string station to this place!
			sprintf_s(buf[i], sizeof(char)*MAX_WINDOW_NAME, "%s:%s\0",class_name,title);
			
			return TRUE;
		}
	}
	return FALSE;
}


BOOL read_from_slot(PHANDLE slot, char **buffer) {
	DWORD next_message_size, message_amount;
	//LOG("1\r\n");
	if (!GetMailslotInfo(*slot, NULL, &next_message_size, &message_amount, NULL)) {
		DWORD err = GetLastError();
		LOG("mailslotinfo fail err=%u\r\n",err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	//LOG("2\r\n");
	if (next_message_size == MAILSLOT_NO_MESSAGE) {
		return TRUE;
	}

	HANDLE event = CreateEvent(NULL, FALSE, FALSE, TEXT("overlapevent"));
	if (event == NULL) {
		LOG("failed to create event for sending to mailslot.\r\n");
		return FALSE;
	}
	OVERLAPPED ol;
	ol.Offset = 0;
	ol.OffsetHigh = 0;
	ol.hEvent = event;

	while (message_amount != 0) {

		char *buf = malloc(sizeof(char)*next_message_size);

		if (buf == NULL) {
			LOG("no mem \r\n");
			return FALSE;
		}
		buf[0] = '\0';
		DWORD count = 0;
		if (!ReadFile(*slot, buf, next_message_size, &count, &ol)) {
			free(buf);
			LOG("!readfile\r\n");
			return FALSE;
		}

		// have msg in buffer, count long
		sprintf_s(*buffer, count + 1, "%s", buf);

		free(buf);
		if (!GetMailslotInfo(*slot, NULL, &next_message_size, &message_amount, NULL)) {
			return FALSE;
		}
	}
	CloseHandle(event);
	return TRUE;
}

BOOL write_to_slot(PHANDLE slot, char *message) {
	DWORD count = 0;
	LOG("trying to send \"%s\" to slot %x msg_length:%u\r\n", message, *slot, strlen(message));
	if (!WriteFile(*slot, message, sizeof(char)*(strlen(message)+1), &count, NULL)) {
		DWORD err = GetLastError();
		LOG("write_to_slot: slot=%x, msg=%s, err=%u \r\n",*slot,message,err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	
	return TRUE;
}



BOOL open_mailslot_in(PHANDLE slot, LPCSTR slotname) {

	// create sid for BUILTIN\System group
	PSID sid_system = NULL;
	SID_IDENTIFIER_AUTHORITY sid_authsystem = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&sid_authsystem, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &sid_system)) {
		DWORD err = GetLastError();
		LOG("failed to alloc and init sid for system group. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}

	// create EXPLICIT_ACCESS structure for an ACE
	EXPLICIT_ACCESS ea[1];
	ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));

	// fill with ACE for system group
	ea[0].grfAccessPermissions = GENERIC_ALL;	// access rights for this entity
	ea[0].grfAccessMode = SET_ACCESS;			// what this entity shall do: set rights, remove them, ...
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)sid_system;

	// create ACL to contain ACE.
	PACL ms_acl = NULL;
	DWORD seia_res = SetEntriesInAcl(1, ea, NULL, &ms_acl);
	if (seia_res != ERROR_SUCCESS) {
		DWORD err = seia_res;
		LOG("failed to create mailslot ACL. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		FreeSid(sid_system);
		return FALSE;
	}

	// initialize security descriptor
	PSECURITY_DESCRIPTOR ms_desc = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (ms_desc == NULL) {
		//free(group_sid);
		FreeSid(sid_system);
		return FALSE;
	}

	if (!InitializeSecurityDescriptor(ms_desc, SECURITY_DESCRIPTOR_REVISION)) {
		DWORD err = GetLastError();
		LOG("failed to init the mailslot security descriptor. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		//free(group_sid);
		FreeSid(sid_system);
		free(ms_desc);
		return FALSE;
	}

	// add ACL to security descriptor
	if (!SetSecurityDescriptorDacl(ms_desc, TRUE, ms_acl, FALSE)) {
		DWORD err = GetLastError();
		LOG("failed to set mailslot acl. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		FreeSid(sid_system);
		free(ms_desc);
		return FALSE;
	}

	// init security attributes
	SECURITY_ATTRIBUTES ms_sa;
	ms_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	ms_sa.lpSecurityDescriptor = ms_desc;
	ms_sa.bInheritHandle = FALSE;



	// create mailslot for incoming messages	
	*slot = CreateMailslotA(slotname, 0, MAILSLOT_WAIT_FOREVER, &ms_sa);
	if (*slot == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		LOG("connection to incoming mailslot \"%s\" failed. errorcode=%u\r\n", slotname, err);
		add_logging_syserror(log, err);
		FreeSid(sid_system);
		free(ms_desc);
		return FALSE;
	}
	else {
		LOG("connection to incoming mailslot \"%s\" established.\r\n", slotname);
		FreeSid(sid_system);
		free(ms_desc);
		return TRUE;
	}
}

BOOL open_mailslot_out(PHANDLE slot, LPCSTR slotname) {
	*slot = CreateFileA(slotname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);	
	if (*slot == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		LOG("connection to outgoing mailslot \"%s\" failed. errorcode=%u\r\n", slotname,err);
		add_logging_syserror(log, err);
		return FALSE;
	} else {		
		LOG("connection to outgoing mailslot \"%s\" established.\r\n", slotname);
		return TRUE;
	}
}




BOOL read_and_log_object(HANDLE h) {

	


	SE_OBJECT_TYPE type = SE_WINDOW_OBJECT;
	SECURITY_INFORMATION info = ATTRIBUTE_SECURITY_INFORMATION | BACKUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | SCOPE_SECURITY_INFORMATION;
	PSID owner = NULL;
	PSID group = NULL;
	PACL dacl = NULL;
	PACL sacl = NULL;
	PSECURITY_DESCRIPTOR desc = NULL;
	DWORD res =  GetSecurityInfo(h,type,info,&owner,&group,&dacl,&sacl,&desc);
	if (res != ERROR_SUCCESS) {
		LOG("failed to get security info from handle %x. error: %d \r\n",h,res);
		add_logging_syserror(log, res);
		return FALSE;
	}
	LOG("reading from objecthandle %x:\r\n",h);
	// TODO
	LPSTR owner_str = NULL;
	if (!ConvertSidToStringSidA(owner, &owner_str)) {
		LOG("failed to convert owner sid to string.\r\n");
	}
	else {
		LOG("owner: %s \r\n",owner_str);
		LocalFree(owner_str);
	}
	LPSTR group_str = NULL;
	if (!ConvertSidToStringSidA(group, &group_str)) {
		LOG("failed to convert group sid to stirng.\r\n");
	}
	else {
		LOG("group: %s \r\n",group_str);
		LocalFree(group_str);
	}

	log_acl(dacl);
	log_acl(sacl);

	return TRUE;

}


BOOL read_and_log_user_from_token(HANDLE token) {
	LOG("starting to read user token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenUser, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenUser, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_USER info = (PTOKEN_USER)buffer;
			char *sid = NULL;
			if (ConvertSidToStringSidA(info->User.Sid, &sid)) {
				LOG("User.Sid       = %s\r\n", sid);
				LocalFree(sid);
			}
			else {
				LOG("User.Sid       = %s\r\n", "Convert failed");
			}
			LOG("User.Attributes= %d\r\n", info->User.Attributes);
		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}

BOOL read_and_log_group_from_token(HANDLE token) {
	LOG("starting to read group from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenGroups, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenGroups, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_GROUPS info = (PTOKEN_GROUPS)buffer;
			LOG("got %d groups \r\n", info->GroupCount);
			for (unsigned i = 0; i < info->GroupCount; i++) {
				LOG("group %d:\r\n", i + 1);
				char *sid = NULL;
				if (ConvertSidToStringSidA(info->Groups[i].Sid, &sid))
				{
					LOG("Groups.Sid       = %s\r\n", sid);
					LocalFree(sid);
				}
				else {
					LOG("Groups.Sid       = %s\r\n", "Convert failed");
				}
				LOG("Groups.Attributes= %d\r\n", info->Groups[i].Attributes);
			}
		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}

BOOL read_and_log_privileges_from_token(HANDLE token) {
	LOG("starting to read privileges from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenPrivileges, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		if (GetTokenInformation(token, TokenPrivileges, buffer, buffer_length, &buffer_length)) {
			LOG("successfull got privileges from token %x done.\r\n",token);
			PTOKEN_PRIVILEGES info = (PTOKEN_PRIVILEGES)buffer;
			for (unsigned i = 0; i < info->PrivilegeCount; i++) {	
				DWORD buf_length = 0;
				char *buf = NULL;
				LookupPrivilegeNameA(NULL, &info->Privileges[i].Luid, buf, &buf_length);
				buf = malloc(buf_length);
				if (!LookupPrivilegeNameA(NULL, &info->Privileges[i].Luid, buf, &buf_length)) {
					DWORD err = GetLastError();
					LOG("failed to LookupPrivilegeName for LUID %d %d. errorcode=%u\r\n", info->Privileges[i].Luid.HighPart, info->Privileges[i].Luid.LowPart, err);
					add_logging_syserror(log, err);
				}
				LOG("Privileges.LUID      = %3d %3d is %s with attributes %d \r\n", info->Privileges[i].Luid.LowPart, info->Privileges[i].Luid.HighPart, buf, info->Privileges[i].Attributes);
				free(buf);
			}
		}
		else {
			DWORD err = GetLastError();
			LOG("failed to get privileges from token %x. errorcode=%d\r\n",token, err);
			add_logging_syserror(log, err);
		}
	}
	return TRUE;
}

BOOL read_and_log_owner_from_token(HANDLE token) {
	LOG("starting to read owner from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenOwner, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		if (GetTokenInformation(token, TokenOwner, buffer, buffer_length, &buffer_length)) {
			PTOKEN_OWNER info = (PTOKEN_OWNER)buffer;
			char *sid = NULL;
			char name[256] = { 0 };
			char domain[256] = { 0 };
			DWORD domain_length = 256;
			DWORD name_length = 256;
			SID_NAME_USE name_use;
			if (ConvertSidToStringSidA(info->Owner, &sid)) {		
				//LOG("name:%s(%d) domain:%s(%d) name_use:- \r\n", name, name_length, domain, domain_length);
				if (!LookupAccountSidA(NULL,info->Owner,name,&name_length,domain,&domain_length,&name_use)) {
					DWORD err = GetLastError();
					LOG("failed to look up account name from sid %s. name_length:%d. domain_length:%d. err=%d.\r\n", sid, name_length, domain_length, err);
					add_logging_syserror(log, err);
				}		
				//LOG("name:%s(%d) domain:%s(%d) name_use:%d \r\n", name, name_length, domain, domain_length, name_use);
			}
			else {
				free(buffer);
				sid = NULL;
			}
			LOG("Owner       = %s (%s\\%s)\r\n", sid, domain, name);
			LocalFree(sid);
		}
	}
	return TRUE;
}

BOOL read_and_log_primary_group_from_token(HANDLE token) {
	LOG("starting to read primary group from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenPrimaryGroup, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenPrimaryGroup, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_PRIMARY_GROUP info = (PTOKEN_PRIMARY_GROUP)buffer;
			char *sid = NULL;
			if (ConvertSidToStringSidA(info->PrimaryGroup, &sid)) {
				LOG("PrimaryGroup       = %s\r\n", sid);
				LocalFree(sid);
			}
			else {
				LOG("PrimaryGroup       = %s\r\n", "Convert failed");
			}
		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
	}
	return TRUE;
}

BOOL read_and_log_default_acl_from_token(HANDLE token) {
	LOG("starting to read default acl from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenDefaultDacl, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenDefaultDacl, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_DEFAULT_DACL info = (PTOKEN_DEFAULT_DACL)buffer;
			log_acl(info->DefaultDacl);
		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
	}
	return TRUE;
}

BOOL read_and_log_token_source_from_token(HANDLE token) {
	LOG("starting to read token source from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenSource, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenSource, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_SOURCE info = (PTOKEN_SOURCE)buffer;

			LOG("SourceName           =%s\r\n", info->SourceName);
			LOG("SourceIdentfier.LUID = %d (low)| %d (high)\r\n", info->SourceIdentifier.LowPart, info->SourceIdentifier.HighPart);

		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
	}
	return TRUE;
}

BOOL read_and_log_token_type_from_token(HANDLE token) {
	LOG("starting to read token type from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenType, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenType, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_TYPE info = (PTOKEN_TYPE)buffer;
			if (*info == TokenPrimary) {
				LOG("TokenType= TokenPrimary\r\n");
			}
			else if (*info == TokenImpersonation) {
				LOG("TokenType= Impersonation\r\n");
			}
			else {
				LOG("TokenType= unknown\r\n");
			}

		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
	}
	return TRUE;
}

BOOL read_and_log_token_impersonation_level_from_token(HANDLE token) {
	LOG("starting to read token impersonation level from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenImpersonationLevel, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenImpersonationLevel, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PSECURITY_IMPERSONATION_LEVEL info = (PSECURITY_IMPERSONATION_LEVEL)buffer;
			if (*info == SecurityAnonymous) {
				LOG("TokenImpersonationLevel= TokenPrimary\r\n");
			}
			else if (*info == SecurityIdentification) {
				LOG("TokenImpersonationLevel= SecurityIdentification\r\n");
			}
			else if (*info == SecurityImpersonation) {
				LOG("TokenImpersonationLevel= SecurityImpersonation\r\n");
			}
			else if (*info == SecurityDelegation) {
				LOG("TokenImpersonationLevel= SecurityDelegation\r\n");
			}
			else {
				LOG("TokenImpersonationLevel= unknown\r\n");
			}

		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
	}
	return TRUE;
}

BOOL read_and_log_token_statistics_from_token(HANDLE token) {
	LOG("starting to read token statistic from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenStatistics, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenStatistics, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_STATISTICS info = (PTOKEN_STATISTICS)buffer;

			LOG("TokenId         = %d (low)| %d (high)\r\n", info->TokenId.LowPart, info->TokenId.HighPart);
			LOG("AuthenticationId= %d (low)| %d (high)\r\n", info->AuthenticationId.LowPart, info->AuthenticationId.HighPart);
			LOG("ExpirationTime  = %ld \r\n", info->ExpirationTime);
			LOG("tokentype & security impersonation level (shown before)\r\nleft our dynamic charged and dnymic availible\r\n");
			LOG("GroupCount      = %d \r\n", info->GroupCount);
			LOG("PrivilegeCount  = %d \r\n", info->PrivilegeCount);
			LOG("ModifiedId      = %d (low)| %d (high)\r\n", info->ModifiedId.LowPart, info->ModifiedId.HighPart);


		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
	}
	return TRUE;
}

BOOL read_and_log_token_restricted_token_from_token(HANDLE token) {
	LOG("starting to read restricted tokens from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize	
	if (!GetTokenInformation(token, TokenRestrictedSids, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenRestrictedSids, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_GROUPS info = (PTOKEN_GROUPS)buffer;
			LOG("got %d restricted tokens\r\n", info->GroupCount);

			for (unsigned i = 0; i < info->GroupCount; i++) {
				char *sid = NULL;

				if (ConvertSidToStringSidA(info->Groups[i].Sid, &sid)) {
					LOG("RestrictedGroupSid       = %s\r\n", sid);
					LOG("Attributes               = %d\r\n", info->Groups[i].Attributes);
					LocalFree(sid);
				}
				else {
					LOG("RestrictedGroupSid       = %s\r\n", "Convert failed");
				}
			}
		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
	}
	return TRUE;
}

BOOL read_and_log_session_id_from_token(HANDLE token) {
	LOG("starting to read session id from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenSessionId, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information

		if (GetTokenInformation(token, TokenSessionId, buffer, buffer_length, &buffer_length)) {
			DWORD info = (DWORD)buffer;
			LOG("SessionID= %u\r\n", info);

		}
		else {
			LOG("failed to get session id. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	else {
		LOG("GetTokenInformation for sessionid failed!\r\n");
	}
	return TRUE;
}

BOOL read_and_log_groups_and_privileges_from_token(HANDLE token) {
	LOG("starting to read groups and privileges from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenGroupsAndPrivileges, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		if (GetTokenInformation(token, TokenGroupsAndPrivileges, buffer, buffer_length, &buffer_length)) {
			PTOKEN_GROUPS_AND_PRIVILEGES info = (PTOKEN_GROUPS_AND_PRIVILEGES)buffer;

			LOG("SidCount            = %d (SidLength= %d)\r\n", info->SidCount, info->SidLength);
			for (unsigned i = 0; i < info->SidCount; i++) {
				char *sid = NULL;
				if (ConvertSidToStringSidA(info->Sids[i].Sid, &sid)) {
					LOG("Sids[%d].Sid            = %s\r\n", i,sid);
					LOG("Sids[%d].Attributes     = %d\r\n", i,info->Sids[i].Attributes);
					LocalFree(sid);
				}
				else {
					LOG("Sids[%d].Sid            = %s\r\n", i, "Convert failed");
				}
			}
			info->Sids[0].Sid;

			LOG("RestrictedSidCount   = %d (RestrictedSidLength= %d)\r\n", info->RestrictedSidCount, info->RestrictedSidLength);
			LOG("PrivilegeCount       = %d (PrivilegeLength= %d)\r\n", info->PrivilegeCount, info->PrivilegeLength);
			LOG("Privileges.LUID      = %d (low)| %d (high)\r\n", info->Privileges->Luid.LowPart, info->Privileges->Luid.HighPart);
			LOG("Privileges.Attributes= %d \r\n", info->Privileges->Attributes);
			LOG("AuthenticationId     = %d (low)| %d (high)\r\n", info->AuthenticationId.LowPart, info->AuthenticationId.HighPart);
		}
		else {
			DWORD err = GetLastError();
			LOG("failed to get token information. errorcode=%d\r\n", err);
			add_logging_syserror(log, err);
		}
		free(buffer);
	}
	return TRUE;
}

// ----------------------

BOOL read_and_log_token_sandbox_inert_from_token(HANDLE token) {
	LOG("starting to read token sandbox inert from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenSandBoxInert, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenSandBoxInert, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PDWORD info = (PDWORD)buffer;

			LOG("TokenSandBoxInert= %d \r\n", *info);
		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}

BOOL read_and_log_token_origin_from_token(HANDLE token) {
	LOG("starting to read token origin from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenOrigin, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenOrigin, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_ORIGIN info = (PTOKEN_ORIGIN)buffer;
			LOG("OriginatingLogonSession= %d (low)| %d (high)\r\n", info->OriginatingLogonSession.LowPart, info->OriginatingLogonSession.HighPart);
		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}

BOOL read_and_log_token_elevation_type_from_token(HANDLE token) {
	LOG("starting to read token elevation type from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenElevationType, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenElevationType, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_ELEVATION_TYPE info = (PTOKEN_ELEVATION_TYPE)buffer;
			if (*info == TokenElevationTypeDefault) {
				LOG("TokenElevationType= TokenElevationTypeDefault\r\n");
			}
			else if (*info == TokenElevationTypeFull) {
				LOG("TokenElevationType= TokenElevationTypeFull\r\n");
			}
			else if (*info == TokenElevationTypeLimited) {
				LOG("TokenElevationType= TokenElevationTypeLimited\r\n");
			}
			else {
				LOG("TokenElevationType= unknown\r\n");
			}

		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}

BOOL read_and_log_token_linked_token_from_token(HANDLE token) {
	LOG("starting to read token linken token from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenLinkedToken, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenLinkedToken, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_LINKED_TOKEN info = (PTOKEN_LINKED_TOKEN)buffer;
			LOG("TokenLinkedToken= %x\r\n", info->LinkedToken);

		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}

BOOL read_and_log_token_elevation_from_token(HANDLE token) {
	LOG("starting to read token elevation from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenElevation, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenElevation, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PTOKEN_ELEVATION info = (PTOKEN_ELEVATION)buffer;
			LOG("TokenElevation= %d\r\n", info->TokenIsElevated);

		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}

BOOL read_and_log_token_has_restrictions_from_token(HANDLE token) {
	LOG("starting to read token has restrictions from token %x\r\n", token);
	DWORD buffer_length = 0;
	LPVOID buffer = NULL;
	// get needed buffersize
	if (!GetTokenInformation(token, TokenHasRestrictions, buffer, buffer_length, &buffer_length)) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			LOG("couldn't get buffersize for token information errorcode=%d\r\n", err);
			return FALSE;
		}
		buffer = malloc(buffer_length);
		if (buffer == NULL) {
			return FALSE;
		}
		ZeroMemory(buffer, buffer_length);
		// get token information
		LOG("trying to get token information ...");
		if (GetTokenInformation(token, TokenHasRestrictions, buffer, buffer_length, &buffer_length)) {
			LOG(" done.\r\n");
			PDWORD info = (PDWORD)buffer;
			LOG("TokenHasRestrictions= %d\r\n", *info);

		}
		else {
			LOG(" failed. errorcode=%d\r\n", GetLastError());
		}
		free(buffer);
	}
	return TRUE;
}










// TODO continue wiht list on https://msdn.microsoft.com/en-us/library/windows/desktop/aa379626(v=vs.85).aspx

BOOL log_token(HANDLE token) {
	read_and_log_session_id_from_token(token);
	read_and_log_token_statistics_from_token(token);
	// all
#if 0
	read_and_log_default_acl_from_token(token);
	read_and_log_groups_and_privileges_from_token(token);
	read_and_log_group_from_token(token);
	read_and_log_owner_from_token(token);
	read_and_log_primary_group_from_token(token);
	read_and_log_privileges_from_token(token);
	read_and_log_session_id_from_token(token);
	read_and_log_token_impersonation_level_from_token(token);
	read_and_log_token_restricted_token_from_token(token);
	read_and_log_token_source_from_token(token);
	read_and_log_token_statistics_from_token(token);
	read_and_log_token_type_from_token(token);
	read_and_log_user_from_token(token);
	read_and_log_token_sandbox_inert_from_token(token);
	read_and_log_token_origin_from_token(token);
	read_and_log_token_elevation_type_from_token(token);
	read_and_log_token_linked_token_from_token(token);
	read_and_log_token_elevation_from_token(token);
	read_and_log_token_has_restrictions_from_token(token);
#endif
	return TRUE;
}



BOOL log_acl(PACL acl) {
	// read the DACL
	EXPLICIT_ACCESS *dd_ea;
	LONG dd_ea_entries;
	//LOG("trying to read the DACL ... ");
	DWORD result;
	result = GetExplicitEntriesFromAclA(acl, &dd_ea_entries, &dd_ea);
	if (ERROR_SUCCESS != result) {
		LOG("failed to read acl. Error: %u \r\n", result);
		add_logging_syserror(log, result);
		//goto clean;
		return FALSE;
	}
	LOG("got %d entries in DACL:\r\n", dd_ea_entries);
	for (int i = 0; i < dd_ea_entries; i++) {
		LOG("Entry Number %2d -------------------------------------------------\r\n", i);
		LOG("\tACCESS_MODE       : %d\r\n", dd_ea[i].grfAccessMode);
		LOG("\tAccess Permissions: %ld \r\n", dd_ea[i].grfAccessPermissions); // dword %ld
		char ap[1024];
		get_permissions_as_string(dd_ea[i].grfAccessPermissions, ap, 1024);
		LOG("\t\t%s\r\n", ap);
		LOG("\tInheritance       : %ld \r\n", dd_ea[i].grfInheritance); //dword
		LOG("\tTrustee\r\n");
		LOG("\t\tMultipleTrusteeOp: %d \r\n", dd_ea[i].Trustee.MultipleTrusteeOperation);// not supported atm, must be NO_MULTIPLE_TRUSTEE
		LOG("\t\tMultipleTrustee  : %X \r\n", dd_ea[i].Trustee.pMultipleTrustee); // not supported atm, must be NULL
		LOG("\t\tTrusteeForm      : %d \r\n", dd_ea[i].Trustee.TrusteeForm); // enum for type of data
		LOG("\t\tTrusteeType      : %d \r\n", dd_ea[i].Trustee.TrusteeType); // enum for the type. is it user, group,..
		LOG("\t\tptstrName        : %X \r\n", dd_ea[i].Trustee.ptstrName); // pointer to buffer that identifies trustee
		switch (dd_ea[i].Trustee.TrusteeForm) {
		case TRUSTEE_IS_NAME:
			LOG("\t\t\tTRUSTEE_IS_NAME\r\n");
			LOG("\t\t\t\tname: %s \r\n", dd_ea[i].Trustee.ptstrName);
			break;
		case TRUSTEE_IS_OBJECTS_AND_NAME:
			LOG("\t\t\tRUSTEE_IS_OBJECTS_AND_NAMEr\n");
			OBJECTS_AND_NAME_ *oan = (OBJECTS_AND_NAME_*)dd_ea[i].Trustee.ptstrName;
			LOG("\t\t\t\tObjectsPresent: %ld \r\n", oan->ObjectsPresent);
			LOG("\t\t\t\tObjectType    : %d \r\n", oan->ObjectType);
			LOG("\t\t\t\tObjectTypeName: %s \r\n", oan->ObjectTypeName);
			LOG("\t\t\t\tInhObjTyName  : %s \r\n", oan->InheritedObjectTypeName);
			LOG("\t\t\t\tptstrName     : %s \r\n", oan->ptstrName);
			if ((oan->ObjectsPresent & ACE_OBJECT_TYPE_PRESENT) == ACE_INHERITED_OBJECT_TYPE_PRESENT) {
				// got the ace_inherited flag set
				//LOG("\t\t\t\tInhObjTyName  : %s \r\n");
			}
			if ((oan->ObjectsPresent & ACE_INHERITED_OBJECT_TYPE_PRESENT) == ACE_OBJECT_TYPE_PRESENT) {
				// got the ace object type flag set
			}
			break;
		case TRUSTEE_IS_OBJECTS_AND_SID:
			LOG("\t\t\tTRUSTEE_IS_OBJECTS_AND_SID\r\n");
			OBJECTS_AND_SID *oas = (OBJECTS_AND_SID*)dd_ea[i].Trustee.ptstrName;
			LOG("\t\t\t\tObjectsPresent: %ld \r\n", oas->ObjectsPresent);
			LOG("\t\t\t\tTObjectTypeGuid: %X \r\n", oas->ObjectTypeGuid);
			// TODO look into guid struct
			LOG("\t\t\t\tInhObjTypeGuid : %X \r\n", oas->InheritedObjectTypeGuid);
			// TODO look into guid struct
			char *sid = NULL;
			if (ConvertSidToStringSidA(dd_ea[i].Trustee.ptstrName, &sid)) {
				LOG("\t\t\t\tSID: %s \r\n", sid);
				LocalFree(sid);
			}
			break;
		case TRUSTEE_IS_SID:
			LOG("\t\t\tTRUSTEE_IS_SID\r\n");
			char *sid2 = NULL;
			if (ConvertSidToStringSidA(dd_ea[i].Trustee.ptstrName, &sid2)) {
				LOG("\t\t\t\tSID      : %s \r\n", sid2);
				LocalFree(sid2);
			}
			else {
				LOG("\t\t\t\tSID      : converting failed.\r\n");
			}
			char psid_name[256];
			DWORD psid_name_length = 255;
			char psid_domain[256];
			DWORD psid_domain_length = 255;
			SID_NAME_USE psid_type;
			if (LookupAccountSid(NULL, dd_ea[i].Trustee.ptstrName, psid_name, &psid_name_length, psid_domain, &psid_domain_length, &psid_type)) {
				LOG("\t\t\t\tSIDname  : %s\r\n", psid_name);
				LOG("\t\t\t\tSIDDomain: %s\r\n", psid_domain);
				LOG("\t\t\t\tSIDType  : %u\r\n", psid_type);
			}
			else {
				DWORD err = GetLastError();
				LOG("\t\t\t\tSIDname  : failed. error: %u \r\n", err);
				LOG("\t\t\t\tSIDDomain: failed. error: %u \r\n", err);
				LOG("\t\t\t\tSIDType  : failed. error: %u \r\n", err);
				//add_logging_syserror(log, err);
			}
			break;
		}
		LOG("-----------------------------------------------------------------\r\n");
	}
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
	// window station access rights
	// TODO
	// token specific access rights
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_ADJUST_DEFAULT, "TOKEN_ADJUST_DEFAULT ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_ADJUST_GROUPS, "TOKEN_ADJUST_GROUPS ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_ADJUST_PRIVILEGES, "TOKEN_ADJUST_PRIVILEGES ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_ADJUST_SESSIONID, "TOKEN_ADJUST_SESSIONID ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_ASSIGN_PRIMARY, "TOKEN_ASSIGN_PRIMARY ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_DUPLICATE, "TOKEN_DUPLICATE ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_EXECUTE, "TOKEN_EXECUTE ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_QUERY, "TOKEN_QUERY ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_QUERY_SOURCE, "TOKEN_QUERY_SOURCE ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_READ, "TOKEN_READ ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_WRITE, "TOKEN_WRITE ");
	GET_STRING_FROM_RIGHTS(buffer, count, permissions, TOKEN_ALL_ACCESS, "TOKEN_ALL_ACCESS ");

		
		
	StringCbCatA(&buffer[count] - 1, sizeof('\0\0'), "\0\0");
	return TRUE;
}





