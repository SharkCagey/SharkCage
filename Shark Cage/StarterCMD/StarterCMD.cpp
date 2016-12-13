// StarterCMD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Cage service/NetworkManager.h"
#include "../Cage service/MSG_Service.h"
#include <Windows.h>
#include <Commdlg.h>

int main() {
    NetworkManager mgr(UI);


	int msgboxID = MessageBox(
		NULL,
		(LPCWSTR)L"Do you want to start the Cage manager through the Cage service?",
		(LPCWSTR)L"Start Cage Manager",
		MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO
	);

	switch (msgboxID) {
	case IDNO:
		return -1; // Just quit
	case IDYES:
        mgr.send(MSG_TO_SERVICE_toString(START_CM));
		break;
	}
    

    TCHAR filename[MAX_PATH] = {0};
    TCHAR filter[MAX_PATH] = 
    {
        TEXT("Executables (.exe)\0*.exe\0")
        TEXT("All Files\0*.*\0")
    };

    OPENFILENAME ofn = {0};
    TCHAR _sbuf[MAX_PATH];
    TCHAR *initDir = _sbuf;

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.lpstrFilter         = filter;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = filename;
    ofn.nMaxFile            = MAX_PATH;
    ofn.lpstrInitialDir     = initDir;
    ofn.lpstrTitle          = L"Select program to run in Cage Manager";
    ofn.Flags               = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_NOREADONLYRETURN | OFN_FILEMUSTEXIST;

    if (!GetOpenFileName(&ofn)) {
        if (CommDlgExtendedError() == 0) {
            return 0;
        }
        return -1;
    }
    std::cout << "Selected file: " << filename << std::endl;

    ///////////////// A bit too simple, should be replaces
    std::string convertedFilename;
    for (char c : filename) {
        convertedFilename += c;
    }
    ////////////////

    mgr.send(MSG_TO_SERVICE_toString(START_PC) + convertedFilename);



    return 0;
}
