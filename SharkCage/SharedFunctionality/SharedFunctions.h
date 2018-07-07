#pragma once

#include "CageData.h"
#include "Messages.h"
#include <optional>

static bool BeginsWith(const std::wstring &string_to_search, const std::wstring &prefix);
static void TrimMessage(std::wstring &msg);

namespace SharedFunctions
{
	DLLEXPORT bool ParseStartProcessMessage(CageData &cage_data);
	DLLEXPORT std::optional<CageMessage> ParseMessage(const std::wstring &msg, std::wstring &message_data);
	DLLEXPORT std::wstring MessageToString(CageMessage msg);
}