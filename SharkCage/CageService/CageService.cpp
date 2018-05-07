#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <tchar.h>
#include <cassert>

#include "CageService.h"
#include "MsgService.h"

CageService::CageService()
{
	cage_manager_process_id = 0;
	image_index = -1;
}

bool CageService::CageManagerRunning()
{
	return cage_manager_process_id > 0;
}

DWORD CageService::StartCageManager(DWORD session_id)
{
	std::wstring cage_manager_path = L"C:\\sharkcage\\CageManager.exe";
	return StartCageManager(cage_manager_path, session_id);
}


DWORD CageService::StartCageManager(const std::wstring &app_name, DWORD session_id)
{
	return StartCageManager(app_name, std::nullopt, session_id);
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
	DWORD process_id = -1;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = nullptr;
	sa.bInheritHandle = true;

	// Use nwe token with privileges for the trudting vomputinh base
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
		0,     // Creation flags
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
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_id,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&message_buffer,
		0,
		NULL);

	std::wstring message(message_buffer, size);

	// Free the buffer.
	::LocalFree(message_buffer);

	return message;
}

void CageService::HandleMessage(const std::wstring &message, NetworkManager* mgr)
{
	if (BeginsWith(message, ServiceMessageToString(ServiceMessage::START_CM)))
	{
		// Start Process
		if (cage_manager_process_id == 0)
		{
			// Get session id from loged on user
			DWORD session_id = ::WTSGetActiveConsoleSessionId();
			cage_manager_process_id = StartCageManager(session_id);
		}
	}
	else if (BeginsWith(message, ServiceMessageToString(ServiceMessage::STOP_CM)))
	{
		// Stop Process
		StopCageManager();
		cage_manager_process_id = 0;
	}
	else if (BeginsWith(message, ServiceMessageToString(ServiceMessage::START_PC)))
	{
		// Forward to cage manager
		mgr->Send(message);

		// Wait for the cageManager to close before receiving the next message
		// This causes that only one cageManager can run a process at a time
		HANDLE cage_manager_handle = ::OpenProcess(SYNCHRONIZE, TRUE, cage_manager_process_id);
		::WaitForSingleObject(cage_manager_handle, INFINITE);
		cage_manager_process_id = 0;

	}
	else if (BeginsWith(message, ServiceMessageToString(ServiceMessage::STOP_PC)))
	{
		// Forward to cage manager
		mgr->Send(message);
	}
	else
	{
		std::wostringstream os;
		os << L"received unknown message: " << message << std::endl;
		::OutputDebugString(os.str().c_str());
	}
}


bool CageService::BeginsWith(const std::wstring &string, const std::wstring &prefix)
{
	if (prefix.length() > string.length())
	{
		return false;
		// Throw Exception "Bad parameters: prefix longer than the actual string"
	}
	else
	{
		return string.compare(0, prefix.length(), prefix) == 0;
	}
}


void CageService::ReadConfigFile()
{
	std::wstring config_file_name = L"C:\\sharkcage\\config.txt";
	std::wifstream config_stream{ config_file_name };

	std::wstring line;
	if (config_stream.is_open())
	{
		std::getline(config_stream, line);

		if (BeginsWith(line, L"picture:"))
		{
			image_index = GetPictureIndexFromLine(line);
		}
	}
	else
	{
		std::wostringstream os;
		os << L"Could not open file for reading: " << config_file_name;
		::OutputDebugString(os.str().c_str());
	}
}

int CageService::GetPictureIndexFromLine(const std::wstring &line)
{
	const int PICTURE_LENGTH = 8;
	assert(line.length() > PICTURE_LENGTH);

	const int length = line.length() - PICTURE_LENGTH;
	std::wstring number_string = line.substr(PICTURE_LENGTH, length);

	return std::stoi(number_string);
}


int CageService::GetImageIndex()
{
	if (image_index < 0)
	{
		// Show Dialog
		dialog_process_id = StartCageManager(L"C:\\sharkcage\\ImageSelectDialog.exe", ::WTSGetActiveConsoleSessionId());
		// Wait for the dialog to be closed
		HANDLE dialog_handle = OpenProcess(SYNCHRONIZE, TRUE, dialog_process_id);
		WaitForSingleObject(dialog_handle, INFINITE);

		// Read config file
		ReadConfigFile();
	}
	return image_index;
}

