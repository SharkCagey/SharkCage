// StarterCMD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Cage service/NetworkManager.h"
#include "../Cage service/MSG_Service.h"


int main() {
    NetworkManager mgr(UI);

    

	int msgboxID = MessageBox(
		NULL,
		(LPCWSTR)L"Resource not available\nDo you want to try again?",
		(LPCWSTR)L"Account Details",
		MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
	);

	switch (msgboxID)
	{
	case IDCANCEL:
		// TODO: add code
		break;
	case IDTRYAGAIN:
		mgr.send(MSG_TO_SERVICE_toString(START_CM));
		break;
	case IDCONTINUE:
		// TODO: add code
		break;
	}

    return 0;
}
