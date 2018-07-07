#pragma once

#include "../SharedFunctionality/NetworkManager.h"
#include "../SharedFunctionality/CageData.h"

#include <Windows.h>
#include <string>
#include <optional>

class CageService
{
public:
	CageService() noexcept;

	bool CageManagerRunning();
	/*
	* Starts the Cage Manager in a new process on the normal desktop in the given session.
	*
	* @session_id The sessionId of the user.
	* @return The process ID of the started process.
	*/
	DWORD StartCageManager(DWORD session_id);

	/*
	 * Starts the executable in a new process on the normal desktop in the given session.
	 *
	 * @app_name The path to the execuatble.
	 * @session_id The sessionId of the user.
	 * @return The process ID of the started process.
	 */
	DWORD StartCageManager(const std::wstring &app_name, DWORD session_id);

	/*
	 * Starts the executable in a new process on the respective desktop in the given session.
	 *
	 * @app_name The path to the execuatble.
	 * @desktop_name The name of the desktop to start the program in.
	 * @session_id The sessionId of the user.
	 * @return The process ID of the started process.
	 */
	DWORD StartCageManager(const std::wstring &app_name, const std::optional<std::wstring> &desktop_name, DWORD session_id);

	void StopCageManager();

	/*
	 * Parses a message and does the action according to content of the message.
	 * The message must be in the form of:
	 * "MSG_TO_SERVICE.constant absolute/path/to/executable"
	 */
	void HandleMessage(const std::wstring &message, NetworkManager &mgr);

private:
	// Process ID of the Cage Manager (Used for closing the Cage Manager)
	DWORD cage_manager_process_id;

	std::wstring GetLastErrorAsString(DWORD error_id);

	void StartProcess(CageData &cage_data);
	void StartCage(PSECURITY_DESCRIPTOR security_descriptor, const CageData &cage_data);

	std::optional<HANDLE> CageService::CreateImpersonatingUserToken();

	// FIXME extra class for this? process handling? -> could also do the wait stuff
	static BOOL CALLBACK GetOpenWindowHandles(_In_ HWND hwnd, _In_ LPARAM l_param);
	static BOOL CALLBACK GetOpenProcesses(_In_ HWND hwnd, _In_ LPARAM l_param);
};
