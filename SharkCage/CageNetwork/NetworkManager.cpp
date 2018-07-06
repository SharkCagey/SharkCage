#pragma once

#include "stdafx.h"

#include "NetworkManager.h"
#include "MsgService.h"

#include <chrono>

DLLEXPORT NetworkManager::NetworkManager(ContextType receive_for_type)
{	
	auto receive_port = GetPort(receive_for_type);

	if (!receive_port.has_value())
	{
		throw std::exception("Failed to initialize network manager");
	}

	acceptor = std::make_unique<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), receive_port.value()));
	acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
}

DLLEXPORT bool NetworkManager::Send(const std::wstring &msg, ContextType send_to_type)
{
	io_context.restart();

	std::vector<char> message = StringToVec(msg);

	try
	{
		auto send_port = GetPort(send_to_type);
		if (!send_port.has_value())
		{
			throw std::exception("Unknown receiver, send failed");
		}

		tcp::resolver resolver(io_context);
		tcp::resolver::query query(tcp::v4(), "localhost", std::to_string(send_port.value()));
		tcp::endpoint send_endpoint = *resolver.resolve(query);

		tcp::socket socket(io_context);
		socket.connect(send_endpoint);
		send_buf = message;
		socket.write_some(asio::buffer(send_buf));
		return true;
	}
	catch (std::system_error e)
	{
		std::wostringstream os;
		os << e.what();
		::OutputDebugString(os.str().c_str());
	}

	return false;
}

DLLEXPORT std::wstring NetworkManager::Listen(long timeout_seconds)
{
	io_context.restart();

	tcp::socket socket(io_context);
	asio::streambuf buffer;

	try
	{
		asio::error_code err_code;
		auto async_read_handler = [&err_code](const asio::error_code& ec, size_t)
		{
			err_code = ec;
		};

		auto async_accept_handler = [&socket, &buffer, &err_code, &async_read_handler](const asio::error_code& ec)
		{
			err_code = ec;
			if (!ec)
			{
				asio::async_read_until(socket, buffer, '\n', async_read_handler);
			}
		};

		acceptor->async_accept(socket, async_accept_handler);

		if (timeout_seconds < 0)
		{
			io_context.run();
		}
		else
		{
			io_context.run_for(std::chrono::seconds::duration(timeout_seconds));

			if (!io_context.stopped())
			{
				acceptor->cancel();
				io_context.run();
			}
		}

		if (err_code)
		{
			throw std::system_error(err_code);
		}
	}
	catch (std::system_error e)
	{
		std::wostringstream os;
		os << e.what();
		::OutputDebugString(os.str().c_str());
	}

	std::istream istr(&buffer);
	std::string narrow_string;
	std::getline(istr, narrow_string);

	// no suitable alternative in c++ standard yet, so it is safe to use for now
	// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(narrow_string);
}

std::wstring NetworkManager::VecToString(const std::vector<char> &message) const
{
	// no suitable alternative in c++ standard yet, so it is safe to use for now
	// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(message.data());
}

std::vector<char> NetworkManager::StringToVec(const std::wstring &string) const
{
	// no suitable alternative in c++ standard yet, so it is safe to use for now
	// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	auto narrow_string = converter.to_bytes(string);
	std::vector<char> message(narrow_string.begin(), narrow_string.end());
	message.push_back(0);

	return message;
}

extern "C" DLLEXPORT void SendConfigAndExternalProgram(const wchar_t *config_path)
{
	if (config_path)
	{
		std::wstring config(config_path);

		std::wostringstream ss;
		ss << ServiceMessageToString(ServiceMessage::START_PC) << " " << config_path;

		NetworkManager mgr(ContextType::CHOOSER);
		mgr.Send(ss.str(), ContextType::SERVICE);
	}
}

// FIXME this method should be deleted after #21 is solved
extern "C" DLLEXPORT void StartCageManager()
{
	NetworkManager mgr(ContextType::CHOOSER);
	mgr.Send(ServiceMessageToString(ServiceMessage::START_CM), ContextType::SERVICE);
}