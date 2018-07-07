#include "stdafx.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <cassert>
#include <thread>
#include <Rpc.h>

#include "wtsapi32.h"

#include "CageService.h"
#include "SecuritySetup.h"
#include "CageDesktop.h"
#include "../SharedFunctionality/SharedFunctions.h"

#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "Wtsapi32.lib")

const std::wstring CAGE_MANAGER_NAME = L"CageManager.exe";

CageService::CageService() noexcept
	: cage_manager_process_id(0)
{
}

bool CageService::CageManagerRunning()
{
	return cage_manager_process_id > 0;
}

DWORD CageService::StartCageManager(DWORD session_id)
{
	std::vector<wchar_t> filename_buffer(MAX_PATH);
	::GetModuleFileName(nullptr, filename_buffer.data(), MAX_PATH);

	std::wstring filename(filename_buffer.data());
	auto pos = filename.rfind(L"\\");
	if (pos != std::wstring::npos)
	{
		filename = filename.substr(0, pos) + L"\\" + CAGE_MANAGER_NAME;
		return StartCageManager(filename, session_id);
	}

	return 0;
}

DWORD CageService::StartCageManager(const std::wstring &app_name, DWORD session_id)
{
	return StartCageManager(app_name, std::nullopt, session_id);
}

std::optional<HANDLE> ImpersonateActiveUser()
{
	DWORD session_id = -1;
	DWORD session_count = 0;

	WTS_SESSION_INFO *pSession = nullptr;


	if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSession, &session_count))
	{
		//log success
	}
	else
	{
		//log error
		return std::nullopt;
	}

	std::optional<HANDLE> hUserToken;
	for (DWORD i = 0; i < session_count; i++)
	{
		session_id = pSession[i].SessionId;

		WTS_CONNECTSTATE_CLASS wts_connect_state = WTSDisconnected;
		WTS_CONNECTSTATE_CLASS* ptr_wts_connect_state = NULL;

		DWORD bytes_returned = 0;
		if (::WTSQuerySessionInformation(
			WTS_CURRENT_SERVER_HANDLE,
			session_id,
			WTSConnectState,
			reinterpret_cast<LPTSTR*>(&ptr_wts_connect_state),
			&bytes_returned))
		{
			wts_connect_state = *ptr_wts_connect_state;
			::WTSFreeMemory(ptr_wts_connect_state);
			if (wts_connect_state != WTSActive) continue;
		}
		else
		{
			//log error
			continue;
		}

		HANDLE hImpersonationToken;

		if (!WTSQueryUserToken(session_id, &hImpersonationToken))
		{
			//log error
			continue;
		}


		//Get real token from impersonation token
		DWORD neededSize1 = 0;
		HANDLE *realToken = new HANDLE;
		if (GetTokenInformation(hImpersonationToken, (::TOKEN_INFORMATION_CLASS) TokenLinkedToken, realToken, sizeof(HANDLE), &neededSize1))
		{
			CloseHandle(hImpersonationToken);
			hImpersonationToken = *realToken;
		}
		else
		{
			//log error
			continue;
		}

		HANDLE test;

		if (!DuplicateTokenEx(hImpersonationToken,
			MAXIMUM_ALLOWED,
			NULL,
			SecurityImpersonation,
			TokenImpersonation,
			&test))
		{
			//log error
			continue;
		}

		CloseHandle(hImpersonationToken);

		hUserToken = test;
		break;
	}

	WTSFreeMemory(pSession);

	return hUserToken;
}

std::optional<HANDLE> CageService::CreateImpersonatingUserToken()
{
	DWORD session_id = ::WTSGetActiveConsoleSessionId();

	HANDLE service_token_handle;
	HANDLE user_session_token_handle;

	// Use new token with privileges for the trusting computing base
	if (!::ImpersonateSelf(SecurityDelegation))
	{
		std::wostringstream os;
		os << "ImpersonateSelf failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return nullptr;
	}

	if (!::OpenThreadToken(::GetCurrentThread(), TOKEN_ALL_ACCESS, false, &service_token_handle))
	{
		std::wostringstream os;
		os << "OpenThreadToken failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return nullptr;
	}

	if (!::DuplicateTokenEx(service_token_handle, 0, NULL, SecurityDelegation, TokenImpersonation, &user_session_token_handle))
	{
		std::wostringstream os;
		os << "DuplicateTokenEx failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return nullptr;
	}

	if (!::SetTokenInformation(user_session_token_handle, TokenSessionId, &session_id, sizeof DWORD))
	{
		std::wostringstream os;
		os << "SetTokenInformation failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		::CloseHandle(user_session_token_handle);

		return nullptr;
	}

	return user_session_token_handle;
}

