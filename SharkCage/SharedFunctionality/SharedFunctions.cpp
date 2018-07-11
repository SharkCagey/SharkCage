
#include "stdafx.h"

#include "SharedFunctions.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <codecvt>
#include <cwctype>

const char APPLICATION_PATH_PROPERTY[] = "application_path";
const char APPLICATION_NAME_PROPERTY[] = "application_name";
const char APPLICATION_TOKEN_PROPERTY[] = "token";
const char APPLICATION_HASH_PROPERTY[] = "binary_hash";
const char ADDITIONAL_APPLICATION_NAME_PROPERTY[] = "additional_application";
const char ADDITIONAL_APPLICATION_PATH_PROPERTY[] = "additional_application_path";
const char CLOSING_POLICY_PROPERTY[] = "restrict_closing";

void TrimMessage(std::wstring &msg)
{
	// trim whitespace at beginning
	msg.erase(msg.begin(), std::find_if(msg.begin(), msg.end(), [](wchar_t c)
	{
		return !std::iswspace(c);
	}));

	// trim whitespace at end
	msg.erase(std::find_if(msg.rbegin(), msg.rend(), [](wchar_t c)
	{
		return !std::iswspace(c);
	}).base(), msg.end());
}

bool BeginsWith(const std::wstring &string_to_search, const std::wstring &prefix)
{
	if (prefix.length() > string_to_search.length())
	{
		throw std::invalid_argument("prefix longer than the actual string");
	}
	else
	{
		return string_to_search.compare(0, prefix.length(), prefix) == 0;
	}
}

namespace SharedFunctions
{
	DLLEXPORT std::optional<CageMessage> ParseMessage(const std::wstring &msg, std::wstring &message_data)
	{
		if (BeginsWith(msg, MessageToString(CageMessage::START_PROCESS)))
		{
			// read config
			auto message_cmd_length = MessageToString(CageMessage::START_PROCESS).length();
			message_data = msg.substr(message_cmd_length);

			TrimMessage(message_data);

			return CageMessage::START_PROCESS;
		}
		else
		{
			std::wcout << "Received unrecognized message: " << msg << std::endl;
		}

		return std::nullopt;
	}

	DLLEXPORT bool ParseStartProcessMessage(CageData &cage_data)
	{
		std::ifstream config_stream;
		config_stream.open(cage_data.config_path);

		if (config_stream.is_open())
		{
			try
			{
				nlohmann::json json_config;
				config_stream >> json_config;

				auto path = json_config[APPLICATION_PATH_PROPERTY].get<std::string>();

				auto application_name_json = json_config[APPLICATION_NAME_PROPERTY];
				std::string application_name = "Name not available";
				if (!application_name_json.is_null())
				{
					application_name = application_name_json.get<std::string>();
				}

				auto token = json_config[APPLICATION_TOKEN_PROPERTY].get<std::string>();
				auto hash = json_config[APPLICATION_HASH_PROPERTY].get<std::string>();
				auto additional_application = json_config[ADDITIONAL_APPLICATION_NAME_PROPERTY].get<std::string>();
				auto additional_application_path = json_config[ADDITIONAL_APPLICATION_PATH_PROPERTY].get<std::string>();
				auto restrict_closing = json_config[CLOSING_POLICY_PROPERTY].get<bool>();

				// no suitable alternative in c++ standard yet, so it is safe to use for now
				// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

				cage_data.app_path = converter.from_bytes(path);
				cage_data.app_name = converter.from_bytes(application_name);
				cage_data.app_token = converter.from_bytes(token);
				cage_data.app_hash = converter.from_bytes(hash);
				cage_data.additional_app_name = converter.from_bytes(additional_application);
				cage_data.additional_app_path = converter.from_bytes(additional_application_path);
				cage_data.restrict_closing = restrict_closing;

				if (!cage_data.hasAdditionalAppInfo() || cage_data.additional_app_name->compare(L"None") == 0)
				{
					cage_data.additional_app_name.reset();
					cage_data.additional_app_path.reset();
				}

				return true;
			}
			catch (nlohmann::json::exception& e)
			{
				std::cout << "Could not parse json: " << e.what() << std::endl;
				return false;
			}
			catch (std::exception e)
			{
				std::cout << "Could not parse json: " << e.what() << std::endl;
				return false;
			}
		}

		return false;
	}

	DLLEXPORT std::wstring MessageToString(CageMessage msg)
	{
		switch (msg)
		{
		case CageMessage::START_PROCESS:
			return L"START_PROCESS";
		}

		return L"";
	}
}