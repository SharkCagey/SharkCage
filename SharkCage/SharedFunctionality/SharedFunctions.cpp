#include "stdafx.h"
#include "ValidateBinary.h"

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

void TrimString(std::wstring &str)
{
	// trim whitespace at beginning
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](wchar_t c)
	{
		return !std::iswspace(c);
	}));

	// trim whitespace at end
	str.erase(std::find_if(str.rbegin(), str.rend(), [](wchar_t c)
	{
		return !std::iswspace(c);
	}).base(), str.end());
}

bool BeginsWith(const std::wstring &string_to_search, const std::wstring &prefix)
{
	if (prefix.length() > string_to_search.length())
	{
		return false;
	}
	else
	{
		return string_to_search.compare(0, prefix.length(), prefix) == 0;
	}
}

ContextType StringToContextType(const std::wstring &type)
{
	std::wstring service(L"SERVICE");
	std::wstring manager(L"MANAGER");
	std::wstring chooser(L"CHOOSER");

	if (type.compare(0, service.length(), service) == 0)
	{
		return ContextType::SERVICE;
	}

	if (type.compare(0, manager.length(), manager) == 0)
	{
		return ContextType::MANAGER;
	}

	if (type.compare(0, chooser.length(), chooser) == 0)
	{
		return ContextType::CHOOSER;
	}

	return ContextType::UNKNOWN;
}

namespace SharedFunctions
{
	DLLEXPORT std::optional<CageMessage> ParseMessage(const std::wstring &msg, ContextType &sender, std::wstring &message_data)
	{
		auto first_delimiter = msg.find('|');
		if (first_delimiter == std::wstring::npos)
		{
			return std::nullopt;
		}
	
		auto message_sender = msg.substr(0, first_delimiter);
		sender = StringToContextType(message_sender);

		auto message_body = msg.substr(first_delimiter + 1);

		auto SplitMessage = [](const std::wstring &msg_to_split) -> std::wstring
		{
			auto delim = msg_to_split.find('|');
			if (delim == std::wstring::npos)
			{
				return L"";
			}

			auto msg_data = msg_to_split.substr(delim + 1);
			TrimString(msg_data);
			return msg_data;
		};

		if (BeginsWith(message_body, MessageToString(CageMessage::START_PROCESS)))
		{
			// read config
			message_data = SplitMessage(message_body);

			return CageMessage::START_PROCESS;
		}
		else if (BeginsWith(message_body, MessageToString(CageMessage::RESPONSE_SUCCESS)))
		{
			return CageMessage::RESPONSE_SUCCESS;
		}
		else if (BeginsWith(message_body, MessageToString(CageMessage::RESPONSE_FAILURE)))
		{
			// read error
			message_data = SplitMessage(message_body);

			return CageMessage::RESPONSE_FAILURE;
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

				cage_data.activate_app = ::CreateEvent(
					nullptr,
					TRUE,
					FALSE,
					L"Sharkcage_ActivateMainApp"
				);

				if (!cage_data.activate_app)
				{
					std::cout << "Create restart event for app failed. Error: " << GetLastError() << std::endl;
					return false;
				}

				if (!cage_data.hasAdditionalAppInfo() || cage_data.additional_app_name->compare(L"None") == 0)
				{
					cage_data.additional_app_name.reset();
					cage_data.additional_app_path.reset();
				}
				else
				{
					cage_data.activate_additional_app = ::CreateEvent(
						nullptr,
						TRUE,
						FALSE,
						L"Sharkcage_ActivateAdditionalApp"
					);

					if (!cage_data.activate_additional_app.value())
					{
						std::cout << "Create restart event for additional app failed. Error: " << GetLastError() << std::endl;
						return false;
					}
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
		case CageMessage::RESPONSE_SUCCESS:
			return L"RESPONSE_SUCCESS";
		case CageMessage::RESPONSE_FAILURE:
			return L"RESPONSE_FAILURE";
		}

		return L"";
	}

	DLLEXPORT std::wstring ContextTypeToString(ContextType type)
	{
		switch (type)
		{
		case ContextType::SERVICE:
			return L"SERVICE";
		case ContextType::MANAGER:
			return L"MANAGER";
		case ContextType::CHOOSER:
			return L"CHOOSER";
		}

		return L"UNKNOWN";
	}

	DLLEXPORT bool ValidateCertificate(const std::wstring &app_path)
	{
		return ValidateBinary::ValidateCertificate(app_path);
	}


	DLLEXPORT bool ValidateHash(const std::wstring &app_path, const std::wstring &app_hash)
	{
		return ValidateBinary::ValidateHash(app_path, app_hash);
	}
}