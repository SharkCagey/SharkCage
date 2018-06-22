#include "stdafx.h"

#include "../CageNetwork/NetworkManager.h"
#include "../CageNetwork/MsgManager.h"

#define byte WIN_BYTE_OVERRIDE

#include "stdio.h"
#include "Aclapi.h"
#include <tchar.h>
#include "sddl.h"
#include <string>
#include <LM.h>
#include <memory>
#include <vector>
#include <thread>
#include <optional>
#include <fstream>
#include <cwctype>
#include <regex>

#include "CageManager.h"
#include "CageLabeler.h"
#include "CageDesktop.h"

NetworkManager network_manager(ContextType::MANAGER);

int main()
{
	//MessageBox(0, L"Attach", L"CageManager", 0);
	CageManager cage_manager;
	auto group_sid = cage_manager.CreateSID();

	auto security_attributes = cage_manager.CreateACL(std::move(group_sid));

	if (!security_attributes.has_value())
	{
		return 1;
	}

	// listen for the message
	std::wstring message = network_manager.Listen(10);
	auto parse_result = cage_manager.ParseMessage(message);

	if (!parse_result.has_value() || parse_result != ManagerMessage::START_PROCESS)
	{
		std::cout << "Could not process incoming message" << std::endl;
		return 1;
	}

	CageData cage_data = { message };
	if (!cage_manager.ParseStartProcessMessage(cage_data))
	{
		std::cout << "Could not process start process message" << std::endl;
		return 1;
	}

	std::thread desktop_thread(
		&CageManager::StartCage,
		cage_manager,
		security_attributes->lpSecurityDescriptor,
		cage_data
	);

	desktop_thread.join();

	return 0;
}

