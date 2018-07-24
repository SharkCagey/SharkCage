#include "stdafx.h"

#include "../SharedFunctionality/NetworkManager.h"
#include "../SharedFunctionality/SharedFunctions.h"
#include "../SharedFunctionality/CageData.h"
#include "../SharedFunctionality/tokenLib/groupManipulation.h"

#include "Aclapi.h"
#include "tlhelp32.h"

#include <unordered_set>
#include <thread>
#include "tlhelp32.h"

#include "CageManager.h"
#include "CageLabeler.h"
#include "SecuritySetup.h"
#include "CageDesktop.h"

#pragma comment(lib, "Rpcrt4.lib")

NetworkManager network_manager(ContextType::MANAGER);
std::optional<std::wstring> generateUuid();

int main()
{
	getchar();
	CageManager cage_manager;
	
	SecuritySetup security_setup;
	//randomize the group name
	auto uuid_stl_opt = generateUuid();
	if (!uuid_stl_opt.has_value()) {
		return 1;
	}
	auto uuid_stl = uuid_stl_opt.value();;

	std::wstring group_name = std::wstring(L"shark_cage_group_").append(uuid_stl);
	auto security_attributes = security_setup.GetSecurityAttributes(group_name);

	if (!security_attributes.has_value())
	{
		std::cout << "Could not get security attributes" << std::endl;
		return 1;
	}

	// listen for the message
	std::wstring message = network_manager.Listen(10);
	std::wstring message_data;
	ContextType sender;
	auto parse_result = SharedFunctions::ParseMessage(message, sender, message_data);

	if (parse_result != CageMessage::START_PROCESS)
	{
		tokenLib::deleteLocalGroup(const_cast<wchar_t*>((group_name.c_str())));
		std::cout << "Could not process incoming message" << std::endl;
		return 1;
	}

	CageData cage_data = { message_data };
	if (!SharedFunctions::ParseStartProcessMessage(cage_data))
	{
		tokenLib::deleteLocalGroup(const_cast<wchar_t*>((group_name.c_str())));
		std::cout << "Could not process start process message" << std::endl;
		return 1;
	}

	CageManager cage_manager;
	if (cage_manager.ProcessRunning(cage_data.app_path) || (cage_data.hasAdditionalAppInfo() && cage_manager.ProcessRunning(cage_data.additional_app_path.value())))
	{
		std::wstring result_data;
		network_manager.Send(sender, CageMessage::RESPONSE_FAILURE, L"One or more of the process(es) you are trying to start on the secure desktop"
			" is already running on the default desktop. Please close them and then try again.", result_data);

		return 1;
	}

	std::wstring result_data;
	network_manager.Send(sender, CageMessage::RESPONSE_SUCCESS, L"", result_data);

	SecuritySetup security_setup;
	auto security_attributes = security_setup.GetSecurityAttributes();

	if (!security_attributes.has_value())
	{
		std::cout << "Could not get security attributes" << std::endl;
		return 1;
	}

	const int work_area_width = 300;
	std::thread desktop_thread(
		&CageManager::StartCage,
		cage_manager,
		security_attributes.value(),
		cage_data,
		group_name
	);

	desktop_thread.join();
	tokenLib::deleteLocalGroup(const_cast<wchar_t*>((group_name.c_str())));
	return 0;
}

