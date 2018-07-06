
#include "stdafx.h"

#include "MsgService.h"

DLLEXPORT std::wstring ServiceMessageToString(ServiceMessage value)
{
	switch (value)
	{
	case ServiceMessage::START_CM:
		return L"START_CM";
	case ServiceMessage::START_PC:
		return L"START_PC";
		// no default to trigger warning if we don't cover all enum values
	}

	return L"";
}