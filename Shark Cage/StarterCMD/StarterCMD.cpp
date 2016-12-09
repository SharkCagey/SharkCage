// StarterCMD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Cage service/NetworkManager.h"
#include "../Cage service/MSG_Service.h"


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
		return 0; // Just quit
	case IDYES:
        mgr.send(MSG_TO_SERVICE_toString(START_CM));
		break;
	}

    // Show file selector dialog and send path to the cage service

    return 0;
}
