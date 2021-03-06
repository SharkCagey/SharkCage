// Cage service.cpp : Defines the entry point for the console application.

// INFO FOR GUYS WORKING ON TASK 1.1 AND 1.2:
// all actual work of service is performed in ServiceWorkerThread 
// (right now there is only sleep() to simulate some work)
// you might also want to look at ServiceMain (that is a place where startup initialization happens
// and also on ServiceCtrlHandler, this is the place where you catch and process the events generated by OS
// if you are not familiar with events in programming, just use google a little, its not complicated
// 
// in project there is a readme file explaining how to install/uninstall the service

#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <vector>

#include "CageService.h"

// Variables for the windows service
const std::wstring SERVICE_NAME = L"shark_cage_service";
SERVICE_STATUS        g_service_status = { 0 };
SERVICE_STATUS_HANDLE g_status_handle = NULL;
HANDLE                g_service_stop_event = INVALID_HANDLE_VALUE;
HANDLE worker_thread;

// Own variables for the cage service
NetworkManager network_manager(ContextType::SERVICE);
CageService cage_service;


// Forward declaration of windows service functions
VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID);


int _tmain(int argc, TCHAR *argv[])
{
	std::vector<wchar_t> service_name_buf(SERVICE_NAME.begin(), SERVICE_NAME.end());
	service_name_buf.push_back(0);

	SERVICE_TABLE_ENTRY service_table[] = {
		{ service_name_buf.data(), static_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMain) },
		{ NULL, NULL }
	};

	if (!::StartServiceCtrlDispatcher(service_table))
	{
		return ::GetLastError();
	}

	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	DWORD status = E_FAIL;

	// Register our service control handler with the SCM
	g_status_handle = ::RegisterServiceCtrlHandler(SERVICE_NAME.c_str(), ServiceCtrlHandler);

	if (g_status_handle == nullptr)
	{
		return;
	}

	// Tell the service controller we are starting
	g_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_service_status.dwControlsAccepted = 0;
	g_service_status.dwCurrentState = SERVICE_START_PENDING;
	g_service_status.dwWin32ExitCode = 0;
	g_service_status.dwServiceSpecificExitCode = 0;
	g_service_status.dwCheckPoint = 0;

	if (!::SetServiceStatus(g_status_handle, &g_service_status))
	{
		std::wcout << SERVICE_NAME << L": ServiceMain: SetServiceStatus returned error" << std::endl;
	}

	/*
	* Perform tasks necessary to start the service here
	*/

	// Create a service stop event to wait on later
	g_service_stop_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_service_stop_event == nullptr)
	{
		// Error creating event
		// Tell service controller we are stopped and exit
		g_service_status.dwControlsAccepted = 0;
		g_service_status.dwCurrentState = SERVICE_STOPPED;
		g_service_status.dwWin32ExitCode = ::GetLastError();
		g_service_status.dwCheckPoint = 1;

		if (::SetServiceStatus(g_status_handle, &g_service_status))
		{
			std::wcout << SERVICE_NAME << L": ServiceMain: SetServiceStatus returned error" << std::endl;
		}
		return;
	}

	// Tell the service controller we are started
	g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_service_status.dwCurrentState = SERVICE_RUNNING;
	g_service_status.dwWin32ExitCode = 0;
	g_service_status.dwCheckPoint = 0;

	if (!::SetServiceStatus(g_status_handle, &g_service_status))
	{
		std::wcout << SERVICE_NAME << L": ServiceMain: SetServiceStatus returned error" << std::endl;
	}

	// Start a thread that will perform the main task of the service
	worker_thread = ::CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	if (worker_thread == nullptr)
	{
		std::wcout << SERVICE_NAME << L": ServiceMain: Could not create worker thread" << std::endl;
		return;
	}

	// Wait until our worker thread exits signaling that the service needs to stop
	::WaitForSingleObject(worker_thread, INFINITE);

	/*
	* Perform any cleanup tasks
	*/
	::CloseHandle(g_service_stop_event);

	// Tell the service controller we are stopped
	g_service_status.dwControlsAccepted = 0;
	g_service_status.dwCurrentState = SERVICE_STOPPED;
	g_service_status.dwWin32ExitCode = 0;
	g_service_status.dwCheckPoint = 3;

	if (!::SetServiceStatus(g_status_handle, &g_service_status))
	{
		std::wcout << SERVICE_NAME << L": ServiceMain: SetServiceStatus returned error" << std::endl;
	}
}

VOID WINAPI ServiceCtrlHandler(DWORD ctrl_code)
{
	switch (ctrl_code)
	{
	case SERVICE_CONTROL_STOP:
		if (g_service_status.dwCurrentState == SERVICE_RUNNING)
		{
			if (cage_service.CageManagerRunning())
			{
				break;
			}

			// Perform tasks necessary to stop the service here 
			g_service_status.dwControlsAccepted = 0;
			g_service_status.dwCurrentState = SERVICE_STOP_PENDING;
			g_service_status.dwWin32ExitCode = 0;
			g_service_status.dwCheckPoint = 4;

			if (!::SetServiceStatus(g_status_handle, &g_service_status))
			{
				std::wcout << SERVICE_NAME << L": ServiceMain: ServiceCtrlHandler returned error" << std::endl;
			}

			// This will signal the worker thread to start shutting down
			::SetEvent(g_service_stop_event);
		}
		break;
	default:
		break;
	}
}

DWORD WINAPI ServiceWorkerThread(LPVOID)
{
	while (::WaitForSingleObject(g_service_stop_event, 0) != WAIT_OBJECT_0)
	{
		// Look for messages and parse them
		std::wstring msg = network_manager.Listen(3);
		if (!msg.empty())
		{
			cage_service.HandleMessage(msg, network_manager);
		}
	}

	return ERROR_SUCCESS;
}
