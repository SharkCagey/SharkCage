#pragma once

#include <string>

enum class ServiceMessage
{
	// Start CageManager
	START_CM = 0,
	// Stop CageManager
	STOP_CM,
	// Start Process (in CageManager)
	START_PC,
	// Stop Process (in CageManager)
	STOP_PC
};

DLLEXPORT std::wstring ServiceMessageToString(ServiceMessage value);
