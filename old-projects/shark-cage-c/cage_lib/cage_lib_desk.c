///
/// \file cage_lib_desk.c
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage library - desktop module
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
#include <time.h>
#include <string.h>

#include <winternl.h>



#include "cage_lib.h"



// enum window stations
BOOL log_window_stations_and_desktops(void) {
	char **window_station_list = alloc_list(MAX_STATION_ENTRIES, MAX_STATION_NAME);
	char **desktop_list = alloc_list(MAX_DESKTOP_ENTRIES, MAX_STATION_NAME);
	EnumWindowStations(enum_station_callback, (LPARAM)window_station_list);

	LOG("Got Window Station List:\r\n");
	for (int i = 0; i < MAX_STATION_ENTRIES; i++){
		if (window_station_list[i][0] == '\0') {
			break;
		}
		LOG("\tfound station %s\r\n", window_station_list[i]);
	}
	// get desktop list
	EnumDesktops(OpenWindowStationA("WinSta0", FALSE, WINSTA_ALL_ACCESS), enum_desktop_callback, (LPARAM)desktop_list);
	LOG("Got Desktop names for window station %s\r\n", window_station_list[0]);
	for (int i = 0; i < MAX_DESKTOP_ENTRIES; i++) {
		if (desktop_list[i][0] == '\0') {
			break;
		}
		LOG("\tfound desktop %s on station %s \r\n", desktop_list[i], window_station_list[0]);
	}
	free_list(MAX_STATION_ENTRIES, window_station_list);
	free_list(MAX_DESKTOP_ENTRIES, desktop_list);
	return TRUE;
}

