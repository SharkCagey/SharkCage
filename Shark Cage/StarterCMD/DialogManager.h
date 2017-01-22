#pragma once

#include <Windows.h>

class DialogManager {
    // Disallow constructor and destructor
    DialogManager() = delete;
    ~DialogManager() = delete;
    // Disallow copying
    DialogManager& operator=(const DialogManager&) = delete;
    DialogManager(const DialogManager&) = delete;
    //Disallow moving
    DialogManager& operator=(DialogManager&&) = delete;
    DialogManager(DialogManager&&) = delete;

public:
    static HWND WINAPI CreateDialogParam(HINSTANCE hinst,
                                         LPCTSTR pszTemplate,
                                         HWND hwndParent,
                                         DLGPROC lpDlgProc,
                                         LPARAM dwInitParam);
};
