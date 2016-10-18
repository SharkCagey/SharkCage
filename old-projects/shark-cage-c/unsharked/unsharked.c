///
/// \file unsharked.c
/// \author Richard Heininger - richmont12@gmail.com
/// unsharked - main module
///


#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <AclAPI.h>
#include <Sddl.h>
#include <strsafe.h>

#include "unsharked.h"
#include "helper.h"






int main() {

	// prepare logging
	HANDLE logger;
	LPSTR log_file_name = "logfile.txt";
	init_logging(log_file_name, &logger);
	char **window_station_list = alloc_list(MAX_STATION_ENTRIES, MAX_STATION_NAME);
	char **desktop_list = alloc_list(MAX_DESKTOP_ENTRIES, MAX_STATION_NAME);
	EnumWindowStations(enum_station_callback, (LPARAM)window_station_list);

	LOG("Got Window Station List:\r\n");
	for (int i = 0; i < MAX_STATION_ENTRIES; i++){
		if (window_station_list[i][0] == '\0') {
			break;
		}
		LOG("found station %s\r\n", window_station_list[i]);
	}

	// open the window station
	LPCSTR mystation = "WinSta0";
	ACCESS_MASK ws_access_mask = WINSTA_ALL_ACCESS;
	LOG("trying to open window station %s with rights %X ... ", mystation, ws_access_mask);
	HWINSTA window_station = OpenWindowStationA(mystation, FALSE, ws_access_mask);
	if (window_station == NULL) {
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n", err);
		add_logging_syserror(logger, err);
		goto clean;
	}
	else {
		LOG("done. \r\n");
	}
	SetProcessWindowStation(window_station);


	

	// create new desktop
	char *desktop_name = "mydesk";
	ACCESS_MASK desk_access_mask = GENERIC_ALL;

	// creating SID for everyone group
	PSID sid_everyone = NULL;
	SID_IDENTIFIER_AUTHORITY sid_authworld = SECURITY_WORLD_SID_AUTHORITY;
	LOG("trying to alloc and init sid for everyone group ...");
	if (!AllocateAndInitializeSid(&sid_authworld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sid_everyone)) {
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n", err);
		add_logging_syserror(logger, err);
		goto clean;
	} else {  
		LOG("done. \r\n");
	}

	// create SID for BUILTIN\Administrators group
	PSID sid_admin = NULL;
	SID_IDENTIFIER_AUTHORITY sid_authnt = SECURITY_NT_AUTHORITY;
	LOG("trying to alloc and init sid for admin group ...");
	if (!AllocateAndInitializeSid(&sid_authnt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid_admin)) {
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n", err);
		add_logging_syserror(logger, err);
		goto clean;
	}
	else {
		LOG("done. \r\n");
	}

	// create EXPLICIT_ACCESS structure for an ACE
	EXPLICIT_ACCESS ea[2];
	ZeroMemory(&ea, 2*sizeof(EXPLICIT_ACCESS));

	// fill with ACE for everyone group
	// TODO: check all the setted infos... and know what they do!
	ea[0].grfAccessPermissions = GENERIC_ALL;	// access rights for this entity
	ea[0].grfAccessMode = SET_ACCESS;			// what this entity shall do set rights, remove them, ...
	ea[0].grfInheritance = NO_INHERITANCE;		
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)sid_everyone;
	// fill EXPLICIT_ACCESS with second ACE for admin group
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR)sid_admin;

	// create ACL to contain ACE.
	PACL desk_acl = NULL;
	LOG("trying to create ACL ...");
	if (SetEntriesInAcl(2, ea, NULL, &desk_acl) != ERROR_SUCCESS) {
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n", err);
		add_logging_syserror(logger, err);
		goto clean;
	}
	else {
		LOG("done. \r\n");
	}

	// initialize security descriptor
	PSECURITY_DESCRIPTOR desk_security_desc = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (desk_security_desc == NULL) {
		goto clean;
	}
	LOG("trying to init security descriptor ... ");
	if (!InitializeSecurityDescriptor(desk_security_desc, SECURITY_DESCRIPTOR_REVISION)) {
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n", err);
		add_logging_syserror(logger, err);
		goto clean;
	}
	else {
		LOG("done. \r\n");
	}

	// add ACL to security descriptor
	LOG("trying to add ACL to security descriptor ... ");
	if (!SetSecurityDescriptorDacl(desk_security_desc, TRUE, desk_acl, FALSE)) {
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n", err);
		add_logging_syserror(logger, err);
		goto clean;
	} else {
		LOG("done. \r\n");
	}

	// init security attributes
	SECURITY_ATTRIBUTES desk_security_attr;
	desk_security_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	desk_security_attr.lpSecurityDescriptor = desk_security_desc;
	desk_security_attr.bInheritHandle = FALSE;

	// creating mydesk
	LOG("trying to create desktop %s with access_mask %X and security_attr %X ... ", desktop_name, desk_access_mask, desk_security_attr);
	HDESK mydesk = CreateDesktopA(desktop_name, NULL, NULL, DF_ALLOWOTHERACCOUNTHOOK, desk_access_mask, &desk_security_attr);
	if (mydesk == NULL) 	{
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n",err);
		add_logging_syserror(logger, err);
		goto clean;
	} else {
	   LOG("done. \r\n");
	}

	// get default desktop
	LOG("trying to open desktop %s with access mask %X ... ", "Default", desk_access_mask);
	HDESK defdesk = OpenDesktopA("Default", DF_ALLOWOTHERACCOUNTHOOK, TRUE, desk_access_mask);
	if (defdesk == NULL) {
		DWORD err = GetLastError();
		LOG("failed. Error: %u \r\n",err);
		add_logging_syserror(logger, err);
	} else {
		LOG("done. \r\n");
	}

	// get desktop list
	EnumDesktops(window_station, enum_desktop_callback, (LPARAM)desktop_list);
	LOG("Got Desktop names for window station %s\r\n", window_station_list[0]);
	for (int i = 0; i < MAX_DESKTOP_ENTRIES; i++) {
		if (desktop_list[i][0] == '\0') {
			break;
		}
		LOG("found desktop %s on station %s \r\n", desktop_list[i], window_station_list[0]);
	}



	
	// get the DACL from WinSta0\Default
	SID *dd_psid_owner, *dd_psid_group;
	ACL *dd_dacl;
	SECURITY_DESCRIPTOR *dd_security_desc;
	LOG("trying to get DACL from WinSta0\\Default ... ");
	DWORD result = GetSecurityInfo(defdesk, SE_WINDOW_OBJECT, DACL_SECURITY_INFORMATION, &dd_psid_owner, &dd_psid_group, &dd_dacl, NULL, &dd_security_desc);
	if (ERROR_SUCCESS != result) {
		LOG("failed. Error: %u \r\n", result);
		add_logging_syserror(logger, result);
		goto clean;
	}
	else {
		LOG("done. \r\n");
	}

	// read the DACL
	EXPLICIT_ACCESS *dd_ea;
	LONG dd_ea_entries;
	LOG("trying to read the DACL ... ");
	result = GetExplicitEntriesFromAclA(dd_dacl,&dd_ea_entries,&dd_ea);
	if (ERROR_SUCCESS != result) {
		LOG("failed. Error: %u \r\n", result);
		add_logging_syserror(logger, result);
		goto clean;
	}
	else {
		LOG("done. \r\n");
	}

	
	LOG("got %d entries in DACL:\r\n", dd_ea_entries);
	for (int i = 0; i < dd_ea_entries; i++) {
		LOG("Entry Number %2d -------------------------------------------------\r\n", i);
		LOG("\tACCESS_MODE       : %d\r\n", dd_ea[i].grfAccessMode);
		LOG("\tAccess Permissions: %ld \r\n", dd_ea[i].grfAccessPermissions); // dword %ld
		char ap[1024];
		get_permissions_as_string(dd_ea[i].grfAccessPermissions, ap, 1024);
		LOG("\t\t%s\r\n",ap);
		LOG("\tInheritance       : %ld \r\n",dd_ea[i].grfInheritance); //dword
		LOG("\tTrustee\r\n");
		LOG("\t\tMultipleTrusteeOp: %d \r\n", dd_ea[i].Trustee.MultipleTrusteeOperation);// not supported atm, must be NO_MULTIPLE_TRUSTEE
		LOG("\t\tMultipleTrustee  : %X \r\n",dd_ea[i].Trustee.pMultipleTrustee); // not supported atm, must be NULL
		LOG("\t\tTrusteeForm      : %d \r\n",dd_ea[i].Trustee.TrusteeForm); // enum for type of data
		LOG("\t\tTrusteeType      : %d \r\n",dd_ea[i].Trustee.TrusteeType); // enum for the type. is it user, group,..
		LOG("\t\tptstrName        : %X \r\n",dd_ea[i].Trustee.ptstrName); // pointer to buffer that identifies trustee
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
			OBJECTS_AND_SID *oas = (OBJECTS_AND_SID*) dd_ea[i].Trustee.ptstrName;
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
			} else {
				LOG("\t\t\t\tSID      : converting failed.\r\n");
			}
			char psid_name[256];
			DWORD psid_name_length = 255;
			char psid_domain[256];
			DWORD psid_domain_length = 255;
			SID_NAME_USE psid_type;
			if (LookupAccountSid(NULL,dd_ea[i].Trustee.ptstrName, psid_name, &psid_name_length, psid_domain, &psid_domain_length, &psid_type)) {
				LOG("\t\t\t\tSIDname  : %s\r\n",psid_name);
				LOG("\t\t\t\tSIDDomain: %s\r\n", psid_domain);
				LOG("\t\t\t\tSIDType  : %u\r\n", psid_type)
			} else {
				DWORD err = GetLastError();
				LOG("\t\t\t\tSIDname  : failed. error: %u \r\n", err);
				LOG("\t\t\t\tSIDDomain: failed. error: %u \r\n", err);
				LOG("\t\t\t\tSIDType  : failed. error: %u \r\n", err); 
				add_logging_syserror(logger, err);
			}

			break;
		}
		LOG("-----------------------------------------------------------------\r\n");
	}

	







	// switch to mydesk
	SwitchDesktop(mydesk);

	// now we are on our desktop


	



	// back to default desktop
	SwitchDesktop(defdesk);

	//getchar();

clean:
	if (dd_security_desc) {
		LocalFree(dd_dacl);
	}
	if (dd_dacl) {
		LocalFree(dd_dacl);
	}
	if (desk_security_desc) {
		free(desk_security_desc);
	}
	if (sid_everyone != NULL) {
		FreeSid(sid_everyone);
	}
	if (sid_admin != NULL) {
		FreeSid(sid_admin);
	}
	if (mydesk) { 
		CloseDesktop(mydesk); 
	}
	if (window_station) {
		CloseWindowStation(window_station);
	}
	free_list(MAX_DESKTOP_ENTRIES, desktop_list);
	free_list(MAX_STATION_ENTRIES, window_station_list);
	if (logger != INVALID_HANDLE_VALUE) {
		CloseHandle(logger);
	}

	return EXIT_SUCCESS;
}