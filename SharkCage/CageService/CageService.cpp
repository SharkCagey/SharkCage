#include "stdafx.h"

#include <sstream>

#include "CageService.h"
#include "ValidateBinary.h"
#include "../SharedFunctionality/SharedFunctions.h"
#include "../SharedFunctionality/TokenLib/groupManipulation.h"

const std::wstring CAGE_MANAGER_NAME = L"CageManager.exe";

CageService::CageService() noexcept
	: cage_manager_process_id(0)
{
}

bool CageService::CageManagerRunning()
{
	return cage_manager_process_id > 0;
}

std::optional<HANDLE> CageService::CreateImpersonatingUserToken()
{
	DWORD session_id = ::WTSGetActiveConsoleSessionId();

	HANDLE appropriate_token = nullptr;

	// Use new token with privileges for the trusting computing base
	if (!::ImpersonateSelf(SecurityImpersonation))
	{
		std::wostringstream os;
		os << "ImpersonateSelf failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		return std::nullopt;
	}

	if (!tokenLib::aquireTokenWithPrivilegesForTokenManipulation(appropriate_token)) {
		std::wostringstream os;
		os << "The token aquisition was unsuccessfull!";
		::OutputDebugString(os.str().c_str());

		return std::nullopt;
	}

	if (!::SetTokenInformation(appropriate_token, TokenSessionId, &session_id, sizeof DWORD))
	{
		std::wostringstream os;
		os << "SetTokenInformation failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError());
		::OutputDebugString(os.str().c_str());
		::CloseHandle(appropriate_token);

		return std::nullopt;
	}

	return appropriate_token;
}

DWORD CageService::StartCageManager(DWORD session_id, HANDLE &user_token)
{
	std::vector<wchar_t> filename_buffer(MAX_PATH);
	::GetModuleFileName(nullptr, filename_buffer.data(), MAX_PATH);

	std::wstring filename(filename_buffer.data());
	auto pos = filename.rfind(L"\\");
	if (pos != std::wstring::npos)
	{
		filename = filename.substr(0, pos) + L"\\" + CAGE_MANAGER_NAME;

		ValidateBinary validator;
		if (validator.ValidateCertificate(filename))
		{
			return StartCageManager(session_id, filename, user_token);
		}
		else
		{
			// TODO write error message
			return 0;
		}
	}

	return 0;
}

DWORD CageService::StartCageManager(DWORD session_id, const std::wstring &app_name, HANDLE &user_token)
{
	return StartCageManager(session_id, app_name, std::nullopt, user_token);
}

// Must be part of the service
DWORD CageService::StartCageManager(DWORD session_id, const std::wstring &app_name, const std::optional<std::wstring> &desktop_name, HANDLE &user_token)
{
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
	auto user_session_token_handle = CreateImpersonatingUserToken();
	
	if (!user_session_token_handle.has_value())
	{
		std::wostringstream os;
		os << "Impersonating active user session failed" << std::endl;
		::OutputDebugString(os.str().c_str());
		return process_id;
	}

	user_token = user_session_token_handle.value();

	std::vector<wchar_t> app_name_buf(app_name.begin(), app_name.end());
	app_name_buf.push_back(0);
	if (!::CreateProcessAsUser(
		user_token,
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
		os << "CreateProcess (" << app_name << ") failed (" << ::GetLastError() << "): " << GetLastErrorAsString(::GetLastError()) << std::endl;
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

void CageService::HandleMessage(const std::wstring &message, NetworkManager &network_manager)
{
	std::wstring message_data;
	ContextType sender;
	auto message_type = SharedFunctions::ParseMessage(message, sender, message_data);
	if (message_type != CageMessage::START_PROCESS)
	{
		std::wostringstream os;
		os << L"received unknown message: " << message << std::endl;
		::OutputDebugString(os.str().c_str());
		return;
	}

	if (cage_manager_process_id != 0)
	{
		std::wostringstream os;
		os << L"Another cage instance is already running" << std::endl;
		::OutputDebugString(os.str().c_str());
		return;
	}

	// get session id from logged on user
	HANDLE created_token;
	DWORD session_id = ::WTSGetActiveConsoleSessionId();
	cage_manager_process_id = StartCageManager(session_id, created_token);

	// Forward to cage manager
	std::wstring result_data;
	auto send_result = network_manager.Send(ContextType::MANAGER, message_type.value(), message_data, result_data);

	if (send_result)
	{
		network_manager.Send(sender, CageMessage::RESPONSE_SUCCESS, L"", result_data);
	}
	else
	{
		network_manager.Send(sender, CageMessage::RESPONSE_FAILURE, result_data, result_data);
	}

	// wait for the cageManager to close before receiving the next message
	// this ensures only one instance of the cage desktop / manager can run simultaneously
	HANDLE cage_manager_handle = ::OpenProcess(SYNCHRONIZE, TRUE, cage_manager_process_id);
	::WaitForSingleObject(cage_manager_handle, INFINITE);

	cage_manager_process_id = 0;

	::CloseHandle(created_token);
}