// Must be part of the service
DWORD CageService::StartCageManager(const std::wstring &app_name, const std::optional<std::wstring> &desktop_name, DWORD session_id)
{
	HANDLE service_token_handle;
	HANDLE user_session_token_handle;

	STARTUPINFO si = { sizeof si };
	if (desktop_name.has_value())
	{
		auto name = desktop_name.value();
		std::vector<wchar_t> desktop_name_buf(name.begin(), name.end());
		desktop_name_buf.push_back(0);
		si.lpDesktop = desktop_name_buf.data();
	}
	else
	{
		si.lpDesktop = nullptr;
	}
	PROCESS_INFORMATION pi;
	DWORD process_id = 0;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = nullptr;
	sa.bInheritHandle = true;

	// Use new token with privileges for the trusting computing base
	if (!::ImpersonateSelf(SecurityImpersonation))
	{
		std::wostringstream os;
		os << "ImpersonateSelf failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return process_id;
	}

	if (!::OpenThreadToken(::GetCurrentThread(), TOKEN_ALL_ACCESS, false, &service_token_handle))
	{
		std::wostringstream os;
		os << "OpenThreadToken failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return process_id;
	}

	if (!::DuplicateTokenEx(service_token_handle, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &user_session_token_handle))
	{
		std::wostringstream os;
		os << "DuplicateTokenEx failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return process_id;
	}

	if (!::SetTokenInformation(user_session_token_handle, TokenSessionId, &session_id, sizeof DWORD))
	{
		std::wostringstream os;
		os << "SetTokenInformation failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return process_id;
	}

	std::vector<wchar_t> app_name_buf(app_name.begin(), app_name.end());
	app_name_buf.push_back(0);
	if (!::CreateProcessAsUser(user_session_token_handle,
		app_name_buf.data(),
		NULL,
		&sa,  // <- Process Attributes
		NULL,  // Thread Attributes
		false, // Inheritaion flags
		// release build should not display console window
#ifdef _DEBUG
		0,
#else
		CREATE_NO_WINDOW,
#endif
		NULL,  // Environment
		NULL,  // Current directory
		&si,   // Startup Info
		&pi))
	{
		std::wostringstream os;
		os << "CreateProcess (" << app_name << ") failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return process_id;
	}

	process_id = pi.dwProcessId;
	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);

	return process_id;
}

void CageService::StopCageManager()
{
	// FIXME: Consider a "graceful" shutdown, i.e. sending a message/signal and then wait for the process terminating itself
	HANDLE cage_manager_handle = ::OpenProcess(PROCESS_ALL_ACCESS, TRUE, cage_manager_process_id);
	if (cage_manager_handle != NULL)
	{
		::TerminateProcess(cage_manager_handle, 55);
		std::wostringstream os;
		os << "Terminated CageManager: 55" << std::endl;
		::OutputDebugString(os.str().c_str());
	}
	else
	{
		std::wostringstream os;
		os << "Could not stop CageManager: Invalid Process Handle" << std::endl;
		::OutputDebugString(os.str().c_str());
	}

}

std::wstring CageService::GetLastErrorAsString(DWORD error_id)
{
	// Get the error message, if any.
	LPWSTR message_buffer = nullptr;
	size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_id,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		message_buffer,
		0,
		NULL);

	std::wstring message(message_buffer, size);

	// Free the buffer.
	::LocalFree(message_buffer);

	return message;
}

void CageService::HandleMessage(const std::wstring &message, NetworkManager &mgr)
{
	std::wstring message_data;
	auto parse_result = SharedFunctions::ParseMessage(message, message_data);
	if (parse_result != CageMessage::START_PROCESS)
	{
		std::wostringstream os;
		os << L"received unknown message: " << message << std::endl;
		::OutputDebugString(os.str().c_str());
		return;
	}

	CageData cage_data = { message_data };
	if (!SharedFunctions::ParseStartProcessMessage(cage_data))
	{
		std::wostringstream os;
		os << L"could not process message: " << message << std::endl;
		::OutputDebugString(os.str().c_str());
		return;
	}

	StartProcess(cage_data);
}

