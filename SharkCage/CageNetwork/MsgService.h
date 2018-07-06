#pragma once

#include <string>

enum class ServiceMessage
{
	// Start CageManager
	START_CM = 0,
	// Start Process (in CageManager)
	START_PC,
};

DLLEXPORT std::wstring ServiceMessageToString(ServiceMessage value);
