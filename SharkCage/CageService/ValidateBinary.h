#pragma once
#pragma once

#include <Windows.h>
#include <string>
#include <optional>

class ValidateBinary
{
public:
	bool ValidateCertificate(const std::wstring &app_path);
	bool ValidateHash(const std::wstring &app_path, const std::wstring & app_hash);
};
