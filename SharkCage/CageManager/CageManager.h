#pragma once

/*!
 * Strarts the sharkcage process in the new desktop, and starts the CageLabeler.
 */
class CageManager
{
public:
	/*!
	 * Starts the sharkcage process in the new desktop, with the chosen config and application
	 * to run inside the Cage.
	 * @param security_attributes the security descriptor
	 * @param &cage_data the adress of the struct containing the config path, and chosen
	 * application info as well as the additional application, if any
	 */
	void CageManager::StartCage(SECURITY_ATTRIBUTES security_attributes, const CageData &cage_data);

private:
	void StartCageLabeler(
		HDESK desktop_handle,
		const CageData &cage_data,
		const int work_area_width,
		const std::wstring &labeler_window_class_name);

	// FIXME: extra class for this? process handling? -> could also do the wait stuff
	static BOOL CALLBACK GetOpenWindowHandles(_In_ HWND hwnd, _In_ LPARAM l_param);
	static BOOL CALLBACK GetOpenProcesses(_In_ HWND hwnd, _In_ LPARAM l_param);
};