#pragma once

#include "Windows.h"
#include <string>
#include "FullWorkArea.h"

/*!
 * \brief Contains desktop information.
 */
class CageDesktop
{
public:

	/*!
	 * \brief Constructor saves the thread ID of the old desktop and creates the new
	 * desktop.
	 * @param security_atributes the security descriptor for the new desktop
	 * @param work_area_width the width of the work area
	 * @param &desktop_name the given name for the desktop
	 */
	CageDesktop(
		SECURITY_ATTRIBUTES security_attributes, 
		const int work_area_width,
		const std::wstring &desktop_name);
	
	/*!
	 * \brief Destructor switches thread and desktop to old desktop.
	 */
	~CageDesktop();

	/*!
	 * \brief Switches the thread and desktop to the new desktop and sets the area to
	 * full screen.
	 * @param &desktop_handle return parameter, the new desktop gets saved here.
	 * @return true if successful, false otherwise
	 */
	bool Init(HDESK &desktop_handle);
private:
	HDESK old_desktop;
	HDESK new_desktop;
	FullWorkArea full_work_area;
};
