#include <Windows.h>
#include <iostream>

class CageDesktop
{
public:
	CageDesktop(PSECURITY_DESCRIPTOR pSD, HDESK *newDesktop);
	~CageDesktop();
	BOOL Init(HDESK *newDesktop);
private:
	HDESK old_desktop;
};

CageDesktop::CageDesktop(PSECURITY_DESCRIPTOR pSD, HDESK *new_desktop)
{
	old_desktop = GetThreadDesktop(GetCurrentThreadId());

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

	*new_desktop = CreateDesktop(TEXT("shark_cage_desktop"), NULL, NULL, NULL, desk_access_mask, &sa);
}

CageDesktop::~CageDesktop()
{
	if (SetThreadDesktop(old_desktop) == false)
	{
		std::cout << "Failed to set thread desktop back to old desktop.Error " << GetLastError() << std::endl;
	}

	if (SwitchDesktop(old_desktop) == false)
	{
		std::cout << "Failed to switch back to old dekstop. Error " << GetLastError() << std::endl;
	}
}


BOOL CageDesktop::Init(HDESK *new_desktop)
{
	if (SwitchDesktop(*new_desktop) == false)
	{
		std::cout << "Failed to switch to cage dekstop. Error " << GetLastError() << std::endl;
		return false;
	}

	if (SetThreadDesktop(*new_desktop) == false)
	{
		std::cout << "Failed to set thread desktop to new desktop. Error " << GetLastError() << std::endl;
		return false;
	}
	return true;
}