BOOL delete_group(LPSTR group_name) {
	WCHAR *group_name_uni2 = malloc(sizeof(WCHAR)* 256);
	int group_name_length = strlen(group_name) + 1;
	int group_name_uni_length = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, group_name, -1, group_name_uni2, 0);
	int ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, group_name, -1, group_name_uni2, group_name_uni_length);
	if (ret == 0) {
		DWORD err = GetLastError();
		LOG("failed to convert group name. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	group_name_uni2[group_name_uni_length] = L'\0';
	delete_group_w(group_name_uni2);
	free(group_name_uni2);
	return TRUE;
}

BOOL delete_group_w(WCHAR * group_name) {
	DWORD group_return = NetLocalGroupDel(NULL, group_name);
	if (group_return != NERR_Success) {
		LOG("deleting the group %ws failed. return=%u \r\n", group_name, group_return);
		add_logging_syserror(log, group_return);
		return FALSE;
	}
	return TRUE;
}

BOOL delete_all_groups(WCHAR *prefix) {
	PLOCALGROUP_INFO_0 groups;
	DWORD entries_read = 0;
	DWORD entries_exist = 0;
	if (NERR_Success != NetLocalGroupEnum(NULL, 0, (LPBYTE*)&groups, MAX_PREFERRED_LENGTH, &entries_read, &entries_exist, NULL)) {
		LOG("failed to enumerate local groups.\r\n");
		return FALSE;
	}
	LOG("we got %d groups from %d groups.\r\n", entries_read, entries_exist);
	for (unsigned i = 0; i < entries_read; i++) {
		if (0 == wcsncmp(groups[i].lgrpi0_name, prefix, wcslen(prefix))) {
			LOG("removing group %S. \r\n", groups[i].lgrpi0_name);
			delete_group_w(groups[i].lgrpi0_name);
		}
	}
	NetApiBufferFree(groups);
	return TRUE;
}

BOOL create_caged_token(PSID group_sid, PHANDLE token_out) {
	DWORD sess_id;
	if (!get_active_session(&sess_id)) {
		LOG("failed to get active session.\r\n");
	}
	// get user token to copy most out of it
	HANDLE user_token = NULL;
	if (!WTSQueryUserToken(sess_id, &user_token)) {
		DWORD err = GetLastError();
		LOG("failed to open user token.\r\n");
		add_logging_syserror(log, err);
		return FALSE;
	}
	else {
		LOG("got user token %x.\r\n",user_token);
	}



	// get privileged token to impersonate to	
	HANDLE priv_token = NULL;
	LUID luid;
	if (!LookupPrivilegeValue(NULL, SE_CREATE_TOKEN_NAME, &luid)) {
		DWORD err = GetLastError();
		LOG("failed to LookupPrivilegeValue for %s. errorcode=%d\r\n", SE_CREATE_TOKEN_NAME, err);
		add_logging_syserror(log, err);
	}
	if (!get_token_with_privileg(luid, &priv_token)) {
		LOG("failed to get privileged token.\r\n");
	}
	//LOG("got privileged token %x.\r\n",priv_token);
	if (!ImpersonateLoggedOnUser(priv_token)) {
		DWORD err = GetLastError();
		LOG("failed to impersonate privileged token. errorcode=%d\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	CloseHandle(priv_token);
	



#if 0
	HANDLE current_token = NULL;
	HANDLE cage_token = NULL;
	if (!WTSQueryUserToken(sess_id, &current_token)) {
		DWORD err = GetLastError();
		LOG("failed to open user token.\r\n");
		add_logging_syserror(log, err);
		if (!RevertToSelf()) {
			DWORD err2 = GetLastError();
			LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
			add_logging_syserror(log, err2);
		}
		return FALSE;
	}
#endif
	HANDLE current_token = NULL;
	HANDLE cage_token = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_ALL_ACCESS, &current_token)) {
		DWORD err = GetLastError();
		LOG("failed to open process token.\r\n");
		add_logging_syserror(log, err);
		if (!RevertToSelf()) {
			DWORD err2 = GetLastError();
			LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
			add_logging_syserror(log, err2);
		}
		return FALSE;
	}

	else {
		LOG("successfull opened process token. current_token=%x\r\n", current_token);	
		CloseHandle(current_token);
		current_token = user_token;

		// OBJECT_ATTRIBUTES
		SECURITY_QUALITY_OF_SERVICE ct_sqos;
		ZeroMemory(&ct_sqos, sizeof(SECURITY_QUALITY_OF_SERVICE));
		ct_sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
		ct_sqos.ImpersonationLevel = SecurityImpersonation;
		ct_sqos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
		ct_sqos.EffectiveOnly = FALSE;
		OBJECT_ATTRIBUTES	ct_oa;
		ZeroMemory(&ct_oa, sizeof(OBJECT_ATTRIBUTES));
		ct_oa.Length = sizeof(OBJECT_ATTRIBUTES);
		ct_oa.SecurityQualityOfService = &ct_sqos;
		// TOKEN_TYPE
		TOKEN_TYPE ct_type = TokenPrimary;
		// PLUID -> AuthenticationId
		// PLARGE_INTEGER -> ExpirationTime
		DWORD ct_buf_length = 0;
		GetTokenInformation(current_token, TokenStatistics, NULL, 0, &ct_buf_length);
		PTOKEN_STATISTICS ct_stats = malloc(ct_buf_length);
		if (ct_stats == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenStatistics, ct_stats, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(stats). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		LUID ct_auth = ct_stats->AuthenticationId;
		LARGE_INTEGER ct_exp = ct_stats->ExpirationTime;
		// PTOKEN_USER
		GetTokenInformation(current_token, TokenUser, NULL, 0, &ct_buf_length);
		PTOKEN_USER ct_user = malloc(ct_buf_length);
		if (ct_user == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenUser, ct_user, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(user). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		// PTOKEN_GROUPS
		GetTokenInformation(current_token, TokenGroups, NULL, 0, &ct_buf_length);
		PTOKEN_GROUPS ct_groups = malloc(ct_buf_length);
		if (ct_groups == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenGroups, ct_groups, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(groups). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		// add Cage Group
		DWORD ct_group_new_length = ct_buf_length + sizeof(SID_AND_ATTRIBUTES)+sizeof(PSID_AND_ATTRIBUTES); // ct_group_length;//
		PTOKEN_GROUPS ct_groups_new = malloc(ct_group_new_length);
		if (ct_groups_new == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		ZeroMemory(ct_groups_new, ct_group_new_length);
		for (unsigned i = 0; i < ct_groups->GroupCount; i++) {
			ct_groups_new->Groups[i].Attributes = ct_groups->Groups[i].Attributes;
			ct_groups_new->Groups[i].Sid = ct_groups->Groups[i].Sid;
		}
		//	ct_group_new->GroupCount = ct_group->GroupCount;
		ct_groups_new->GroupCount = ct_groups->GroupCount + 1;
		ct_groups_new->Groups[ct_groups->GroupCount].Attributes = 7;			// from securedesktop why 7 ?
		ct_groups_new->Groups[ct_groups->GroupCount].Sid = group_sid;

		// PTOKEN_PRIVILEGES
		GetTokenInformation(current_token, TokenPrivileges, NULL, 0, &ct_buf_length);
		PTOKEN_PRIVILEGES ct_priv = malloc(ct_buf_length);
		if (ct_priv == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenPrivileges, ct_priv, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(privileges). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		// PTOKEN_OWNER 
		GetTokenInformation(current_token, TokenOwner, NULL, 0, &ct_buf_length);
		PTOKEN_OWNER ct_owner = malloc(ct_buf_length);
		if (ct_owner == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenOwner, ct_owner, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(owner). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		// PTOKEN_PRIMARY_GROUP
		GetTokenInformation(current_token, TokenPrimaryGroup, NULL, 0, &ct_buf_length);
		PTOKEN_PRIMARY_GROUP ct_pgroup = malloc(ct_buf_length);
		if (ct_pgroup == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenPrimaryGroup, ct_pgroup, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(primgroup). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		// PTOKEN_DEFAULT_DACL
		GetTokenInformation(current_token, TokenDefaultDacl, NULL, 0, &ct_buf_length);
		PTOKEN_DEFAULT_DACL ct_ddacl = malloc(ct_buf_length);
		if (ct_ddacl == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenDefaultDacl, ct_ddacl, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(ddacl). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		// PTOKEN_SOURCE
		GetTokenInformation(current_token, TokenSource, NULL, 0, &ct_buf_length);
		PTOKEN_SOURCE ct_source = malloc(ct_buf_length);
		if (ct_source == NULL) {
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!GetTokenInformation(current_token, TokenSource, ct_source, ct_buf_length, &ct_buf_length)) {
			DWORD err = GetLastError();
			LOG("failed to gettokeninformation(source). errorcode=%u\r\n", err);
			add_logging_syserror(log, err);
			if (!RevertToSelf()) {
				DWORD err2 = GetLastError();
				LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
				add_logging_syserror(log, err2);
			}
			return FALSE;
		}
		if (!get_token_functions()) {
			LOG("failed to get token functions.\r\n");
		}
		UINT status = ZwCreateToken(&cage_token,
			TOKEN_ALL_ACCESS,
			&ct_oa,
			ct_type,
			&ct_auth,
			&ct_exp,
			ct_user,
			ct_groups_new,
			ct_priv,
			ct_owner,
			ct_pgroup,
			ct_ddacl,
			ct_source);

		//UINT s2 = 1;//RtlNtStatusToDosError(status);
		LOG("zwcreatetoken=%u \r\n", status);
		add_logging_syserror(log, status);
		//add_logging_syserror(log, s2);


		read_and_log_session_id_from_token(cage_token);

		// change session to input session NOT necessary if we use a usertoken

		if (!SetTokenInformation(cage_token, TokenSessionId, &sess_id, sizeof(DWORD))) {
			DWORD err = GetLastError();
			LOG("failed to set new session %d in token. errorcode=%d\r\n", sess_id, err);
			add_logging_syserror(log, err);
			CloseHandle(cage_token);			
		}

		*token_out = cage_token;

		CloseHandle(current_token);
		free(ct_stats);
		free(ct_user);
		free(ct_groups);
		free(ct_groups_new);
		free(ct_priv);
		free(ct_owner);
		free(ct_pgroup);
		free(ct_ddacl);
		free(ct_source);
		if (!RevertToSelf()) {
			DWORD err2 = GetLastError();
			LOG("failed to RevertToSelf. errorcode=%d\r\n", err2);
			add_logging_syserror(log, err2);
		}
		if (status == 0) {
			return TRUE;
		}
		return FALSE;
	}
}



BOOL create_cage_desk(PSID group_sid, HDESK *desk_out) {

	if (group_sid == NULL) {
		LOG("group_sid is null\r\n");
		return FALSE;
	}

	// create sid for BUILTIN\System group
	PSID sid_system = NULL;
	SID_IDENTIFIER_AUTHORITY sid_authsystem = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&sid_authsystem, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &sid_system)) {
		DWORD err = GetLastError();
		LOG("failed to alloc and init sid for system group. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		free(group_sid);
		return FALSE;
	}

	// create SID for BUILTIN\Administrators group
	PSID sid_admin = NULL;
	SID_IDENTIFIER_AUTHORITY sid_authnt = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&sid_authnt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid_admin)) {
		DWORD err = GetLastError();
		LOG("failed to alloc and init sid for admin group. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		free(group_sid);
		LocalFree(sid_system);
		return FALSE;
	}

	// create EXPLICIT_ACCESS structure for an ACE
	EXPLICIT_ACCESS ea[3];
	ZeroMemory(&ea, 3 * sizeof(EXPLICIT_ACCESS));

	// fill with ACE for system group
	ea[0].grfAccessPermissions = GENERIC_ALL;	// access rights for this entity
	ea[0].grfAccessMode = SET_ACCESS;			// what this entity shall do: set rights, remove them, ...
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)sid_system;
	// fill EXPLICIT_ACCESS with second ACE for admin group
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR)sid_admin;
	// fill with group
	ea[2].grfAccessPermissions = DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD |
		DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS;
	ea[2].grfAccessMode = SET_ACCESS;
	ea[2].grfInheritance = NO_INHERITANCE;
	ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[2].Trustee.ptstrName = (LPTSTR)group_sid;

	// create ACL to contain ACE.
	PACL desk_acl = NULL;
	DWORD seia_res = SetEntriesInAcl(3, ea, NULL, &desk_acl);
	if (seia_res != ERROR_SUCCESS) {
		DWORD err = seia_res;
		LOG("failed to create ACL. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		//free(group_sid);
		LocalFree(sid_admin);
		LocalFree(sid_system);
		return FALSE;
	}

	// initialize security descriptor
	PSECURITY_DESCRIPTOR desk_security_desc = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (desk_security_desc == NULL) {
		//free(group_sid);
		LocalFree(sid_admin);
		LocalFree(sid_system);
		return FALSE;
	}

	if (!InitializeSecurityDescriptor(desk_security_desc, SECURITY_DESCRIPTOR_REVISION)) {
		DWORD err = GetLastError();
		LOG("failed to init the security descriptor. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		//free(group_sid);
		LocalFree(sid_admin);
		LocalFree(sid_system);
		free(desk_security_desc);
		return FALSE;
	}

	// add ACL to security descriptor
	if (!SetSecurityDescriptorDacl(desk_security_desc, TRUE, desk_acl, FALSE)) {
		DWORD err = GetLastError();
		LOG("failed to set acl. Error: %u \r\n", err);
		add_logging_syserror(log, err);
		//free(group_sid);
		LocalFree(sid_admin);
		LocalFree(sid_system);
		free(desk_security_desc);
		return FALSE;
	}

	// init security attributes
	SECURITY_ATTRIBUTES desk_security_attr;
	desk_security_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	desk_security_attr.lpSecurityDescriptor = desk_security_desc;
	desk_security_attr.bInheritHandle = FALSE;

	// creating mydesk
	char *desktop_name = "mydesk";
	ACCESS_MASK desk_access_mask = DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | DESKTOP_WRITEOBJECTS | READ_CONTROL | WRITE_DAC | WRITE_OWNER;
	HDESK mydesk = CreateDesktopA(desktop_name, NULL, NULL, 0, desk_access_mask, &desk_security_attr);
	if (mydesk == NULL) {
		DWORD err = GetLastError();
		LOG("failed to create desktop %s. Error: %u \r\n", desktop_name, err);
		add_logging_syserror(log, err);
		//free(group_sid);
		LocalFree(sid_admin);
		LocalFree(sid_system);
		free(desk_security_desc);
		return FALSE;
	}

	LOG("successfull created the caged desktop. desk=%x\r\n", mydesk);
	*desk_out = mydesk;
	//free(group_sid);
	LocalFree(sid_admin);
	LocalFree(sid_system);
	free(desk_security_desc);

	return TRUE;
}



BOOL generate_and_add_group_id(char **group_name_out) {
	// create random group id
	char *group_name = malloc(sizeof(char)* 32);
	srand((unsigned)time(NULL));
	sprintf_s(group_name, sizeof(char)* 14, "CAGE_00%.05d\0", rand());

	// convert to widechar / unicode
	WCHAR *group_name_uni2 = malloc(sizeof(WCHAR)* 256);
	int group_name_length = strlen(group_name) + 1;
	int group_name_uni_length = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, group_name, -1, group_name_uni2, 0);
	int ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, group_name, -1, group_name_uni2, group_name_uni_length);
	if (ret == 0) {
		DWORD err = GetLastError();
		LOG("MultiByteToWideChar failed. Error: %u \r\n", err);
		add_logging_syserror(log, err);
	}
	group_name_uni2[group_name_uni_length] = L'\0';

	// create group
	LOCALGROUP_INFO_0 group_info;
	group_info.lgrpi0_name = group_name_uni2;
	DWORD group_error;
	DWORD group_return = NetLocalGroupAdd(NULL, 0, (LPBYTE)&group_info, &group_error);
	if (group_return != NERR_Success) {
		switch (group_return) {
		case ERROR_ACCESS_DENIED:
			LOG("adding local group %s (%ws) failed. with ERROR_ACCESS_DENIED\r\n", group_name, group_name_uni2);
			break;
		case ERROR_ALIAS_EXISTS:
			LOG("adding local group %s (%ws) failed. with ERROR_ALIAS_EXISTS \r\n", group_name, group_name_uni2);
			break;
		case ERROR_INVALID_LEVEL:
			LOG("adding local group %s (%ws) failed. with ERROR_INVALID_LEVEL\r\n", group_name, group_name_uni2);
			break;
		case ERROR_INVALID_PARAMETER:
			LOG("adding local group %s (%ws) failed. with ERROR_INVALID_PARAMETER\r\n", group_name, group_name_uni2);
			break;
		case NERR_GroupExists:
			LOG("adding local group %s (%ws) failed. with NERR_GroupExists \r\n", group_name, group_name_uni2);
			break;
		case NERR_InvalidComputer:
			LOG("adding local group %s (%ws) failed. with NERR_InvalidComputer \r\n", group_name, group_name_uni2);
			break;
		case NERR_NotPrimary:
			LOG("adding local group %s (%ws) failed. with NERR_NotPrimary \r\n", group_name, group_name_uni2);
			break;
		case NERR_UserExists:
			LOG("adding local group %s (%ws) failed. with NERR_UserExists \r\n", group_name, group_name_uni2);
			break;
		default:
			LOG("adding local group %s (%ws) failed. with unknown error\r\n", group_name, group_name_uni2);
		}
		free(group_name);
		free(group_name_uni2);
		return FALSE;
	}
	else {
		LOG("successfull added local group %s (%ws).\r\n", group_name, group_name_uni2);
		if (*group_name_out != NULL) {
			strcpy_s(*group_name_out, strlen(group_name) + 1, group_name);
		}
		free(group_name);
		free(group_name_uni2);
		return TRUE;
	}

}


BOOL clean_desktop() {
	/*
	if (NtQueryInformationProcess == NULL) {
		LOG("getting NtQueryInformationProcess function pointer.\r\n");
		NtQueryInformationProcess = (PVOID)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationProcess");
	}
	if (NtQueryInformationProcess == NULL) {
		DWORD err = GetLastError();
		LOG("failed to get NtQueryInformationProcess function Pointer. errorcode=%u\r\n", err);
		add_logging_syserror(log, err);
		return FALSE;
	}
	*/
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
	for (unsigned i = 0; i < procs_count; i++) {
		

		HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procs[i]);
		if (proc == NULL) {
			continue;
		}
		
		LOG("is process with pid %3d (%x) on desk mydesk (%3d/%3d)\r\n", procs[i], proc, i, procs_count);
		is_process_on_desk(procs[i], "mydesk");

		LOG("after check i=%d.\r\n",i);

		/*
		// http://stackoverflow.com/questions/23144350/how-to-get-window-station-for-a-given-process
		PROCESS_BASIC_INFORMATION proc_info = { 0 };
		DWORD proc_info_length;
		LOG("before ntquery..\r\n");
		// 0->ProcessBasicInformation
		DWORD status = NtQueryInformationProcess(proc,0,&proc_info,sizeof(proc_info),&proc_info_length);
		LOG("NtQueryInformationProcess: %d\r\n",status);
		if (!ReadProcessMemory(proc, proc_info.PebBaseAddress, &proc_info, proc_info_length, NULL)) {
			DWORD err = GetLastError();
			LOG("unable to read process memory from pid %d.\r\n",procs[i]);
			add_logging_syserror(log, err);
			CloseHandle(proc);
			continue;
		}
		//proc_info.PebBaseAddress->ProcessParameters->CommandLine;
		LOG("bla: %s \r\n", proc_info.PebBaseAddress->ProcessParameters->DesktopName);
		*/
		/*
		HMODULE mods[1024];
		ZeroMemory(mods, sizeof(HMODULE)* 1024);
		DWORD mod_length;
		if (!EnumProcessModules(proc, mods, sizeof(mods), &mod_length)) {
			DWORD err = GetLastError();
			LOG("failed to enumerate all process modules for process with pid %d.\r\n", procs[i]);
			add_logging_syserror(log, err);
			continue;
		}
		DWORD mod_count = mod_length / sizeof(HMODULE);
		for (unsigned j = 0; j < mod_count; j++){
			LPSTR mod_name[256] = { '\0' };
			if (!K32GetModuleBaseNameA(proc, mods[j], *mod_name, sizeof(mod_name) / sizeof(LPSTR))) {
				DWORD err = GetLastError();
				LOG("failed to get module base name from pid %d.\r\n", procs[i]);
				add_logging_syserror(log, err);
				continue;
			}
			LOG("pid %d module %d base name: %s. \r\n", procs[i], mods[j], mod_name);
			
			MODULEINFO info;
			if (!GetModuleInformation(proc,mods[j],&info,sizeof(MODULEINFO))) {
			LOG("failed to get module information from pid %d. \r\n", procs[i]);
			continue;
			}
			
		}
		*/


		CloseHandle(proc);
	}
	return TRUE;
}

BOOL is_process_on_desk(DWORD proc_id, LPSTR desk_name) {
	LOG("isprocondesk\r\n");
	// http://stackoverflow.com/questions/23144350/how-to-get-window-station-for-a-given-process
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, proc_id);
	if (hProc)
	{
		BOOL(WINAPI *pfnIsWow64Process)(HANDLE, PBOOL);

		(FARPROC)pfnIsWow64Process = GetProcAddress(GetModuleHandle("kernel32.dll"), "IsWow64Process");

		SYSTEM_INFO si = { 0 };
		GetNativeSystemInfo(&si);

		//See if 32-bit process on 64-bit OS
		BOOL bWow64Proc = TRUE;
		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		{
			if (pfnIsWow64Process)
			if (!pfnIsWow64Process(hProc, &bWow64Proc))
			{
				//Error
				LOG("ERROR in IsWow64Process: %d\n", GetLastError());
			}
		}

		NTSTATUS ntStatus;

		
		if (bWow64Proc)
		{
			//32-bit process
			NTSTATUS(WINAPI *pfnNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
			(FARPROC)pfnNtQueryInformationProcess = GetProcAddress(GetModuleHandle("Ntdll.dll"), "NtQueryInformationProcess");
			if (pfnNtQueryInformationProcess)
			{
				PROCESS_BASIC_INFORMATION pbi = { 0 };
				DWORD dwsz = 0;
				if ((ntStatus = pfnNtQueryInformationProcess(hProc, ProcessBasicInformation, &pbi, sizeof(pbi), &dwsz)) == 0 &&
					dwsz <= sizeof(pbi) &&
					pbi.PebBaseAddress)
				{
					//Define PEB structs
					typedef struct _RTL_DRIVE_LETTER_CURDIR
					{
						WORD Flags;
						WORD Length;
						ULONG TimeStamp;
						STRING DosPath;
					} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

					typedef struct _RTL_USER_PROCESS_PARAMETERS_32
					{
						ULONG                   MaximumLength;
						ULONG                   Length;
						ULONG                   Flags;
						ULONG                   DebugFlags;
						PVOID                   ConsoleHandle;
						ULONG                   ConsoleFlags;
						HANDLE                  StdInputHandle;
						HANDLE                  StdOutputHandle;
						HANDLE                  StdErrorHandle;
						UNICODE_STRING          CurrentDirectoryPath;
						HANDLE                  CurrentDirectoryHandle;
						UNICODE_STRING          DllPath;
						UNICODE_STRING          ImagePathName;
						UNICODE_STRING          CommandLine;
						PVOID                   Environment;
						ULONG                   StartingPositionLeft;
						ULONG                   StartingPositionTop;
						ULONG                   Width;
						ULONG                   Height;
						ULONG                   CharWidth;
						ULONG                   CharHeight;
						ULONG                   ConsoleTextAttributes;
						ULONG                   WindowFlags;
						ULONG                   ShowWindowFlags;
						UNICODE_STRING          WindowTitle;
						UNICODE_STRING          DesktopName;
						UNICODE_STRING          ShellInfo;
						UNICODE_STRING          RuntimeData;
						RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
					}RTL_USER_PROCESS_PARAMETERS_32, *PRTL_USER_PROCESS_PARAMETERS_32;

					typedef struct _PEB_32
					{
						BYTE                          Reserved1[2];
						BYTE                          BeingDebugged;
						BYTE                          Reserved2[1];
						PVOID                         Reserved3[2];
						void*                         Ldr;
						RTL_USER_PROCESS_PARAMETERS_32* ProcessParameters;
						BYTE                          Reserved4[104];
						PVOID                         Reserved5[52];
						void*                         PostProcessInitRoutine;
						BYTE                          Reserved6[128];
						PVOID                         Reserved7[1];
						ULONG                         SessionId;
					}PEB_32, *PPEB_32;

					//Read PEB-32
					PEB_32 peb32 = { 0 };

					DWORD dwcbSzRead = 0;
					if (ReadProcessMemory(hProc, pbi.PebBaseAddress, &peb32, sizeof(peb32), &dwcbSzRead) &&
						dwcbSzRead == sizeof(peb32) &&
						peb32.ProcessParameters)
					{
						//Read RTL_USER_PROCESS_PARAMETERS_32
						RTL_USER_PROCESS_PARAMETERS_32 rupp32 = { 0 };

						dwcbSzRead = 0;
						if (ReadProcessMemory(hProc, peb32.ProcessParameters, &rupp32, sizeof(rupp32), &dwcbSzRead) &&
							dwcbSzRead == sizeof(rupp32) &&
							rupp32.DesktopName.Buffer)
						{
							//Get desktop name
							int ncbSzLn = rupp32.DesktopName.Length + sizeof(TCHAR);
							BYTE* pDesktopName = malloc(sizeof(BYTE)*ncbSzLn);
							if (pDesktopName)
							{
								dwcbSzRead = 0;
								if (ReadProcessMemory(hProc, rupp32.DesktopName.Buffer, pDesktopName, ncbSzLn, &dwcbSzRead) &&
									dwcbSzRead == ncbSzLn)
								{
									//Set last NULL
									*(TCHAR*)(pDesktopName + ncbSzLn - sizeof(TCHAR)) = 0;

									//We're done
									LOG("Desktop32: %s\n", (LPCTSTR)pDesktopName);
								}
								else
									LOG("ERROR in ReadProcessMemory DesktopName: %d\n", GetLastError());

								free( pDesktopName);
							}
							else
								LOG("ERROR DesktopName ptr\n");
						}
						else
							LOG("ERROR in ReadProcessMemory RTL_USER_PROCESS_PARAMETERS_32: %d\n", GetLastError());
					}
					else
						LOG("ERROR in ReadProcessMemory PEB-32: %d\n", GetLastError());
				}
				else
					LOG("ERROR in NtQueryInformationProcess: %d\n", ntStatus);
			}
			else
				LOG("ERROR NtQueryInformationProcess API\n");
		}
		else
		{
			//64-bit process
			NTSTATUS(WINAPI *pfnNtQueryInformationProcess64)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
			NTSTATUS(WINAPI *pfnNtWow64ReadVirtualMemory64)(HANDLE, PVOID64, PVOID, ULONG64, PULONG64);

			(FARPROC)pfnNtQueryInformationProcess64 = GetProcAddress(GetModuleHandle("Ntdll.dll"), "NtWow64QueryInformationProcess64");
			(FARPROC)pfnNtWow64ReadVirtualMemory64 = GetProcAddress(GetModuleHandle("Ntdll.dll"), "NtWow64ReadVirtualMemory64");

			if (pfnNtQueryInformationProcess64 && pfnNtWow64ReadVirtualMemory64)
			{
				//Define PEB structs			
				typedef struct UNICODE_STRING_64 {
					USHORT Length;
					USHORT MaximumLength;
					PVOID64 Buffer;
				}UNICODE_STRING_64, *PUNICODE_STRING_64;
				
				typedef struct PROCESS_BASIC_INFORMATION64
				{
					PVOID Reserved1[2];
					PVOID64 PebBaseAddress;
					PVOID Reserved2[4];
					ULONG_PTR UniqueProcessId[2];
					PVOID Reserved3[2];
				}PROCESS_BASIC_INFORMATION64, *PPROCESS_BASIC_INFORMATION64;

				PROCESS_BASIC_INFORMATION64 pbi64 = { 0 };
				DWORD dwsz = 0;
				if ((ntStatus = pfnNtQueryInformationProcess64(hProc, ProcessBasicInformation, &pbi64, sizeof(pbi64), &dwsz)) == 0 &&
					dwsz <= sizeof(pbi64))
				{
					typedef struct PEB_64
					{
						UCHAR               InheritedAddressSpace;
						UCHAR               ReadImageFileExecOptions;
						UCHAR               BeingDebugged;
						BYTE                b003;
						ULONG               Reserved0;
						ULONG64             Mutant;
						ULONG64             ImageBaseAddress;
						ULONG64             Ldr;
						PVOID64             ProcessParameters;
					}PEB_64, *PPEB_64;

					//Read PEB-64
					PEB_64 peb64 = { 0 };

					ULONG64 uicbSzRead = 0;
					if (pfnNtWow64ReadVirtualMemory64(hProc, pbi64.PebBaseAddress, &peb64, sizeof(peb64), &uicbSzRead) == 0 &&
						uicbSzRead == sizeof(peb64) &&
						peb64.ProcessParameters)
					{
						//Don't know the structure of RTL_USER_PROCESS_PARAMETERS_64 thus read raw bytes
						int ncbSz_rawRUPP64 = sizeof(DWORD)* (6 * 8) + sizeof(UNICODE_STRING_64);
						BYTE *rawRUPP64 = malloc(sizeof(BYTE)*ncbSz_rawRUPP64);
						if (rawRUPP64 == NULL) {
							LOG("failed to get mem for rawRUPP64.\r\n");
						}
						ZeroMemory(rawRUPP64, ncbSz_rawRUPP64);
						
						uicbSzRead = 0;
						LOG("1uicbSzRead:%d ncbSz_rawRUPP64:%d\r\n", uicbSzRead, ncbSz_rawRUPP64);
						if (pfnNtWow64ReadVirtualMemory64(hProc, peb64.ProcessParameters, &rawRUPP64, ncbSz_rawRUPP64, &uicbSzRead) == 0 &&	uicbSzRead == ncbSz_rawRUPP64)
						{
							//Point to the location in raw byte array
							LOG("2uicbSzRead:%d ncbSz_rawRUPP64:%d\r\n", uicbSzRead, ncbSz_rawRUPP64);
							UNICODE_STRING_64* pDesktopName = (UNICODE_STRING_64*)(rawRUPP64 + sizeof(DWORD)* (6 * 8));
							LOG("3\r\n");
							//Get desktop name
							int ncbSzLn = pDesktopName->Length + sizeof(TCHAR);
							LOG("4\r\n");
							BYTE* pBytesDesktopName = malloc(sizeof(BYTE)*ncbSzLn);
							LOG("5\r\n");
							if (pBytesDesktopName != NULL)
							{
								LOG("lal\r\n");
								uicbSzRead = 0;
								if (pfnNtWow64ReadVirtualMemory64(hProc, pDesktopName->Buffer, pBytesDesktopName, ncbSzLn, &uicbSzRead) == 0 &&
									uicbSzRead == ncbSzLn)
								{
									LOG("asdf\r\n");
									//Set last NULL
									*(TCHAR*)(pBytesDesktopName + ncbSzLn - sizeof(TCHAR)) = 0;
									LPCTSTR pStrDesktopName = (LPCTSTR)pBytesDesktopName;

									//We're done
									LOG("blub\r\n");
									LOG("Desktop64: %s\n", pStrDesktopName);
								}
								else
									LOG("ERROR in NtWow64ReadVirtualMemory64 DesktopName: %d\n", GetLastError());

								free(pBytesDesktopName);
							}
							else
								LOG("ERROR DesktopName64 ptr\n");
						}
						else {
							DWORD err = GetLastError();							
							LOG("ERROR in NtWow64ReadVirtualMemory64 RTL_USER_PROCESS_PARAMETERS_32: %d\n", err);
							add_logging_syserror(log, err);
						}
					}
					else
						LOG("ERROR in NtWow64ReadVirtualMemory64 PEB-64: %d\n", GetLastError());
				}
				else
					LOG("ERROR in NtQueryInformationProcess64: %d\n", ntStatus);
			}
			else
				LOG("ERROR NtWow64QueryInformationProcess64 API\n");
		}

		CloseHandle(hProc);
	}
	else {
		LOG("ERROR in OpenProcess: %d\n", GetLastError());
	}
	LOG("finished is process on desk.\r\n");
	return TRUE;
}


BOOL start_app(APPLICATION_INFO appinfo, PSID sid_group, LPSTR mydesk) {
	HANDLE cage_token = NULL;
	if (!create_caged_token(sid_group, &cage_token)) {
		LOG("failed to create caged token.\r\n");
	}
	else {
		LOG("successfull created caged token %x. \r\n",cage_token);
		//read_and_log_groups_and_privileges_from_token(cage_token);
	}
	// create labeller
	if (!create_process_on_desk(CP_LABELLER, mydesk, cage_token)) {
		LOG("failed to create process %s on cage desk with token %x\r\n", CP_LABELLER, cage_token);
		return FALSE;
	}
	else {
		LOG("successfull created process %s on cage desk %s with token %x\r\n", CP_LABELLER, mydesk, cage_token);
	}
	// TODO open mailslot to labeller and send secret picture:: warten auf proz: WaitForInputIdle anschauen
	HANDLE mailslot_labeller = NULL;

	while (!open_mailslot_out(&mailslot_labeller, MAILSLOT_NAME_LABELLER)) {
		LOG("couldn't open mailslot to labeller.\r\n");
		Sleep(300);
	}

	char msg[256] = { '\0' };
	int arg = 0;
	sprintf_s(msg, sizeof(char)* 256, "%s-%d -%s -%s\0", CP_APPINFO_MSG, appinfo.id, appinfo.name, appinfo.window_class_name);
	if (!write_to_slot(&mailslot_labeller, msg)) {
		LOG("failed to send appinfo message.\r\n");
	}



	// create the wanted apllication
	if (!create_process_on_desk(appinfo.name, mydesk, cage_token)) {
		LOG("failed to create process %s on cage desk with token %x\r\n", appinfo.name, cage_token);
		return FALSE;
	}
	else {
		LOG("successfull created process %s on cage desk %s with token %x\r\n", appinfo.name, mydesk, cage_token);
	}
	return TRUE;
}