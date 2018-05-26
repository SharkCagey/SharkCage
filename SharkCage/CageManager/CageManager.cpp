#include "stdafx.h"

#include "../CageService/NetworkManager.h"

#define byte WIN_BYTE_OVERRIDE
#include "CageDesktop.h"

#include "stdio.h"
#include "Aclapi.h"
#include <tchar.h>
#include "sddl.h"
#include <string>
#include <LM.h>
#include <memory>
#include <vector>

#include <LMaccess.h>
#include <lmerr.h>

#include "../CageService/MsgManager.h"

#pragma comment(lib, "netapi32.lib")

template<typename T>
auto local_free_deleter = [&](T resource) { ::LocalFree(resource); };

std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> CreateSID();
bool CreateACL(std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> group_sid);
std::wstring OnReceive(const std::wstring &message);
bool BeginsWith(const std::wstring &string, const std::wstring &prefix);

NetworkManager network_manager(ExecutableType::MANAGER);

int main()
{
	auto group_sid = CreateSID();
	return CreateACL(std::move(group_sid));
}

std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> CreateSID()
{
	std::wstring group_name = L"shark_cage_group";
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

// FIXME: move this and other duplicate functions and their respective implementation in cage service to its own helper class or something like that
bool BeginsWith(const std::wstring &string_to_search, const std::wstring &prefix)
{
	if (prefix.length() > string_to_search.length())
	{
		return false;
		// Throw Exception "Bad parameters: prefix longer than the actual string"
	}
	else
	{
		if (string_to_search.compare(0, prefix.length(), prefix) == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

// Function to decode the message and do a respective action
// "START_PC" "path/to.exe"
std::wstring OnReceive(const std::wstring &message)
{
	std::wstring path = L"";
	if (BeginsWith(message, ManagerMessageToString(ManagerMessage::START_PROCESS)))
	{
		// Start process
		path = message.substr(9);
	}
	else if (BeginsWith(message, ManagerMessageToString(ManagerMessage::STOP_PROCESS)))
	{
		// Stop process
	}
	else
	{
		std::wcout << "Received unrecognized message: " << message << std::endl;
	}
	return path;
}

void StartCageDesktop(PSECURITY_DESCRIPTOR security_descriptor)
{
	CageDesktop cage_desktop = CageDesktop::CageDesktop(security_descriptor);
	if (!cage_desktop.Init())
	{
		std::cout << "Failed to create/launch the cage desktop" << std::endl;
	}
}

bool CreateACL(std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> group_sid)
{
	DWORD result;
	PACL acl = NULL;
	PSECURITY_DESCRIPTOR security_descriptor = NULL;
	SECURITY_ATTRIBUTES security_attributes;
	//Listen for the message
	std::wstring message = network_manager.Listen();
	std::wstring path = OnReceive(message);

	// create SID for BUILTIN\Administrators group
	PSID sid_admin = NULL;
	SID_IDENTIFIER_AUTHORITY sid_authnt = SECURITY_NT_AUTHORITY;
	if (!::AllocateAndInitializeSid(&sid_authnt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid_admin))
	{
		std::cout << "Obtain Admin SID Error: " << ::GetLastError() << std::endl;
		return false;
	}

	// create EXPLICIT_ACCESS structure for an ACE
	EXPLICIT_ACCESS explicit_access_group = { 0 };
	EXPLICIT_ACCESS explicit_access_admin = { 0 };

	// EXPLICIT_ACCESS for created groupc 
	explicit_access_group.grfAccessPermissions = GENERIC_ALL;
	explicit_access_group.grfAccessMode = SET_ACCESS;
	explicit_access_group.grfInheritance = NO_INHERITANCE;
	explicit_access_group.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicit_access_group.Trustee.TrusteeType = TRUSTEE_IS_GROUP;

	wchar_t *group_sid_tmp;
	::ConvertSidToStringSid(group_sid.get(), &group_sid_tmp);
	std::unique_ptr<wchar_t, decltype(local_free_deleter<wchar_t*>)> group_sid_string(group_sid_tmp, local_free_deleter<wchar_t*>);

	explicit_access_group.Trustee.ptstrName = group_sid_string.get();

	// EXPLICIT_ACCESS with second ACE for admin group
	explicit_access_admin.grfAccessPermissions = GENERIC_ALL;
	explicit_access_admin.grfAccessMode = SET_ACCESS; //DENY_ACCES
	explicit_access_admin.grfInheritance = NO_INHERITANCE;
	explicit_access_admin.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicit_access_admin.Trustee.TrusteeType = TRUSTEE_IS_GROUP;

	wchar_t *admin_sid_tmp;
	::ConvertSidToStringSid(group_sid.get(), &admin_sid_tmp);
	std::unique_ptr<wchar_t, decltype(local_free_deleter<wchar_t*>)> admin_sid_string(admin_sid_tmp, local_free_deleter<wchar_t*>);

	explicit_access_admin.Trustee.ptstrName = admin_sid_string.get();

	// Create a new ACL that contains the new ACEs.
	EXPLICIT_ACCESS ea[2] = { explicit_access_group, explicit_access_admin };
	result = ::SetEntriesInAcl(2, ea, NULL, &acl);
	if (result != ERROR_SUCCESS)
	{
		std::cout << "SetEntriesInAcl Error: " << ::GetLastError() << std::endl;
	}

	// Initialize a security descriptor.  
	security_descriptor = const_cast<PSECURITY_DESCRIPTOR>(::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));
	if (security_descriptor == nullptr)
	{
		std::cout << "LocalAlloc Error: " << ::GetLastError() << std::endl;
	}

	if (!::InitializeSecurityDescriptor(security_descriptor, SECURITY_DESCRIPTOR_REVISION))
	{
		std::cout << "InitializeSecurityDescriptor Error: " << ::GetLastError() << std::endl;
	}

	// Add the ACL to the security descriptor. 
	if (!::SetSecurityDescriptorDacl(security_descriptor,
		TRUE,     // bDaclPresent flag   
		acl,
		FALSE))   // not a default DACL 
	{
		std::cout << "SetSecurityDescriptorDacl Error: " << ::GetLastError() << std::endl;
	}

	// Initialize a security attributes structure.
	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.lpSecurityDescriptor = security_descriptor;
	security_attributes.bInheritHandle = FALSE;

	thread desktop_thread(StartCageDesktop, security_descriptor);

	//PETER´S ACCESS TOKEN THINGS

	// We need in order to create the process.
	STARTUPINFO info = {};

	// The desktop's name where we are going to start the application. In this case, our new desktop.
	std::wstring new_desktop_name = L"shark_cage_desktop";
	std::vector<wchar_t> new_desktop_name_buf(new_desktop_name.begin(), new_desktop_name.end());
	new_desktop_name_buf.push_back(0);
	info.lpDesktop = new_desktop_name_buf.data();

	// PETER´S ACCESS TOKEN THINGS

	// Create the process.
	PROCESS_INFORMATION process_info = {};
	std::vector<wchar_t> path_buf(path.begin(), path.end());
	path_buf.push_back(0);
	if (::CreateProcess(NULL, path_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &process_info))
	{
		std::cout << "Failed to start process. Err " << ::GetLastError() << std::endl;
	}

	desktop_thread.join();

	//::SendMessage(nullptr /*main handle of new process*/, WM_CLOSE, NULL);
	// with timeout, if nothing happens, terminate process
	if (!::TerminateProcess(process_info.hProcess, 0))
	{
		std::cout << "Failed to terminate process Err " << ::GetLastError() << std::endl;
	}

	// wait for the process to exit
	::WaitForSingleObject(process_info.hProcess, INFINITE);
	::CloseHandle(process_info.hProcess);
	::CloseHandle(process_info.hThread);
	return Ok;
}
