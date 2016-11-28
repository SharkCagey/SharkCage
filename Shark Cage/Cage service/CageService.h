#pragma once

#include <Windows.h>
#include <string>

class CageService
{
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
    void startCageManager(LPCTSTR appName, DWORD sessionId);

    /*
     * Starts the executable in a new process on the respective desktop in the given session.
     *
     * @appName The path to the execuatble:
     * @desktopName The name of the desktop to start the program in.
     * @sessionId The sessionId of the user.
     */
    void startCageManager(LPCTSTR appName, LPTSTR desktopName, DWORD sessionId);

private:
    std::string CageService::GetLastErrorAsString(DWORD errorID);
};
