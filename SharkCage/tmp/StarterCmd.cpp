// StarterCmd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../CageService/NetworkManager.h"

#include <Windows.h>
#include <Commdlg.h>
#include <vector>
#include <sstream>

#include "../CageService/MsgService.h"

int main()
{
	// Display a message box that asks wheter to start the cage manager or not
	int msgbox_id = MessageBox(
		NULL,
		const_cast<LPCWSTR>(L"Do you want to start the Cage manager through the Cage service?"),
		const_cast<LPCWSTR>(L"Start Cage Manager"),
		MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO
	);

	NetworkManager network_manager(ExecutableType::UI);
	switch (msgbox_id)
	{
	case IDNO:
		return -1; // Just quit
	case IDYES:
		// Send message to service to start cage manager
		network_manager.Send(ServiceMessageToString(ServiceMessage::START_CM));
		break;
	}

	// Init filters for the second dialog
	std::vector<wchar_t> internal_filename(MAX_PATH);

	// Init buffers for the for the file name
	OPENFILENAME open_file = { 0 };

	open_file.lStructSize = sizeof(OPENFILENAME);
	open_file.lpstrFilter = L"Executables (.exe)\0*.exe\0All files\0*.*\0"; // Set filters
	open_file.nFilterIndex = 1;        // Default filter
	open_file.lpstrFile = internal_filename.data(); // Buffer for the file name
	open_file.nMaxFile = MAX_PATH; // Max length of the path to the executable
	open_file.lpstrInitialDir = L"";  // Pass empty directory path to display last user selected directory
	open_file.lpstrTitle = L"Select program to run in cage manager"; // Title with description
	open_file.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_NOREADONLYRETURN | OFN_FILEMUSTEXIST;

	// Display the file selector dialog
	if (!::GetOpenFileName(&open_file))
	{
		if (::CommDlgExtendedError() == 0)
		{
			return 0;
		}
		return -1;
	}
	std::wstring filename(internal_filename.data());

	std::wostringstream os;
	os << "Selected file: " << filename << std::endl;
	::OutputDebugString(os.str().c_str());

	network_manager.Send(ServiceMessageToString(ServiceMessage::START_PC) + L" " + filename);

	return 0;
}
