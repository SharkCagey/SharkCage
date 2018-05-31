#pragma once

#include <string>

enum class ManagerMessage
{
	START_PROCESS = 0,       // Start Process (in CageManager)
	STOP_PROCESS,            // Stop Process (in CageManager)
};

DLLEXPORT std::wstring ManagerMessageToString(ManagerMessage value);