void CageManager::StartCage(SECURITY_ATTRIBUTES security_attributes, const CageData &cage_data, const std::wstring &group_name)
{
	// name should be unique every time -> create UUID
	auto uuid_stl_opt = generateUuid();
	if (!uuid_stl_opt.has_value()) {
		return;
	}
	auto uuid_stl = uuid_stl_opt.value();
	HANDLE tokenHandle = nullptr;
	//TODO: close this handle
	if (!tokenLib::constructUserTokenWithGroup(const_cast<wchar_t*>((group_name.c_str())), tokenHandle)) {
		std::cout << "Cannot create required token" << std::endl;
		return;
	}

	const std::wstring DESKTOP_NAME = std::wstring(L"shark_cage_desktop_").append(uuid_stl);
	const int work_area_width = 300;
	CageDesktop cage_desktop(
		security_attributes,
		work_area_width,
		DESKTOP_NAME);

	HDESK desktop_handle;
	if (!cage_desktop.Init(desktop_handle))
	{
		std::cout << "Failed to create/launch the cage desktop" << std::endl;
		return;
	}

	const std::wstring LABELER_WINDOW_CLASS_NAME = std::wstring(L"shark_cage_token_window_").append(uuid_stl);
	std::thread labeler_thread(
		&CageManager::StartCageLabeler,
		this,
		desktop_handle,
		cage_data,
		work_area_width,
		LABELER_WINDOW_CLASS_NAME
	);

	// We need in order to create the process.
	STARTUPINFO info = {};
	info.dwFlags = STARTF_USESHOWWINDOW;
	info.wShowWindow = SW_SHOW;

	// The desktop's name where we are going to start the application. In this case, our new desktop.
	info.lpDesktop = const_cast<LPWSTR>(DESKTOP_NAME.c_str());

	// Create the process.
	PROCESS_INFORMATION process_info = {};

	if (::CreateProcessAsUser(tokenHandle, path_buf.data(), nullptr, &security_attributes, nullptr, false, 0, nullptr, nullptr, &info, &process_info) == 0)
	{
		std::cout << "Failed to start process. Error: " << ::GetLastError() << std::endl;
	}

	PROCESS_INFORMATION process_info_additional_app = { 0 };
	if (cage_data.hasAdditionalAppInfo())
	{
		std::vector<wchar_t> additional_app_path_buf(cage_data.additional_app_path->begin(), cage_data.additional_app_path->end());
		additional_app_path_buf.push_back(0);
		if(::CreateProcessAsUser(tokenHandle, additional_app_path_buf.data(), nullptr, &security_attributes, nullptr, FALSE, 0, nullptr, nullptr, &info, &process_info_additional_app) == 0)
		{
			std::cout << "Failed to start additional process. Error: " << GetLastError() << std::endl;
		}
	}

	bool keep_cage_running = true;

	std::vector<HANDLE> handles = { labeler_thread.native_handle(), process_info.hProcess };
	std::vector<HANDLE> wait_handles;

	wait_handles.push_back(cage_data.activate_app);

	if (cage_data.hasAdditionalAppInfo())
	{
		handles.push_back(process_info_additional_app.hProcess);
		wait_handles.push_back(cage_data.activate_additional_app.value());
	}

	for (auto handle : wait_handles)
	{
		handles.push_back(handle);
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
				// check all other handles
				for (size_t i = 1; i < handles.size(); ++i)
				{
					if (res == WAIT_OBJECT_0 + i)
					{
						// get the handle we got an event for
						auto iterator = handles.begin();
						std::advance(iterator, i);

						if (*iterator == cage_data.activate_app)
						{
							CageManager::ActivateApp(
								cage_data.app_path,
								cage_data.activate_app,
								desktop_handle,
								process_info,
								security_attributes,
								info,
								handles);
						}
						else if (cage_data.hasAdditionalAppInfo() && *iterator == cage_data.activate_additional_app.value())
						{
							CageManager::ActivateApp(
								cage_data.additional_app_path.value(),
								cage_data.activate_additional_app.value(),
								desktop_handle,
								process_info_additional_app,
								security_attributes,
								info,
								handles);
						}
						else
						{
							// remove the handle we got an event for
							handles.erase(iterator);
						}
					}
				}
			}
		}

		const int running_processes_to_keep_cage_alive = 2; // labeler + app or additional app
		if (!keep_cage_running || (handles.size() < running_processes_to_keep_cage_alive + wait_handles.size() && !cage_data.restrict_closing))
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
	// Solution: enumerate all top level windows on the desktop not belonging to our
	// process and message these handles
	std::pair<DWORD, std::unordered_set<HWND>*> callback_window_data;
	std::unordered_set<HWND> window_handles_to_signal;
	callback_window_data.first = ::GetCurrentProcessId();
	callback_window_data.second = &window_handles_to_signal;

	::EnumDesktopWindows(
		desktop_handle,
		&CageManager::GetOpenWindowHandles,
		reinterpret_cast<LPARAM>(&callback_window_data)
	);

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

	::EnumDesktopWindows(
		desktop_handle,
		&CageManager::GetOpenProcesses,
		reinterpret_cast<LPARAM>(&callback_process_data)
	);

	// give users up to 5s to react to close prompt of process, maybe increase this?
	if (::WaitForMultipleObjects(
		process_handles_for_closing.size(),
		std::vector(process_handles_for_closing.begin(), process_handles_for_closing.end()).data(),
		TRUE,
		5000) != WAIT_OBJECT_0)
	{
		for (HANDLE process_handle : process_handles_for_closing)
		{
			::SetLastError(0);
			::TerminateProcess(process_handle, 0);
		}
	}

	// close our handles
	::CloseHandle(process_info.hProcess);
	::CloseHandle(process_info.hThread);
	::CloseHandle(cage_data.activate_app);

	if (cage_data.hasAdditionalAppInfo())
	{
		::CloseHandle(process_info_additional_app.hProcess);
		::CloseHandle(process_info_additional_app.hThread);
		::CloseHandle(cage_data.activate_additional_app.value());
	}
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