void CageService::StartProcess(CageData &cage_data)
{
	// Start Process
	if (cage_manager_process_id != 0)
	{
		std::wostringstream os;
		os << "Another cage instance is already running" << std::endl;
		::OutputDebugString(os.str().c_str());
		return;
	}

	SecuritySetup security_setup;
	auto security_attributes = security_setup.GetSecurityAttributes();

	if (!security_attributes.has_value())
	{
		std::wostringstream os;
		os << "Could not get security attributes" << std::endl;
		::OutputDebugString(os.str().c_str());
		return;
	}

	std::thread desktop_thread(
		&CageService::StartCage,
		this,
		security_attributes->lpSecurityDescriptor,
		cage_data
	);

	desktop_thread.join();

	// Get session id from logged on user
	//DWORD session_id = ::WTSGetActiveConsoleSessionId();
	//cage_manager_process_id = StartCageManager(session_id);

	// Forward to cage 
	//mgr.Send(message, ContextType::MANAGER);

	// Wait for the cageManager to close before receiving the next message
	// This causes that only one cageManager can run a process at a time
	//HANDLE cage_manager_handle = ::OpenProcess(SYNCHRONIZE, TRUE, cage_manager_process_id);
	//::WaitForSingleObject(cage_manager_handle, INFINITE);
	//cage_manager_process_id = 0;
}

