#pragma once

#include "CageData.h"
#include "NetworkManager.h"
#include "Messages.h"
#include <optional>

static bool BeginsWith(const std::wstring &string_to_search, const std::wstring &prefix);
static void TrimString(std::wstring &msg);
ContextType StringToContextType(const std::wstring &type);

namespace SharedFunctions
{
	DLLEXPORT bool ParseStartProcessMessage(CageData &cage_data);
	DLLEXPORT std::optional<CageMessage> ParseMessage(const std::wstring &msg, ContextType &sender, std::wstring &message_data);
	DLLEXPORT std::wstring MessageToString(CageMessage msg);
	DLLEXPORT std::wstring ContextTypeToString(ContextType type);
	DLLEXPORT bool ValidateCertificate(const std::wstring &app_path);
	DLLEXPORT bool ValidateHash(const std::wstring &app_path, const std::wstring &app_hash);
}