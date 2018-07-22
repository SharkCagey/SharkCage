#include "stdafx.h"

#include <iostream>

#include "CageDesktop.h"

CageDesktop::CageDesktop(
	SECURITY_ATTRIBUTES security_attributes,
	const int work_area_width,
	const std::wstring &desktop_name)
	: full_work_area(work_area_width)
{
	old_desktop = ::GetThreadDesktop(GetCurrentThreadId());

	ACCESS_MASK desk_access_mask = DESKTOP_CREATEWINDOW
		| DESKTOP_READOBJECTS
		| DESKTOP_SWITCHDESKTOP
		| DESKTOP_WRITEOBJECTS;

	new_desktop = ::CreateDesktop(desktop_name.c_str(), NULL, NULL, NULL, desk_access_mask, &security_attributes);
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

	::CloseDesktop(new_desktop);
}

bool CageDesktop::Init(HDESK &desktop_handle)
{
	if (!::SwitchDesktop(new_desktop))
	{
		std::cout << "Failed to switch to cage desktop. Error " << ::GetLastError() << std::endl;
		return false;
	}

	if (!::SetThreadDesktop(new_desktop))
	{
		std::cout << "Failed to set thread desktop to new desktop. Error " << ::GetLastError() << ". Switching back to old desktop" << std::endl;
		::SwitchDesktop(old_desktop);
		return false;
	}

	if (!full_work_area.Init())
	{
		std::cout << "Failed to set area too fullscreen" << std::endl;
		return false;
	}

	// FIXME close ctfmon.exe (input handler, leaks on win7 according to keepass)

	desktop_handle = new_desktop;

	return true;
}