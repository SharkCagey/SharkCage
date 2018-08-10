#include "stdafx.h"

#include "SecuritySetup.h"
#include <vector>
#include <iostream>
#include <LM.h>
#include <AclAPI.h>

#pragma comment(lib, "netapi32.lib")

std::optional<SECURITY_ATTRIBUTES> SecuritySetup::GetSecurityAttributes(const std::wstring &group_name)
{
	auto group_sid = CreateSID(group_name);
	auto access_control_list = CreateACL(std::move(group_sid));

	if (!access_control_list.has_value())
	{
		return std::nullopt;
	}

	// Initialize a security descriptor.  
	PSECURITY_DESCRIPTOR security_descriptor = const_cast<PSECURITY_DESCRIPTOR>(::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));
	if (security_descriptor == nullptr)
	{
		std::cout << "LocalAlloc error: " << ::GetLastError() << std::endl;
		return std::nullopt;
	}
	else
	{
		this->security_descriptor = security_descriptor;
	}

	if (!::InitializeSecurityDescriptor(security_descriptor, SECURITY_DESCRIPTOR_REVISION))
	{
		std::cout << "InitializeSecurityDescriptor error: " << ::GetLastError() << std::endl;
		::LocalFree(security_descriptor);
		return std::nullopt;
	}

	// Add the ACL to the security descriptor. 
	auto acl = access_control_list.value();
	if (!::SetSecurityDescriptorDacl(security_descriptor,
		TRUE,     // bDaclPresent flag   
		acl,
		FALSE))   // not a default DACL 
	{
		std::cout << "SetSecurityDescriptorDacl error: " << ::GetLastError() << std::endl;
		::LocalFree(security_descriptor);
		return std::nullopt;
	}

	// Initialize a security attributes structure
	SECURITY_ATTRIBUTES security_attributes = { 0 };
	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.lpSecurityDescriptor = security_descriptor;
	security_attributes.bInheritHandle = FALSE;

	return security_attributes;
}

std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> SecuritySetup::CreateSID(const std::wstring &group_name)
{
	LOCALGROUP_INFO_0 localgroup_info;
	DWORD buffer_size = 0;

	// create a group
	std::vector<wchar_t> group_name_buf(group_name.begin(), group_name.end());
	group_name_buf.push_back(0);
	localgroup_info.lgrpi0_name = group_name_buf.data();
	::NetLocalGroupAdd(NULL, 0, reinterpret_cast<LPBYTE>(&localgroup_info), NULL);

	// obtain sid
	const DWORD INITIAL_SIZE = 32;
	DWORD cb_sid = 0;
	DWORD domain_buffer_size = INITIAL_SIZE;
	std::vector<wchar_t> domain_name(INITIAL_SIZE);
	DWORD cch_domain_name = 0;
	SID_NAME_USE sid_type;
	DWORD sid_buffer_size = INITIAL_SIZE;

	// First call of the function in order to get the size needed for the SID
	::LookupAccountName(
		NULL,            // Computer name. NULL for the local computer  
		group_name_buf.data(),
		NULL,
		&cb_sid,
		domain_name.data(),
		&cch_domain_name,
		&sid_type
	);

	// Second call of the function in order to get the SID
	std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> sid((PSID*)::LocalAlloc(LPTR, cb_sid), local_free_deleter<PSID>);

	::LookupAccountName(
		NULL,
		group_name_buf.data(),
		sid.get(),
		&cb_sid,
		domain_name.data(),
		&cch_domain_name,
		&sid_type
	);

	return sid;
}

std::optional<PACL> SecuritySetup::CreateACL(std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> group_sid)
{
	// create SID for BUILTIN\Administrators group
	PSID sid_admin;
	SID_IDENTIFIER_AUTHORITY sid_authnt = SECURITY_NT_AUTHORITY;
	if (!::AllocateAndInitializeSid(&sid_authnt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid_admin))
	{
		std::cout << "Obtain admin SID error: " << ::GetLastError() << std::endl;
		return std::nullopt;
	}

	// create SID for NT AUTHORITY\SYSTEM group
	DWORD sid_size = SECURITY_MAX_SID_SIZE;
	//PSID sid_system = (PSID) new BYTE[sid_size];
	std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> sid_system((PSID*)::LocalAlloc(LPTR, sid_size), local_free_deleter<PSID>);
	if (!CreateWellKnownSid(WinLocalSystemSid, NULL, sid_system.get(), &sid_size)) {
		FreeSid(sid_admin);
		std::wcout << L"Cannot create system SID" << std::endl;
		return std::nullopt;
	}

	// create EXPLICIT_ACCESS structure for an ACE
	EXPLICIT_ACCESS explicit_access_group = { 0 };
	EXPLICIT_ACCESS explicit_access_admin = { 0 };
	EXPLICIT_ACCESS explicit_access_system = { 0 };

	// EXPLICIT_ACCESS for created group
	explicit_access_group.grfAccessPermissions = DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU |
													DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK | 
													DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS;
	explicit_access_group.grfAccessMode = SET_ACCESS;
	explicit_access_group.grfInheritance = NO_INHERITANCE;
	explicit_access_group.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicit_access_group.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	// if TrusteeForm is TRUSTEE_IS_SID, the ptstrName must point to the binary representation of the SID (do NOT convert to string!)
	PSID group_sid_raw = group_sid.get();
	explicit_access_group.Trustee.ptstrName = static_cast<LPWSTR>(group_sid_raw);

	// EXPLICIT_ACCESS with second ACE for admin group
	explicit_access_admin.grfAccessPermissions = DELETE | DESKTOP_ENUMERATE | READ_CONTROL | WRITE_DAC |
													WRITE_OWNER;
	explicit_access_admin.grfAccessMode = SET_ACCESS;
	explicit_access_admin.grfInheritance = NO_INHERITANCE;
	explicit_access_admin.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicit_access_admin.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	// if TrusteeForm is TRUSTEE_IS_SID, the ptstrName must point to the binary representation of the SID (do NOT convert to string!)
	explicit_access_admin.Trustee.ptstrName = static_cast<LPWSTR>(sid_admin);

	// EXPLICIT_ACCESS with third ACE for local system account
	explicit_access_system.grfAccessPermissions = DELETE | DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU |
													DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK |
													DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP | READ_CONTROL |
													WRITE_DAC | WRITE_OWNER;
	explicit_access_system.grfAccessMode = SET_ACCESS;
	explicit_access_system.grfInheritance = NO_INHERITANCE;
	explicit_access_system.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicit_access_system.Trustee.TrusteeType = TRUSTEE_IS_USER;
	// if TrusteeForm is TRUSTEE_IS_SID, the ptstrName must point to the binary representation of the SID (do NOT convert to string!)
	PSID system_sid_raw = sid_system.get();
	explicit_access_system.Trustee.ptstrName = static_cast<LPWSTR>(system_sid_raw);

	// Create a new ACL that contains the new ACEs.
	PACL acl = nullptr;
	EXPLICIT_ACCESS ea[3] = { explicit_access_group, explicit_access_admin, explicit_access_system };
	auto result = ::SetEntriesInAcl(3, ea, NULL, &acl);
	if (result != ERROR_SUCCESS)
	{
		FreeSid(sid_admin);
		std::cout << "SetEntriesInAcl error: " << ::GetLastError() << std::endl;
		return std::nullopt;
	}

	FreeSid(sid_admin);
	return acl;
}

