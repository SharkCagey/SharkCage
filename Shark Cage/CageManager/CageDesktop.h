#include <Windows.h>
#include <iostream>
#include "FullWorkArea.h"

class CageDesktop
{
public:
	CageDesktop(PSECURITY_DESCRIPTOR pSD);
	~CageDesktop();
private:
	HDESK desktop;
};

CageDesktop::CageDesktop(PSECURITY_DESCRIPTOR pSD)
{
	HDESK desktop = GetThreadDesktop(GetCurrentThreadId());

	ACCESS_MASK desk_access_mask = DESKTOP_CREATEMENU
		| DESKTOP_CREATEWINDOW
		| DESKTOP_ENUMERATE
		| DESKTOP_HOOKCONTROL
		| DESKTOP_JOURNALPLAYBACK
		| DESKTOP_JOURNALRECORD
		| DESKTOP_READOBJECTS
		| DESKTOP_SWITCHDESKTOP
		| DESKTOP_WRITEOBJECTS
		| READ_CONTROL
		| WRITE_DAC
		| WRITE_OWNER;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	HDESK newDesktop = CreateDesktop(TEXT("SharkCageDesktop"), NULL, NULL, NULL, desk_access_mask, &sa);

	if (SwitchDesktop(newDesktop) == 0)
	{
		std::cout << "Failed to switch to cage dekstop. Error " << GetLastError() << std::endl;
	}

	if (SetThreadDesktop(newDesktop) == false)
	{
		std::cout << "Failed to set thread desktop to new desktop. Error " << GetLastError() << std::endl;
	}

	FullWorkArea::FullWorkArea();
}

CageDesktop::~CageDesktop()
{
	if (SetThreadDesktop(desktop) == false)
	{
		std::cout << "Failed to set thread desktop back to old desktop.Error " << GetLastError() << std::endl;
	}

	if (SwitchDesktop(desktop) == 0)
	{
		std::cout << "Failed to switch back to old dekstop. Error " << GetLastError() << std::endl;
	}
}
