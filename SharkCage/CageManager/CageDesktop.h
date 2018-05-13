#include <Windows.h>
#include <iostream>

class CageDesktop
{
public:
	CageDesktop(PSECURITY_DESCRIPTOR pSD, HDESK *newDesktop);
	~CageDesktop();
private:
	HDESK desktop;
};

CageDesktop::CageDesktop(PSECURITY_DESCRIPTOR pSD, HDESK *newDesktop)
{
	desktop = GetThreadDesktop(GetCurrentThreadId());

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

	*newDesktop = CreateDesktop(TEXT("SharkCageDesktop"), NULL, NULL, NULL, desk_access_mask, &sa);

	if (SwitchDesktop(*newDesktop) == false)
	{
		std::cout << "Failed to switch to cage dekstop. Error " << GetLastError() << std::endl;
	}

	if (SetThreadDesktop(*newDesktop) == false)
	{
		std::cout << "Failed to set thread desktop to new desktop. Error " << GetLastError() << std::endl;
	}
}

CageDesktop::~CageDesktop()
{
	if (SetThreadDesktop(desktop) == false)
	{
		std::cout << "Failed to set thread desktop back to old desktop.Error " << GetLastError() << std::endl;
	}

	if (SwitchDesktop(desktop) == false)
	{
		std::cout << "Failed to switch back to old dekstop. Error " << GetLastError() << std::endl;
	}
}