void CageService::StartCage(PSECURITY_DESCRIPTOR security_descriptor, const CageData &cage_data)
{
	// name should be unique every time -> create UUID
	// FIXME error handling
	UUID uuid;
	::UuidCreate(&uuid);

	RPC_WSTR uuid_str;
	::UuidToString(&uuid, &uuid_str);

	std::wstring uuid_stl(reinterpret_cast<wchar_t*>(uuid_str)); // FIXME test this
	
	::RpcStringFree(&uuid_str);

	const std::wstring DESKTOP_NAME = std::wstring(L"shark_cage_desktop_").append(uuid_stl);
	const int work_area_width = 300;
	CageDesktop cage_desktop(
		security_descriptor,
		work_area_width,
		DESKTOP_NAME);

	HDESK desktop_handle;
	if (!cage_desktop.Init(desktop_handle))
	{
		std::cout << "Failed to create/launch the cage desktop" << std::endl;
		return;
	}

	//PETER´S ACCESS TOKEN THINGS

	// we need in order to create the process
	STARTUPINFO info = {};
	info.dwFlags = STARTF_USESHOWWINDOW;
	info.wShowWindow = SW_MAXIMIZE;

	// the desktop's name where we are going to start the application. In this case, our new desktop
	info.lpDesktop = const_cast<LPWSTR>(DESKTOP_NAME.c_str());

	// create the labeler process
	PROCESS_INFORMATION process_info_labeler = { 0 };
	std::wstring path_labeler = L"C:\\sharkcage\\CageLabeler.exe";
	std::vector<wchar_t> labeler_path_buf(path_labeler.begin(), path_labeler.end());
	labeler_path_buf.push_back(0);
	if (!::CreateProcess(NULL, labeler_path_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &process_info_labeler))
	{
		std::cout << "Failed to start labeler process. Error: " << ::GetLastError() << std::endl;
	}

	// create the process
	PROCESS_INFORMATION process_info = { 0 };
	std::vector<wchar_t> path_buf(cage_data.app_path.begin(), cage_data.app_path.end());
	path_buf.push_back(0);

	if (!::CreateProcess(NULL, path_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &process_info))
	{
		std::cout << "Failed to start process. Error: " << ::GetLastError() << std::endl;
	}

	PROCESS_INFORMATION process_info_additional_app = { 0 };
	if (cage_data.hasAdditionalAppInfo())
	{
		std::vector<wchar_t> additional_app_path_buf(cage_data.additional_app_path->begin(), cage_data.additional_app_path->end());
		additional_app_path_buf.push_back(0);

		if (!::CreateProcess(NULL, additional_app_path_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &process_info_additional_app))
		{
			std::cout << "Failed to start additional process. Error: " << GetLastError() << std::endl;
		}
	}

	bool keep_cage_running = true;
	std::vector<HANDLE> handles = { process_info_labeler.hProcess, process_info.hProcess };
	if (cage_data.hasAdditionalAppInfo())
	{
		handles.push_back(process_info_additional_app.hProcess);
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

		if (!keep_cage_running || (handles.size() < 2 && !cage_data.restrict_closing))
		{
			// labeler still running, tell it to shut down
			// FIXME pass this to the process
			const std::wstring LABELER_WINDOW_CLASS_NAME = L"shark_cage_token_window";
			if (keep_cage_running)
			{
				auto labeler_hwnd = ::FindWindow(LABELER_WINDOW_CLASS_NAME.c_str(), nullptr);
				if (labeler_hwnd)
				{
					::SetLastError(0);
					::PostMessage(labeler_hwnd, WM_CLOSE, NULL, NULL);
				}
				else
				{

					::SetLastError(0);
					::TerminateProcess(process_info_labeler.hProcess, 0);
				}
			}
			::WaitForSingleObject(process_info_labeler.hProcess, INFINITE);
			break;
		}
	}


	// we can't rely on the process handles to keep track of open processes on
	// the secure desktop as programs (e.g. Internet Explorer) spawn multiple processes and 
	// maybe even close the initial process we spawned ourselves
	// Solution: enumerate all top level windows on the desktop not belonging to our process and message these handles
	std::pair<DWORD, std::unordered_set<HWND>*> callback_window_data;
	std::unordered_set<HWND> window_handles_to_signal;
	callback_window_data.first = ::GetCurrentProcessId();
	callback_window_data.second = &window_handles_to_signal;

	::EnumDesktopWindows(desktop_handle, &CageService::GetOpenWindowHandles, reinterpret_cast<LPARAM>(&callback_window_data));

	for (HWND hwnd_handle : window_handles_to_signal)
	{
		::SetLastError(0);
		::PostMessage(hwnd_handle, WM_CLOSE, NULL, NULL);
	}

	// and get all open process handles we have to wait for
	std::pair<DWORD, std::unordered_set<HANDLE>*> callback_process_data;
	std::unordered_set<HANDLE> process_handles_for_closing;
	callback_process_data.first = ::GetCurrentProcessId();
	callback_process_data.second = &process_handles_for_closing;

	::EnumDesktopWindows(desktop_handle, &CageService::GetOpenProcesses, reinterpret_cast<LPARAM>(&callback_process_data));

	// give users up to 5s to react to close prompt of process, maybe increase this?
	if (::WaitForMultipleObjects(process_handles_for_closing.size(), std::vector(process_handles_for_closing.begin(), process_handles_for_closing.end()).data(), TRUE, 5000) != WAIT_OBJECT_0)
	{
		for (HANDLE process_handle : process_handles_for_closing)
		{
			::SetLastError(0);
			::TerminateProcess(process_handle, 0);
		}
	}

	// close our handles
	::CloseHandle(process_info_labeler.hProcess);
	::CloseHandle(process_info_labeler.hThread);

	::CloseHandle(process_info.hProcess);
	::CloseHandle(process_info.hThread);

	if (cage_data.hasAdditionalAppInfo())
	{
		::CloseHandle(process_info_additional_app.hProcess);
		::CloseHandle(process_info_additional_app.hThread);
	}

}

BOOL CALLBACK CageService::GetOpenProcesses(_In_ HWND hwnd, _In_ LPARAM l_param)
{
	auto data = reinterpret_cast<std::pair<DWORD, std::unordered_set<HANDLE> *> *>(l_param);
	auto current_process_id = data->first;
	auto handles = data->second;

	DWORD process_id;
	::GetWindowThreadProcessId(hwnd, &process_id);

	if (process_id != current_process_id && ::IsWindowVisible(hwnd))
	{
		::SetLastError(0);
		auto handle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
		handles->insert(handle);
	}

	return TRUE;
}

BOOL CALLBACK CageService::GetOpenWindowHandles(_In_ HWND hwnd, _In_ LPARAM l_param)
{
	auto data = reinterpret_cast<std::pair<DWORD, std::unordered_set<HWND> *> *>(l_param);
	auto current_process_id = data->first;
	auto hwnds = data->second;

	::SetLastError(0);
	DWORD process_id;
	::GetWindowThreadProcessId(hwnd, &process_id);

	if (process_id != current_process_id && ::IsWindowVisible(hwnd))
	{
		hwnds->insert(hwnd);
	}

	return TRUE;
}