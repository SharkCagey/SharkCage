#include <Windows.h>
#include <iostream>

#include "CageLabeler.h"
#include "FullWorkArea.h"

class CageDesktop
{
public:
	CageDesktop(PSECURITY_DESCRIPTOR p_sd, std::wstring app_name, std::wstring app_token, std::wstring additional_app);
	~CageDesktop();
	bool Init();
private:
	int cage_default_size = 300;
	HDESK old_desktop;
	HDESK new_desktop;
	FullWorkArea full_work_area = FullWorkArea(cage_default_size);
	CageLabeler cage_labeler;
};

CageDesktop::CageDesktop(PSECURITY_DESCRIPTOR p_sd, std::wstring app_name, std::wstring app_token, std::wstring additional_app)
{
	old_desktop = ::GetThreadDesktop(GetCurrentThreadId());

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
	sa.lpSecurityDescriptor = p_sd;
	sa.bInheritHandle = FALSE;

	new_desktop = ::CreateDesktop(TEXT("shark_cage_desktop"), NULL, NULL, NULL, desk_access_mask, &sa);

	cage_labeler = CageLabeler(app_name, app_token, additional_app, cage_default_size);
}

CageDesktop::~CageDesktop()
{
	if (!::SetThreadDesktop(old_desktop))
	{
		std::cout << "Failed to set thread desktop back to old desktop. Error " << ::GetLastError() << std::endl;
	}

	if (!::SwitchDesktop(old_desktop))
	{
		std::cout << "Failed to switch back to old desktop. Error " << ::GetLastError() << std::endl;
	}
}


bool CageDesktop::Init()
{
	if (!::SwitchDesktop(new_desktop))
	{
		std::cout << "Failed to switch to cage desktop. Error " << ::GetLastError() << std::endl;
		return false;
	}

	if (!::SetThreadDesktop(new_desktop))
	{
		std::cout << "Failed to set thread desktop to new desktop. Error " << ::GetLastError() << "Switching back to old desktop" << std::endl;
		::SwitchDesktop(old_desktop);
		return false;
	}

	if (!full_work_area.Init())
	{
		std::cout << "Failed to set area too fullscreen" << std::endl;
		return false;
	}

	if (!cage_labeler.Init())
	{
		std::cout << "Failed to label cage" << std::endl;
		return false;
	}

	return true;
}