#pragma once

#include "CageData.h"

#include "json.hpp"

#pragma comment(lib, "netapi32.lib")

const char APPLICATION_PATH_PROPERTY[] = "application_path";
const char APPLICATION_NAME_PROPERTY[] = "application_name";
const char APPLICATION_TOKEN_PROPERTY[] = "token";
const char APPLICATION_HASH_PROPERTY[] = "binary_hash";
const char ADDITIONAL_APPLICATION_NAME_PROPERTY[] = "additional_application";
const char ADDITIONAL_APPLICATION_PATH_PROPERTY[] = "additional_application_path";
const char CLOSING_POLICY_PROPERTY[] = "restrict_closing";

template<typename T>
auto local_free_deleter = [&](T resource) { ::LocalFree(resource); };

class CageManager
{
public:
	std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> CreateSID();
	std::optional<SECURITY_ATTRIBUTES> CreateACL(std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> group_sid); 
	std::optional<ManagerMessage> ParseMessage(std::wstring &message);
	bool ParseStartProcessMessage(CageData &cage_data);
	void StartCage(PSECURITY_DESCRIPTOR security_descriptor, const CageData &cage_data);

private:
	void StartCageLabeler(
		HDESK desktop_handle,
		const CageData &cage_data,
		const int work_area_width,
		const std::wstring &labeler_window_class_name);
	static BOOL CALLBACK GetOpenWindowHandles(_In_ HWND hwnd, _In_ LPARAM l_param);
	static BOOL CALLBACK GetOpenProcesses(_In_ HWND hwnd, _In_ LPARAM l_param);
};