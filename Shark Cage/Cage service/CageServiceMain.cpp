// Cage service.cpp : Defines the entry point for the console application.

// INFO FOR GUYS WORKING ON TASK 1.1 AND 1.2:
// all actual work of service is performed in ServiceWorkerThread 
// (right now there is only sleep() to simulate some work)
// you might also want to look at ServiceMain (that is a place where startup initialization happens
// and also on ServiceCtrlHandler, this is the place where you catch and process the events generated by OS
// if you are not familiar with events in programming, just use google a little, its not complicated
// 
// in project there is a readme file explaining how to install/uninstall the service

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include "StatusManager.h"
#include "CageService.h"
#include "NetworkManager.h"
#include "stdafx.h"
#include <Windows.h>
#include <fstream>
#include <string>

// Variables for the windows service
#define SERVICE_NAME _T("Cage Service")
SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE workerThread;

// Own variables for the cage service
StatusManager statusManager; //holds the current status of Shark Cage Service
NetworkManager networkMgr(SERVICE);
CageService cageService;


// Forward declaration of windows service functions
VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler (DWORD);
DWORD WINAPI ServiceWorkerThread (LPVOID lpParam);


int _tmain (int argc, TCHAR *argv[]) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher (ServiceTable) == FALSE) {
        return GetLastError ();
    }

    return 0;
}

VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv) {
    DWORD Status = E_FAIL;

    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler (SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL) {
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory (&g_ServiceStatus, sizeof (g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle , &g_ServiceStatus) == FALSE) {
        OutputDebugString(_T("Cage Service: ServiceMain: SetServiceStatus returned error"));
    }

    /*
    * Perform tasks necessary to start the service here
    */

    // Create a service stop event to wait on later
    g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL) {
        // Error creating event
        // Tell service controller we are stopped and exit
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
            OutputDebugString(_T("Cage Service: ServiceMain: SetServiceStatus returned error"));
        }
        return;
    }    

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(_T("Cage Service: ServiceMain: SetServiceStatus returned error"));
    }

    cageService.readConfigFile();
    cageService.getImageIndex();

    // Start a thread that will perform the main task of the service
    workerThread = CreateThread (NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    // Wait until our worker thread exits signaling that the service needs to stop
    WaitForSingleObject (workerThread, INFINITE);


    /*
    * Perform any cleanup tasks 
    */
    CloseHandle (g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(_T("Cage Service: ServiceMain: SetServiceStatus returned error"));
    }

    return;
}

VOID WINAPI ServiceCtrlHandler (DWORD CtrlCode) {
    switch (CtrlCode) {
        case SERVICE_CONTROL_STOP:
            if (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
                if (cageService.cageManagerRunning()) {
                    break;
                }

                // Perform tasks necessary to stop the service here 
                g_ServiceStatus.dwControlsAccepted = 0;
                g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
                g_ServiceStatus.dwWin32ExitCode = 0;
                g_ServiceStatus.dwCheckPoint = 4;

                if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
                    OutputDebugString(_T("Cage Service: ServiceCtrlHandler: SetServiceStatus returned error"));
                }

                // This will signal the worker thread to start shutting down
                TerminateThread(workerThread, -1); // Have to kill it because listen() is blocking
                SetEvent (g_ServiceStopEvent);
            }
            break;
        default:
            break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    // periodically check if the service has been requested to stop
    // But does not work properly because netwirkMgr.listen() is a blocking call.
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0) {
        // Look for messages and parse them
        std::string msg = networkMgr.listen();
        cageService.handleMessage(msg, &networkMgr);
    }

    return ERROR_SUCCESS;
}
