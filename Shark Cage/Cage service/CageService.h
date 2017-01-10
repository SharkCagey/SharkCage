#pragma once

#include "NetworkManager.h"
#include <Windows.h>
#include <string>

class CageService {
public:
    CageService();
    ~CageService();

    bool beginsWith(const std::string string, const std::string prefix);

    /*
    * Starts the Cage Manager in a new process on the normal desktop in the given session.
    *
    * @sessionId The sessionId of the user.
    * @return The process ID of the started process.
    */
    DWORD startCageManager(DWORD sessionId);

    /*
     * Starts the executable in a new process on the normal desktop in the given session.
     *
     * @appName The path to the execuatble.
     * @sessionId The sessionId of the user.
     * @return The process ID of the started process.
     */
    DWORD startCageManager(LPCTSTR appName, DWORD sessionId);

    /*
     * Starts the executable in a new process on the respective desktop in the given session.
     *
     * @appName The path to the execuatble.
     * @desktopName The name of the desktop to start the program in.
     * @sessionId The sessionId of the user.
     * @return The process ID of the started process.
     */
    DWORD startCageManager(LPCTSTR appName, LPTSTR desktopName, DWORD sessionId);

    void stopCageManager();

    /*
     * Parses a message and does the action according to content of the message.
     * The message must be in the form of:
     * "MSG_TO_SERVICE.constant absolute/path/to/executable"
     */
    void handleMessage(std::string message, NetworkManager* mgr);

private:
    DWORD cageManagerProcessId;
    std::string CageService::GetLastErrorAsString(DWORD errorID);
};
