///
/// \file cage_lib_token.c
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage library - token module
/// all the dirty little helpers ;)
///
#ifndef __cage_lib_token_h__
#define __cage_lib_token_h__

#include <winternl.h>

BOOL get_privileged_process(LUID luid, PDWORD privileged_pid_out);
BOOL get_sid(char *name, PSID *sid);
BOOL get_token_functions(void);
BOOL get_active_session(PDWORD session_id_active);
BOOL get_token_with_privileg(LUID privilege, PHANDLE token_out);
BOOL create_manager_token(HANDLE token_in, DWORD session_id, LUID luid_to_enable, PHANDLE token_out);
BOOL create_process_on_desk(LPSTR process, LPSTR desk_name, HANDLE token);


// taking these 2 structs from kernel headers, so we can't just include the needed headers, this workaround is needed
/*typedef struct _UNICODE_STRING {
	unsigned short Length;
	unsigned short MaximumLength;
	PWCHAR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	unsigned long Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	unsigned long Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
*/
UINT(WINAPI * ZwCreateToken)(
	PHANDLE				TokenHandle,
	ACCESS_MASK			DesiredAccess,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	TOKEN_TYPE			Type,
	PLUID					AuthenticationId,
	PLARGE_INTEGER		ExpirationTime,
	PTOKEN_USER			User,
	PTOKEN_GROUPS			Groups,
	PTOKEN_PRIVILEGES		Privileges,
	PTOKEN_OWNER			Owner,
	PTOKEN_PRIMARY_GROUP	PrimaryGroup,
	PTOKEN_DEFAULT_DACL	DefaultDacl,
	PTOKEN_SOURCE			Source
	);
//UINT(WINAPI * RtlNtStatusToDosError)(UINT dwError);



#endif