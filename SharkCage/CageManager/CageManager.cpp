#include "stdafx.h"

#include "../SharedFunctionality/NetworkManager.h"
#include "../SharedFunctionality/SharedFunctions.h"
#include "../SharedFunctionality/CageData.h"

#define byte WIN_BYTE_OVERRIDE

#include "stdio.h"
#include "Aclapi.h"
#include <tchar.h>
#include "sddl.h"
#include <string>
#include <LM.h>
#include <memory>
#include <vector>
#include <thread>
#include <optional>
#include <fstream>
#include <cwctype>
#include <algorithm>

#include "CageManager.h"
#include "CageLabeler.h"

NetworkManager network_manager(ContextType::MANAGER);

int main()
{
	CageManager cage_manager;
	
	// listen for the message
	std::wstring message = network_manager.Listen(10);
	std::wstring message_data;
	auto parse_result = SharedFunctions::ParseMessage(message, message_data);

	if (parse_result != CageMessage::LABELER_CONFIG)
	{
		std::cout << "Could not process incoming message" << std::endl;
		return 1;
	}

	CageData cage_data = { message_data };
	if (!SharedFunctions::ParseStartProcessMessage(cage_data))
	{
		std::cout << "Could not process start process message" << std::endl;
		return 1;
	}

	const int work_area_width = 300;

	const std::wstring LABELER_WINDOW_CLASS_NAME = L"shark_cage_token_window";
	std::thread labeler_thread(
		&CageManager::StartCageLabeler,
		cage_manager,
		cage_data,
		work_area_width,
		LABELER_WINDOW_CLASS_NAME
	);

	labeler_thread.join();

	return 0;
}

void CageManager::StartCageLabeler(
	const CageData &cage_data,
	const int work_area_width,
	const std::wstring &labeler_window_class_name)
{
	CageLabeler cage_labeler = CageLabeler(cage_data, work_area_width, labeler_window_class_name);
	cage_labeler.Init();
}