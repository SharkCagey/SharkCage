#pragma once

#include "stdafx.h"

#include "NetworkManager.h"
#include "MsgService.h"


DLLEXPORT NetworkManager::NetworkManager(ExecutableType type) : io_service(), socket(io_service), acceptor(io_service, tcp::endpoint(tcp::v4(), 1338))
{
	switch (type)
	{
	case ExecutableType::UI:
		InitUi();
		break;
	case ExecutableType::SERVICE:
		InitService();
		break;
	case ExecutableType::MANAGER:
		InitManager();
		break;
	default:
		break;
	}
}

DLLEXPORT std::wstring NetworkManager::Receive()
{
	rec_buf.clear();
	rec_buf.resize(1024);
	try
	{
		size_t len = socket.receive(asio::buffer(rec_buf));
		rec_buf.resize(len);
	}
	catch (std::system_error e)
	{
		std::wostringstream os;
		os << e.what();
		::OutputDebugString(os.str().c_str());
	}
	return ToString(rec_buf);
}


DLLEXPORT bool NetworkManager::Send(const std::wstring &msg)
{
	std::vector<char> message = ToCharVector(msg);

	try
	{
		tcp::socket tmp_socket(io_service);
		tmp_socket.connect(send_endpoint);
		send_buf = message;
		tmp_socket.write_some(asio::buffer(send_buf));
		tmp_socket.close();
		return true;
	}
	catch (std::system_error e)
	{
		std::wostringstream os;
		os << e.what();
		::OutputDebugString(os.str().c_str());
		return false;
	}
}

DLLEXPORT std::wstring NetworkManager::Listen()
{
	tcp::socket temp_socket(io_service);
	acceptor.accept(temp_socket);

	asio::streambuf buffer;

	try
	{
		size_t len = asio::read_until(temp_socket, buffer, '\n');
	}
	catch (std::system_error e)
	{
		std::wostringstream os;
		os << e.what();
		::OutputDebugString(os.str().c_str());
	}

	std::istream str(&buffer);
	std::string narrow_string;
	std::getline(str, narrow_string);

	// no suitable alternative in c++ standard yet, so it is safe to use for now
	// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(narrow_string);
}

// ports lisened to: ui 1337, service 1338, manager 1339
bool NetworkManager::InitUi()
{
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(tcp::v4(), "localhost", "1338");
	send_endpoint = *resolver.resolve(query);

	return true;
}

bool NetworkManager::InitService()
{
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(tcp::v4(), "localhost", "1339");
	send_endpoint = *resolver.resolve(query);

	acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	acceptor = tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), 1338));

	return true;
}

bool NetworkManager::InitManager()
{
	acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	acceptor = tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), 1339));

	return true;
}

std::wstring NetworkManager::ToString(const std::vector<char> &message)
{
	// no suitable alternative in c++ standard yet, so it is safe to use for now
	// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(message.data());
}

std::vector<char> NetworkManager::ToCharVector(const std::wstring &string)
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

		NetworkManager mgr(ExecutableType::UI);
		mgr.Send(ss.str());
	}
}

// FIXME this method should be deleted after #21 is solved
extern "C" DLLEXPORT void StartCageManager()
{
	NetworkManager mgr(ExecutableType::UI);
	mgr.Send(ServiceMessageToString(ServiceMessage::START_CM));
}