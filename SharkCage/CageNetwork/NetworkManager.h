#pragma once

// FIXME use nuget package instead
#include ".\asio\include\asio.hpp"

#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <vector>
#include <sstream>
#include <optional>

enum class DLLEXPORT ContextType
{
	SERVICE,
	MANAGER,
	CHOOSER
};

class NetworkManager
{
	using tcp = asio::ip::tcp;

private:
	asio::io_service io_context;
	std::unique_ptr<tcp::acceptor> acceptor;

	std::vector<char> send_buf;

	const int LISTEN_PORT_SERVICE = 51234;
	const int LISTEN_PORT_MANAGER = 51235;
	const int LISTEN_PORT_CHOOSER = 51236;

public:
	/*
	* Constructor
	* type variable says in what role network manager should be initialized
	*/
	DLLEXPORT NetworkManager(ContextType context);

	/**
	* Function to be used to send messages
	* max message lenght is 1024 characters
	*
	**/
	DLLEXPORT bool Send(const std::wstring &msg, ContextType context);

	/**
	* function used by all components to listen for messages
	*
	**/
	DLLEXPORT std::wstring Listen(long timeout_seconds = -1);

private:
	std::wstring VecToString(const std::vector<char> &message) const;

	std::vector<char> StringToVec(const std::wstring &string) const;

	std::optional<const int> GetPort(ContextType type) const
	{
		switch (type)
		{
		case ContextType::SERVICE:
			return LISTEN_PORT_SERVICE;
		case ContextType::MANAGER:
			return LISTEN_PORT_MANAGER;
		case ContextType::CHOOSER:
			return LISTEN_PORT_CHOOSER;
		default:
			return std::nullopt;
		}
	}
};

// make a pinvoke callable interface which is just able to send
// a .config file + path to external program (like keepass)
extern "C" DLLEXPORT void SendConfigAndExternalProgram(const wchar_t *config_path);

// make a pinvoke callable interface which is just able to tell the service to
// start the cage manager (after #21: no longer necessary as manager only shows ínformation on new desktop and gets started implicitely)
extern "C" DLLEXPORT void StartCageManager();