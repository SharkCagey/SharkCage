#pragma once

#include <Windows.h>
#include <string>

class CageService {
public:
    CageService();
    ~CageService();

    /*
     * Starts the executable in a new process on the normal desktop in the given session.
     *
     * @appName The path to the execuatble:
     * @desktopName The name of the desktop to start the program in.
     * @sessionId The sessionId of the user.
     */
    DWORD startCageManager(LPCTSTR appName, DWORD sessionId);

    /*
     * Starts the executable in a new process on the respective desktop in the given session.
     *
     * @appName The path to the execuatble:
     * @desktopName The name of the desktop to start the program in.
     * @sessionId The sessionId of the user.
     */
    DWORD startCageManager(LPCTSTR appName, LPTSTR desktopName, DWORD sessionId);

    /*
     * Parses a message and does the action according to content of the message.
     * The message must be in the form of:
     * "MSG_TO_SERVICE.constant absolute/path/to/executable"
     */
    void handleMessage(std::string message);

private:
    std::string CageService::GetLastErrorAsString(DWORD errorID);
};
