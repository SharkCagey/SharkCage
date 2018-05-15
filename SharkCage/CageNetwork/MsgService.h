#pragma once

#include <string>

enum class ServiceMessage
{
	START_CM = 0,       // Start CageManager
	STOP_CM,            // Stop CageManager
	START_PC,           // Start Process (in CageManager)
	STOP_PC				// Stop Process (in CageManager)
};

DLLEXPORT std::wstring ServiceMessageToString(ServiceMessage value);
