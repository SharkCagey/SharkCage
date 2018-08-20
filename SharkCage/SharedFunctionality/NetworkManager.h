#pragma once

#include "stdafx.h"

#include ".\asio\include\asio.hpp"

#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <vector>
#include <sstream>
#include <optional>
#include "Messages.h"

enum class DLLEXPORT ContextType
{
	SERVICE,
	MANAGER,
	CHOOSER,
	UNKNOWN
};

/*!
 * \brief 
 */
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
	/*!
	 * \brief Constructor.
	 * @param context type variable says in what role network manager should be initialized
	 */
	DLLEXPORT NetworkManager(ContextType context);

	/*!
	 * \brief Function to be used to send messages.
	 * @param msg the message string, max 1024 characters
	 * @param context the role of the receiving Network Manager
	 * @return true if successful
	 * @throws std::exception if GetPort doesn't find a port.
	 */
	DLLEXPORT bool Send(ContextType receiver, CageMessage message_type, const std::wstring &message_body, std::wstring& result_msg);

	/*!
	 * \brief Function used by all components to listen for messages.
	 * Uses async read and a timeout in seconds. 
	 * @param timeout_seconds how many seconds until timeout, never for less than 0
	 * @return a wstring containing the received message.
	 * @throws an std::system_error with the error code of the most recent failed async function
	 */
	DLLEXPORT std::wstring Listen(long timeout_seconds = -1);

private:
	bool Send(const std::wstring &msg, ContextType context);

	std::wstring VecToString(const std::vector<char> &message) const;

	std::vector<char> StringToVec(const std::wstring &string) const;

	std::optional<const int> (ContextType type) const
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

	ContextType receive_type;
};

// make a pinvoke callable interface which is just able to send a .config file
extern "C" DLLEXPORT bool SendConfig(const wchar_t *config_path, wchar_t *result, int result_capacity);