BOOL CALLBACK CageManager::GetOpenProcesses(_In_ HWND hwnd, _In_ LPARAM l_param)
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

BOOL CALLBACK CageManager::GetOpenWindowHandles(_In_ HWND hwnd, _In_ LPARAM l_param)
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

BOOL CALLBACK CageManager::ActivateProcess(_In_ HWND hwnd, _In_ LPARAM l_param)
{
	auto data = reinterpret_cast<DWORD*>(l_param);
	auto current_process_id = *data;

	::SetLastError(0);
	DWORD process_id;
	::GetWindowThreadProcessId(hwnd, &process_id);

	if (process_id == current_process_id && ::IsWindowVisible(hwnd))
	{
		if (::IsIconic(hwnd))
		{
			::ShowWindow(hwnd, SW_RESTORE);
		}
		else
		{
			::SetForegroundWindow(hwnd);
		}
	}

	return TRUE;
}

bool CageManager::ProcessRunning(const std::wstring &process_path)
{
	bool app_running = false;

	HANDLE process_snapshot;
	PROCESSENTRY32 process_entry;
	process_snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (process_snapshot != INVALID_HANDLE_VALUE)
	{
		process_entry.dwSize = sizeof(PROCESSENTRY32);
		if (::Process32First(process_snapshot, &process_entry))
		{
			std::wstring comparison(process_entry.szExeFile);

			if (process_path.find(comparison) != std::wstring::npos)
			{
				app_running = true;
			}

			while (!app_running && ::Process32Next(process_snapshot, &process_entry))
			{
				comparison = std::wstring(process_entry.szExeFile);
				if (process_path.find(comparison) != std::wstring::npos)
				{
					app_running = true;
				}
			}

			::CloseHandle(process_snapshot);
		}
	}

	return app_running;
}

void CageManager::ActivateApp(
	const std::wstring &path,
	const HANDLE &event,
	const HDESK &desktop_handle,
	PROCESS_INFORMATION &process_info,
	SECURITY_ATTRIBUTES security_attributes,
	STARTUPINFO info,
	std::vector<HANDLE> &handles)
{
	std::cout << "restart app" << std::endl;

	if (!::ResetEvent(event))
	{
		std::cout << "Reset event failed. Error: " << ::GetLastError() << std::endl;
	}

	if (CageManager::ProcessRunning(path))
	{
		::EnumDesktopWindows(
			desktop_handle,
			&CageManager::ActivateProcess,
			reinterpret_cast<LPARAM>(&process_info.dwProcessId)
		);
	}
	else
	{
		::CloseHandle(process_info.hProcess);
		::CloseHandle(process_info.hThread);

		if (::CreateProcess(
			path.c_str(),
			nullptr,
			&security_attributes,
			nullptr,
			FALSE,
			0,
			nullptr,
			nullptr,
			&info,
			&process_info) == 0)
		{
			std::cout << "Failed to start process. Error: " << ::GetLastError() << std::endl;
		}
		else
		{
			handles.push_back(process_info.hProcess);
		}
	}
}

std::optional<std::wstring> generateUuid() {
	UUID uuid;
	if (::UuidCreate(&uuid) != RPC_S_OK)
	{
		std::cout << "Failed to create UUID" << std::endl;
		return std::nullopt;
	}

	RPC_WSTR uuid_str;
	if (::UuidToString(&uuid, &uuid_str) != RPC_S_OK)
	{
		std::cout << "Failed to convert UUID to rpc string" << std::endl;
		return std::nullopt;
	}

	std::wstring uuid_stl(reinterpret_cast<wchar_t*>(uuid_str));
	if (uuid_stl.empty())
	{
		::RpcStringFree(&uuid_str);
		std::cout << "Failed to convert UUID rpc string to stl string" << std::endl;
		return std::nullopt;
	}

	::RpcStringFree(&uuid_str);
	return uuid_stl;
}
