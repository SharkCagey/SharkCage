#pragma once

#include <string>

enum class ManagerMessage
{
	START_PROCESS = 0,
	// Start Process (in CageManager)
	STOP_PROCESS,
	// Stop Process (in CageManager)
	RESTART_MAIN_APP,
	// Restart main application
	RESTART_ADDITIONAL_APP
	// Restart the additional app
};

DLLEXPORT std::wstring ManagerMessageToString(ManagerMessage value);
