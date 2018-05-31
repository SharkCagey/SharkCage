
#include "stdafx.h"

#include "MsgManager.h"

DLLEXPORT std::wstring ManagerMessageToString(ManagerMessage value)
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