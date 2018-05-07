#pragma once

#include <string>

enum class ManagerMessage
{
	START_PROCESS = 0,       // Start Process (in CageManager)
	STOP_PROCESS,            // Stop Process (in CageManager)
};

std::wstring ManagerMessageToString(ManagerMessage value)
{
	switch (value)
	{
	case ManagerMessage::START_PROCESS:
		return L"START_PC";
	case ManagerMessage::STOP_PROCESS:
		return L"STOP_PC";
		// no default to trigger warning if we don't cover all enum values
	}

	return L"";
}