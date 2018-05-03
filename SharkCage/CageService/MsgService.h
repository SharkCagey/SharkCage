#pragma once

#include <string>

enum class ServiceMessage
{
	START_CM = 0,       // Start CageManager
	STOP_CM,            // Stop CageManager
	START_PC,           // Start Process (in CageManager)
	STOP_PC            // Stop Process (in CageManager)
};

std::wstring ServiceMessageToString(ServiceMessage value)
{
	switch (value)
	{
	case ServiceMessage::START_CM:
		return L"START_CM";
	case ServiceMessage::STOP_CM:
		return L"STOP_CM";
	case ServiceMessage::START_PC:
		return L"START_PC";
	case ServiceMessage::STOP_PC:
		return L"STOP_PC";
		// no default to trigger warning if we don't cover all enum values
	}

	return L"";
}


