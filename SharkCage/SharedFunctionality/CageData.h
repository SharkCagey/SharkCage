#pragma once

#include <string>
#include <optional>

struct CageData
{
	const std::wstring config_path;
	std::wstring app_path;
	std::wstring app_name;
	std::wstring app_token;
	std::wstring app_hash;
	std::optional<std::wstring> additional_app_name;
	std::optional<std::wstring> additional_app_path;
	bool restrict_closing;

	HANDLE activate_app;
	std::optional<HANDLE> activate_additional_app;

	bool hasAdditionalAppInfo() const
	{
		return additional_app_name.has_value() && additional_app_path.has_value();
	}
};