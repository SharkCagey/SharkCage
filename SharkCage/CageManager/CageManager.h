/*! \file CageManager.h
 * \brief Manages CageConfigurator and CageLabeler, essentially the user control unit.
 */
#pragma once

/*!
 * \brief Starts the sharkcage process in the new desktop, and starts the CageLabeler.
 */
class CageManager
{
public:
<<<<<<< HEAD
	/*!
	 * \brief Starts the sharkcage process in the new desktop, with the chosen config and application
	 * to run inside the Cage.
	 * @param security_attributes the security descriptor
	 * @param &cage_data the adress of the struct containing the config path, and chosen
	 * application info as well as the additional application, if any
	 */
	void CageManager::StartCage(SECURITY_ATTRIBUTES security_attributes, const CageData &cage_data);
=======
	void CageManager::StartCage(SECURITY_ATTRIBUTES security_attributes, const CageData &cage_data, const std::wstring &group_name);
	bool CageManager::ProcessRunning(const std::wstring &process_path);
>>>>>>> develop

private:
	void StartCageLabeler(
		HDESK desktop_handle,
		const CageData &cage_data,
		const int work_area_width,
		const std::wstring &labeler_window_class_name);
	void CageManager::ActivateApp(
		const HANDLE token_handle,
		const std::wstring &path,
		const HANDLE &event,
		const HDESK &desktop_handle,
		PROCESS_INFORMATION &process_info,
		SECURITY_ATTRIBUTES security_attributes,
		STARTUPINFO info,
		std::vector<HANDLE> &handles);

	// FIXME: extra class for this? process handling? -> could also do the wait stuff
	static BOOL CALLBACK GetOpenWindowHandles(_In_ HWND hwnd, _In_ LPARAM l_param);
	static BOOL CALLBACK GetOpenProcesses(_In_ HWND hwnd, _In_ LPARAM l_param);
	static BOOL CALLBACK CageManager::ActivateProcess(_In_ HWND hwnd, _In_ LPARAM l_param);
};
