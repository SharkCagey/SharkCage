#pragma once

#include "Windows.h"
#include <string>
#include "FullWorkArea.h"

/*!
 * contains desktop information.
 */
class CageDesktop
{
public:
	CageDesktop(
		SECURITY_ATTRIBUTES security_attributes, 
		const int work_area_width,
		const std::wstring &desktop_name);
	~CageDesktop();
	bool Init(HDESK &desktop_handle);
private:
	HDESK old_desktop;
	HDESK new_desktop;
	FullWorkArea full_work_area;
};
