#pragma once

#include "stdafx.h"

#include ".\asio\include\asio.hpp"

#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <vector>
#include <sstream>

enum class DLLEXPORT ExecutableType
{
	UI,      // for the front end contacting the service
	SERVICE, // for the CageService
	MANAGER  // for the CageManager
};

class NetworkManager
{
	using tcp = asio::ip::tcp;

private:
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
	DLLEXPORT NetworkManager(ExecutableType type);


	/**
	* DO NOT USE - OBSOLETE FUNCTION
	* Function to be used by UI and cageManager to listen for messages from service
	* max lenth of message is 1024 characters  - evey message must end with '\n' character
	**/
	DLLEXPORT std::wstring Receive();


	/**
	* Function to be used to send messages
	* max message lenght is 1024 characters
	*
	**/
	DLLEXPORT bool Send(const std::wstring &msg);

	/**
	* function used by all components to listen for messages
	*  #blocking call
	*
	**/
	DLLEXPORT std::wstring Listen();

private:
	// ports lisened to: ui 1337, service 1338, manager 1339
	bool InitUi();

	bool InitService();

	bool InitManager();

	std::wstring ToString(const std::vector<char> &message);

	std::vector<char> ToCharVector(const std::wstring &string);
};

// make a pinvoke callable interface which is just able to send
// a .config file + path to external program (like keepass)
extern "C" DLLEXPORT void SendConfigAndExternalProgram(const wchar_t *config_path, const wchar_t *secondary_program_name);

// make a pinvoke callable interface which is just able to tell the service to
// start the cage manager (after #21: no longer necessary as manager only shows ínformation on new desktop and gets started implicitely)
extern "C" DLLEXPORT void StartCageManager();