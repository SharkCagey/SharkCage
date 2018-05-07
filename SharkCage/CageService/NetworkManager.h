#pragma once

#define ASIO_STANDALONE

#include "asio-1.10.8\include\asio.hpp"
#include "Windows.h"
#include <iostream>
#include <locale>
#include <codecvt>
#include <vector>
#include <sstream>


using namespace asio::ip;

enum class ExecutableType
{
	UI,      // for the front end contacting the service
	SERVICE, // for the CageService
	MANAGER  // for the CageManager
};

class NetworkManager
{
	asio::io_service io_service;
	tcp::socket socket;

	tcp::endpoint send_endpoint;
	tcp::endpoint send_endpoint_ui; // this is only used by service to give feedback to ui - not implemented so far
	tcp::endpoint rec_endpoint;

	tcp::acceptor acceptor; // accepting tcp connections

	std::vector<char> send_buf;
	std::vector<char> rec_buf;

public:
	/*
	* Constructor
	* type variable says in what role network manager should be initialized
	*/
	NetworkManager(ExecutableType type) : io_service(), socket(io_service), acceptor(io_service, tcp::endpoint(tcp::v4(), 1338))
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


	/**
	* DO NOT USE - OBSOLETE FUNCTION
	* Function to be used by UI and cageManager to listen for messages from service
	* max lenth of message is 1024 characters  - evey message must end with '\n' character
	**/
	std::wstring Receive()
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


	/**
	* Function to be used to send messages
	* max message lenght is 1024 characters
	*
	**/
	bool Send(const std::wstring &msg)
	{
		std::vector<char> message = ToCharVector(msg);

		try
		{
			tcp::socket tmp_socket(io_service);
			tmp_socket.connect(send_endpoint);
			send_buf = message;
			tmp_socket.write_some(asio::buffer(send_buf));
			return true;
		}
		catch (std::exception&)
		{
			return false;
		}
	}

	/**
	* function used by all components to listen for messages
	*  #blocking call
	*
	**/
	std::wstring Listen()
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
			std::wcout << e.what();
		}

		std::istream str(&buffer);
		std::string narrow_string;
		std::getline(str, narrow_string);

		// no suitable alternative in c++ standard yet, so it is safe to use for now
		// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(narrow_string);
	}

private:
	// ports lisened to: ui 1337, service 1338, manager 1339
	bool InitUi()
	{
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), "localhost", "1338");
		send_endpoint = *resolver.resolve(query);

		//acceptor = tcp::acceptor(ioservice, tcp::endpoint(tcp::v4(), 1337));

		return true;
	}

	bool InitService()
	{
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), "localhost", "1339");
		send_endpoint = *resolver.resolve(query);


		acceptor = tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), 1338));

		return true;
	}

	bool InitManager()
	{
		acceptor = tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), 1339));

		return true;
	}

	std::wstring ToString(const std::vector<char> &message)
	{
		std::string(message.data());

		// no suitable alternative in c++ standard yet, so it is safe to use for now
		// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(message.data());
	}

	std::vector<char> ToCharVector(const std::wstring &string)
	{
		// no suitable alternative in c++ standard yet, so it is safe to use for now
		// warning is suppressed by a define in project settings: _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto narrow_string = converter.to_bytes(string);
		std::vector<char> message(narrow_string.begin(), narrow_string.end());
		message.push_back(0);

		return message;
	}
};