// FIXME: move this and other duplicate functions and their respective implementation in cage service to its own helper class or something like that
bool BeginsWith(const std::wstring &string_to_search, const std::wstring &prefix)
{
	if (prefix.length() > string_to_search.length())
	{
		throw std::invalid_argument("prefix longer than the actual string");
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

std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> CageManager::CreateSID()
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

// Function to decode the message and do a respective action
// "START_PC" "path/to.exe"
std::optional<ManagerMessage> CageManager::ParseMessage(std::wstring &message)
{
	if (BeginsWith(message, ManagerMessageToString(ManagerMessage::START_PROCESS)))
	{
		// read config
		auto message_cmd_length = ManagerMessageToString(ManagerMessage::START_PROCESS).length();
		message = message.substr(message_cmd_length);
		
		// trim whitespace at beginning
		message.erase(message.begin(), std::find_if(message.begin(), message.end(), [](wchar_t c)
		{
			return !std::iswspace(c);
		}));

		// trim whitespace at end
		message.erase(std::find_if(message.rbegin(), message.rend(), [](wchar_t c)
		{
			return !std::iswspace(c);
		}).base(), message.end());

		return ManagerMessage::START_PROCESS;
	}
	else if (BeginsWith(message, ManagerMessageToString(ManagerMessage::STOP_PROCESS)))
	{
		// Stop process
		return ManagerMessage::STOP_PROCESS;
	}
	else
	{
		std::wcout << "Received unrecognized message: " << message << std::endl;
	}
	return std::nullopt;
}

bool CageManager::ParseStartProcessMessage(CageData &cage_data)
{
	std::ifstream config_stream;
	config_stream.open(cage_data.config_path);
	
	if (config_stream.is_open())
	{	
		try
		{
			nlohmann::json json_config;
			config_stream >> json_config;

			auto path = json_config[APPLICATION_PATH_PROPERTY].get<std::string>();
			auto application_name = json_config[APPLICATION_NAME_PROPERTY].get<std::string>();
			auto token = json_config[APPLICATION_TOKEN_PROPERTY].get<std::string>();
			auto hash = json_config[APPLICATION_HASH_PROPERTY].get<std::string>();
			auto additional_application = json_config[ADDITIONAL_APPLICATION_NAME_PROPERTY].get<std::string>();
			auto additional_application_path = json_config[ADDITIONAL_APPLICATION_PATH_PROPERTY].get<std::string>();

			// no suitable alternative in c++ standard yet, so it is safe to use for now
			// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			cage_data.app_path = converter.from_bytes(path);
			cage_data.app_name = converter.from_bytes(application_name);
			cage_data.app_token = converter.from_bytes(token);
			cage_data.app_hash = converter.from_bytes(hash);
			cage_data.additional_app_name = converter.from_bytes(additional_application);
			cage_data.additional_app_path = converter.from_bytes(additional_application_path);

			if (!cage_data.hasAdditionalAppInfo() || cage_data.additional_app_name->compare(L"None") == 0)
			{
				cage_data.additional_app_name.reset();
				cage_data.additional_app_path.reset();
			}
	
			return true;
		}
		catch (std::exception e)
		{
			std::cout << "Could not parse json: " << e.what() << std::endl;
			return false;
		}
	}

	return false;
}

void CageManager::StartCageLabeler(
	HDESK desktop_handle,
	const CageData &cage_data,
	const int work_area_width,
	const std::wstring &labeler_window_class_name)
{
	::SetThreadDesktop(desktop_handle);
	CageLabeler cage_labeler = CageLabeler(cage_data, work_area_width, labeler_window_class_name);
	cage_labeler.Init();
}

void CageManager::StartCage(PSECURITY_DESCRIPTOR security_descriptor, const CageData &cage_data)
{
	const std::wstring DESKTOP_NAME = L"shark_cage_desktop";
	const int work_area_width = 300;
	CageDesktop cage_desktop(
		security_descriptor, 
		cage_data,
		work_area_width,
		DESKTOP_NAME);

	HDESK desktop_handle;
	if (!cage_desktop.Init(desktop_handle))
	{
		std::cout << "Failed to create/launch the cage desktop" << std::endl;
		return;
	}

	const std::wstring LABELER_WINDOW_CLASS_NAME = L"shark_cage_token_window";
	std::thread labeler_thread(
		&CageManager::StartCageLabeler,
		this,
		desktop_handle,
		cage_data,
		work_area_width,
		LABELER_WINDOW_CLASS_NAME
	);


	//PETER´S ACCESS TOKEN THINGS

	// We need in order to create the process.
	STARTUPINFO info = {};
	info.dwFlags = STARTF_USESHOWWINDOW;
	info.wShowWindow = SW_MAXIMIZE;

	// The desktop's name where we are going to start the application. In this case, our new desktop.
	info.lpDesktop = const_cast<LPWSTR>(DESKTOP_NAME.c_str());

	// Create the process.
	PROCESS_INFORMATION process_info = {};
	std::vector<wchar_t> path_buf(cage_data.app_path.begin(), cage_data.app_path.end());
	path_buf.push_back(0);

	if (!::CreateProcess(NULL, path_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &process_info))
	{
		std::cout << "Failed to start process. Err " << ::GetLastError() << std::endl;
	}

	std::optional<PROCESS_INFORMATION> process_info_additional_app;
	if (cage_data.hasAdditionalAppInfo())
	{
		std::vector<wchar_t> additional_app_path_buf(cage_data.additional_app_path->begin(), cage_data.additional_app_path->end());
		additional_app_path_buf.push_back(0);
		process_info_additional_app = { 0 };
		STARTUPINFO info_additional_app = { 0 };
		info_additional_app.lpDesktop = const_cast<LPWSTR>(DESKTOP_NAME.c_str());

		if (!::CreateProcess(NULL, additional_app_path_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &info_additional_app, &process_info_additional_app.value()))
		{
			std::cout << "Failed to start additional process. Err " << GetLastError() << std::endl;
		}
	}

	bool keep_cage_running = true;
	std::vector<HANDLE> handles = { labeler_thread.native_handle(), process_info.hProcess };
	if (process_info_additional_app.has_value())
	{
		handles.push_back(process_info_additional_app->hProcess);
	}
	
	// wait for all open window handles on desktop + cage_labeler
	while (keep_cage_running)
	{
		DWORD res = ::WaitForMultipleObjects(handles.size(), handles.data(), FALSE, 500);

		if (res != WAIT_TIMEOUT)
		{
			// this is always the labeler_thread
			if (res == WAIT_OBJECT_0)
			{
				keep_cage_running = false;
			}
			else
			{
				std::cout << "other handle" << std::endl;
				// check all other handles
				for (size_t i = 1; i < handles.size(); ++i)
				{
					if (res == WAIT_OBJECT_0 + i)
					{
						std::cout << "removing" << std::endl;
						// remove the handle we got an event for
						auto iter = handles.begin();
						std::advance(iter, i);
						handles.erase(iter);
					}
				}
			}
		}

		if (!keep_cage_running || handles.size() < 2)
		{
			// labeler still running, tell it to shut down
			if (keep_cage_running)
			{
				auto labeler_hwnd = ::FindWindow(LABELER_WINDOW_CLASS_NAME.c_str(), nullptr);
				if (labeler_hwnd)
				{
					::SendMessage(labeler_hwnd, WM_CLOSE, NULL, NULL);
				}
				else
				{
					// FIXME: last resort, there might be a better alternative to just exiting the message loop? (e.g. synchronization object)
					::PostThreadMessage(::GetThreadId(labeler_thread.native_handle()), WM_QUIT, NULL, NULL);
				}
			}
			labeler_thread.join();
			break;
		}
	}


	// we can't rely on the process handles to keep track of open processes on
	// the secure desktop as programs (e.g. Internet Explorer) spawn multiple processes and 
	// maybe even close the initial process we spawned ourselves
	// Solution: enumerate all top level windows on the desktop not belonging to our process and message these handles
	std::pair<DWORD, std::vector<HWND>*> callback_window_data;
	std::vector<HWND> window_handles_to_signal;
	callback_window_data.first = ::GetCurrentProcessId();
	callback_window_data.second = &window_handles_to_signal;

	::EnumDesktopWindows(desktop_handle, &CageManager::GetOpenWindowHandles, reinterpret_cast<LPARAM>(&callback_window_data));

	for (HWND hwnd_handle : window_handles_to_signal)
	{
		::SetLastError(0);
		::PostMessage(hwnd_handle, WM_CLOSE, NULL, NULL);
		std::cout << "Sending WM_CLOSE to window: " << hwnd_handle << ", last error: " << ::GetLastError() << std::endl;
	}

	// and get all open process handles we have to wait for
	std::pair<DWORD, std::vector<HANDLE>*> callback_process_data;
	std::vector<HANDLE> process_handles_for_closing;
	callback_process_data.first = ::GetCurrentProcessId();
	callback_process_data.second = &process_handles_for_closing;

	::EnumDesktopWindows(desktop_handle, &CageManager::GetOpenProcesses, reinterpret_cast<LPARAM>(&callback_process_data));

	// give users up to 5s to react to close prompt of process, maybe increase this?
	if (::WaitForMultipleObjects(process_handles_for_closing.size(), process_handles_for_closing.data(), TRUE, 5000) != WAIT_OBJECT_0)
	{
		for (HANDLE process_handle : process_handles_for_closing)
		{
			::SetLastError(0);
			::TerminateProcess(process_handle, 0);
			std::cout << "Closing process: " << process_handle << ", last error: " << ::GetLastError() << std::endl;
		}		
	}

	// close our handles
	::CloseHandle(process_info.hProcess);
	::CloseHandle(process_info.hThread);

	if (process_info_additional_app.has_value())
	{
		::CloseHandle(process_info_additional_app->hProcess);
		::CloseHandle(process_info_additional_app->hThread);
	}
}

std::optional<SECURITY_ATTRIBUTES> CageManager::CreateACL(std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> group_sid)
{
	// create SID for BUILTIN\Administrators group
	PSID sid_admin;
	SID_IDENTIFIER_AUTHORITY sid_authnt = SECURITY_NT_AUTHORITY;
	if (!::AllocateAndInitializeSid(&sid_authnt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid_admin))
	{
		std::cout << "Obtain admin SID error: " << ::GetLastError() << std::endl;
		return std::nullopt;
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
	// if TrusteeForm is TRUSTEE_IS_SID, the ptstrName must point to the binary representation of the SID (do NOT convert to string!)
	PSID group_sid_raw = group_sid.get();
	explicit_access_group.Trustee.ptstrName = static_cast<LPWSTR>(group_sid_raw);

	// EXPLICIT_ACCESS with second ACE for admin group
	explicit_access_admin.grfAccessPermissions = GENERIC_ALL;
	explicit_access_admin.grfAccessMode = SET_ACCESS; //DENY_ACCES
	explicit_access_admin.grfInheritance = NO_INHERITANCE;
	explicit_access_admin.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicit_access_admin.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	// if TrusteeForm is TRUSTEE_IS_SID, the ptstrName must point to the binary representation of the SID (do NOT convert to string!)
	explicit_access_admin.Trustee.ptstrName = static_cast<LPWSTR>(sid_admin);

	// Create a new ACL that contains the new ACEs.
	PACL acl = NULL;
	EXPLICIT_ACCESS ea[2] = { explicit_access_group, explicit_access_admin };
	auto result = ::SetEntriesInAcl(2, ea, NULL, &acl);
	if (result != ERROR_SUCCESS)
	{
		std::cout << "SetEntriesInAcl error: " << ::GetLastError() << std::endl;
		return std::nullopt;
	}

	// Initialize a security descriptor.  
	PSECURITY_DESCRIPTOR security_descriptor = const_cast<PSECURITY_DESCRIPTOR>(::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));
	if (security_descriptor == nullptr)
	{
		std::cout << "LocalAlloc error: " << ::GetLastError() << std::endl;
		return std::nullopt;
	};

	if (!::InitializeSecurityDescriptor(security_descriptor, SECURITY_DESCRIPTOR_REVISION))
	{
		std::cout << "InitializeSecurityDescriptor error: " << ::GetLastError() << std::endl;
		return std::nullopt;
	}

	// Add the ACL to the security descriptor. 
	if (!::SetSecurityDescriptorDacl(security_descriptor,
		TRUE,     // bDaclPresent flag   
		acl,
		FALSE))   // not a default DACL 
	{
		std::cout << "SetSecurityDescriptorDacl error: " << ::GetLastError() << std::endl;
		return std::nullopt;
	}

	// Initialize a security attributes structure
	SECURITY_ATTRIBUTES security_attributes = { 0 };
	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.lpSecurityDescriptor = security_descriptor;
	security_attributes.bInheritHandle = FALSE;

	return security_attributes;
}

BOOL CALLBACK CageManager::GetOpenProcesses(_In_ HWND hwnd, _In_ LPARAM l_param)
{
	auto data = reinterpret_cast<std::pair<DWORD, std::vector<HANDLE> *> *>(l_param);
	auto current_process_id = data->first;
	auto handles = data->second;

	wchar_t title[300];
	::GetWindowText(hwnd, title, 280);
	wchar_t class_name[300];
	::GetClassName(hwnd, class_name, 280);
	std::wcout << L"enumerating window (process), title: " << title << ", class: " << class_name << std::endl;

	DWORD process_id;
	::GetWindowThreadProcessId(hwnd, &process_id);

	if (process_id != current_process_id && ::IsWindowVisible(hwnd))
	{
		::SetLastError(0);
		auto handle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
		std::cout << "Push process, last error: " << ::GetLastError() << std::endl;
		handles->push_back(handle);
	}

	return TRUE;
}

BOOL CALLBACK CageManager::GetOpenWindowHandles(_In_ HWND hwnd, _In_ LPARAM l_param)
{
	auto data = reinterpret_cast<std::pair<DWORD, std::vector<HWND> *> *>(l_param);
	auto current_process_id = data->first;
	auto hwnds = data->second;

	wchar_t title[300];
	::GetWindowText(hwnd, title, 280);
	wchar_t class_name[300];
	::GetClassName(hwnd, class_name, 280);
	std::wcout << L"enumerating window (handle), title: " << title << ", class: " << class_name << std::endl;

	::SetLastError(0);
	DWORD process_id;
	::GetWindowThreadProcessId(hwnd, &process_id);

	if (process_id != current_process_id && ::IsWindowVisible(hwnd))
	{
		std::cout << "Push window, last error: " << ::GetLastError() << std::endl;
		hwnds->push_back(hwnd);
	}

	return TRUE;
}