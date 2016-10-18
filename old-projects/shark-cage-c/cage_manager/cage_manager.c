///
/// \file cage_manager.c
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage manager - main module
/// shows the config on the winlogon screan
///



#include <Windows.h>
#include <stdio.h>
#include <LM.h>
#include <time.h>
#include <Psapi.h>

//#include <ntdef.h>
//#include <Ntsecpkg.h>

#include "cage_lib.h"

HANDLE log;
BOOL mailslot_stop_receiver = FALSE;
PSID sid_group = NULL;
HDESK mydesk = NULL;
HDESK defdesk = NULL;
HWND window = NULL;
LPSTR group_name = NULL;
HWND start_app_button = NULL;
HWND gotodesk_button = NULL;

LRESULT CALLBACK window_callback(HWND window, UINT message, WPARAM w_para, LPARAM l_para);

DWORD WINAPI mailslot_receiver_thread(LPVOID arg);

BOOL generate_and_add_group_id(char **group_name_out);


BOOL WINAPI switcher_func(LPSTR desk);
BOOL make_switch = FALSE;
BOOL switcher_thread_stop = FALSE;

// app ifno
APPLICATION_INFO appinfo[MAX_APPLICATIONS] = { 0 };
DWORD appinfo_members = 0;


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous_instance, PWSTR command_line, int command_show) {
	const char window_class_name[] = "cage_manager";
	WNDCLASS window_class = { 0 };
	window_class.lpfnWndProc = window_callback;
	window_class.hInstance = instance;
	window_class.lpszClassName = window_class_name;

	RegisterClass(&window_class);

	if (!init_logging(&log, "manager")) {
		goto clean;
	}


	// TODO read appinfo from registry or file or anywhere else than hardcoded...
	appinfo_members = 1;
	appinfo[0].id = 0;
	char name[] = "notepad.exe\0";
	for (unsigned i = 0; i < strlen(name); i++) {
		appinfo[0].name[i] = name[i];
	}
	char window_name[] = "Notepad\0";
	for (unsigned i = 0; i < strlen(name); i++) {
		appinfo[0].window_class_name[i] = window_name[i];
	}


	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff381409(v=vs.85).aspx
	window = CreateWindowEx(0, window_class_name, "cage_manager", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, instance, NULL);
	if (window == NULL) {
		goto clean;
	}

	group_name = malloc(sizeof(char)* 256);
	if (group_name == NULL) {
		LOG("failed to get memory for group_name.\r\n");
		goto clean;
	}

	// create mailslot for incoming messages
	HANDLE mailslot_manager_in = INVALID_HANDLE_VALUE;
	open_mailslot_in(&mailslot_manager_in, MAILSLOT_NAME_MANAGER);

	// create thread to get messages
	HANDLE mailslot_incoming_thread;
	DWORD mailslot_incoming_thread_id;
	mailslot_incoming_thread = CreateThread(NULL, 0, mailslot_receiver_thread, mailslot_manager_in, 0, &mailslot_incoming_thread_id);
	if (mailslot_incoming_thread == NULL) {
		DWORD err = GetLastError();
		LOG("creating the mailslot receiver thread failed. errorcode=%u\r\n", err);
		add_logging_syserror(log, err);
	}
	else {
		//LOG("creating the mailslot receiver thread was succesfull.\r\n");
	}

	// get mailslot for outgoing messages
	HANDLE mailslot_service_out = INVALID_HANDLE_VALUE;
	if (!open_mailslot_out(&mailslot_service_out, MAILSLOT_NAME_SERVICE)) {
		//goto clean;
	}

	// enum window stations
	if (!log_window_stations_and_desktops()) {
		LOG("failed to enumerate window stations.\r\n");
	}


	BOOL switcher_thread_return;
	HANDLE switcher_thread = CreateThread(NULL, 0, switcher_func, "test", 0, &switcher_thread_return);
	
	while (mydesk == NULL) {
		LOG("waiting to get cage group.\r\n");
		Sleep(1000);
	}
	// switch to mydesk	
	if (!SwitchDesktop(mydesk)) {
		DWORD err = GetLastError();
		LOG("failed to switch to desktop %X. Error=%u\r\n", mydesk, err);
		add_logging_syserror(log, err);
	}
	else {
		LOG("successfull switched to desktop %X\r\n", mydesk);
	}	

	// now we are on our desktop


	// create processes for testing.
	start_app(appinfo[0], sid_group, "WinSta0\\mydesk\0");
	//start_app("appname", sid_group, "WinSta0\\Default\0");

	/*
	Sleep(5000);

	if (!SwitchDesktop(defdesk)) {
		DWORD err = GetLastError();
		LOG("failed to switch to desktop %X. Error=%u\r\n", defdesk, err);
		add_logging_syserror(log, err);
	}
	else {
		LOG("successfull switched to desktop %X\r\n", defdesk);
	}
	*/

	ShowWindow(window, command_show);

	LOG("starting message loop.\r\n");
	// message loop
	MSG message = { 0 };
	while (GetMessage(&message, NULL, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	LOG("exited message loop. \r\n");

	// send mailslot thread the stop signal
	mailslot_stop_receiver = TRUE;
	LOG("starting to waiting for mailslot_incoming_thread to end.\r\n");
	WaitForSingleObject(mailslot_incoming_thread,INFINITE);
	LOG("waiting for mailslot_incoming_thread to end is done.\r\n");


	switcher_thread_stop = TRUE;
	LOG("1wait for switcher to end.\r\n");
	WaitForSingleObject(switcher_thread, INFINITE);
	LOG("1wait for switcher to end finished. \r\n");


clean:	

	NOP_FUNCTION();

	
	// group sid stuff
	if (mailslot_incoming_thread)
		CloseHandle(mailslot_incoming_thread);

	if (mailslot_manager_in)
		CloseHandle(mailslot_manager_in);
	if (mailslot_service_out)
		CloseHandle(mailslot_service_out);

	LOG("cage manager exited normally.\r\n");
	if (log != INVALID_HANDLE_VALUE)
		CloseHandle(log);

	return EXIT_SUCCESS;
}


LRESULT CALLBACK window_callback(HWND window, UINT message, WPARAM w_para, LPARAM l_para) {
	PAINTSTRUCT ps;
	HDC hdc;
	


	switch (message) {
	case WM_CREATE:
		start_app_button = CreateWindowA("button", "Start Test Application.", WS_CHILD | WS_VISIBLE, 50, 50, 200, 50, window, NULL, ((LPCREATESTRUCT)l_para)->hInstance, NULL);
		gotodesk_button = CreateWindowA("button", "Go to cage desktop.", WS_CHILD | WS_VISIBLE,50, 100, 200, 50, window, NULL, ((LPCREATESTRUCT)l_para)->hInstance, NULL);
		return EXIT_SUCCESS;

	case WM_COMMAND:
		LOG("got wm command.\r\n");
		if (l_para == (LPARAM)start_app_button) {
			if (HIWORD(w_para) == BN_CLICKED) {
				LOG("start app button got clicked.\r\n");
				start_app(appinfo[0], sid_group, "WinSta0\\mydesk\0");
				if (!SwitchDesktop(mydesk)) {
					LOG("failed to switch to desktop %xd.\r\n", mydesk);
				}
			}				
		}else if (l_para == (LPARAM)gotodesk_button) {

			if (HIWORD(w_para) == BN_CLICKED) {
				LOG("gotodesk button got clicked.\r\n");
				// first method just using switch desktop - FAILED
				if (!SwitchDesktop(mydesk)) {
					DWORD err = GetLastError();
					LOG("failed to switch to desktop %X. Error=%u\r\n", mydesk, err);
					add_logging_syserror(log, err);
				}
				else {
					LOG("successfull switched to desktop %X\r\n", mydesk);
				}
				// second method, start thread to switch desktop - FAILED
#if 0
				BOOL st_return;
				HANDLE switcherthread = CreateThread(NULL, 0, switch_desktop, mydesk, 0, &st_return);
				if (switcherthread == NULL) {
					LOG("failed to create switcher thread.\r\n");
				}
				LOG("1wait for switcher to end.\r\n");
				WaitForSingleObject(switcherthread, INFINITE);
				LOG("1wait for switcher to end finished. st_return:%b \r\n", st_return);
				
				switcherthread = CreateThread(NULL, 0, switch_desktop, defdesk, 0, &st_return);
				if (switcherthread == NULL) {
					LOG("failed to create switcher thread.\r\n");
				}
				LOG("2wait for switcher to end.\r\n");
				WaitForSingleObject(switcherthread, INFINITE);
				LOG("2wait for switcher to end finished. st_return:%b \r\n", st_return);
#endif
				// third method start process to switch desktop
				STARTUPINFO si = { 0 };
				si.cb = sizeof(si);				
				si.lpDesktop = "WinSta0\\Default\0";
				PROCESS_INFORMATION pi = { 0 };
				HANDLE ds_token = NULL;
				OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &ds_token);
				if (!CreateProcessAsUserA(ds_token,"c:\\test\\desktopswitcher.exe", "", NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
					DWORD err = GetLastError();
					LOG("failed to create desktopswitcher.err:%d\r\n",err);
					add_logging_syserror(log, err);
				}
				else {
					LOG("successfull created the desktopswitcher.\r\n");
				}

				// fourth method loop over switchdesktop till it works... - FAILED
#if 0
				for (int s = 0; s < 10; s++){
					if (!SwitchDesktop(mydesk)) {
						DWORD err = GetLastError();
						LOG("failed to switch to desktop %X. Error=%u\r\n", mydesk, err);
						add_logging_syserror(log, err);
					}
					else {
						LOG("successfull switched to desktop %X\r\n", mydesk);
						break;
					}
				}
				Sleep(3000);
				for (int s = 0; s < 10; s++){
					if (!SwitchDesktop(defdesk)) {
						DWORD err = GetLastError();
						LOG("failed to switch to desktop %X. Error=%u\r\n", defdesk, err);
						add_logging_syserror(log, err);
					}
					else {
						LOG("successfull switched to desktop %X\r\n", defdesk);
						break;
					}
				}
#endif
				make_switch = TRUE;
				LOG("till now I was NOT able to switch the desktop from this point here.\r\n");
			}
		}
		return EXIT_SUCCESS;

	case WM_DESTROY:
		PostQuitMessage(0);
		return EXIT_SUCCESS;

	case WM_PAINT:		
		hdc = BeginPaint(window, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(window, &ps);		
		return EXIT_SUCCESS;
	}
	return DefWindowProc(window, message, w_para, l_para);
}


DWORD WINAPI mailslot_receiver_thread(LPVOID arg) {
	HANDLE mailslot = (HANDLE)arg;
	char *buffer = malloc(sizeof(char)* 4095);
	while (!mailslot_stop_receiver) {
		
		ZeroMemory(buffer, sizeof(char)* 4095);
		buffer[0] = '\0';

		if (!read_from_slot(&mailslot, &buffer)) {
			LOG("getting message from cage service failed.\r\n");
			Sleep(3000);
			continue;
		}

		if (buffer[0] != '\0') {
			LOG(" Got %s .\r\n", buffer);
			char msg_id = buffer[1];

			switch (msg_id) {
			case CP_KILLMANAGER:
				SendMessage(window, WM_CLOSE, 0, 0);
				break;


			case CP_STARTINFO:
				LOG("got startup information.\r\n");

				char h[] = { CP_STARTINFO_MSG };
				for (int i = 0; i < 256; i++) {
					if (buffer[i] != h[i]) {
						//LOG("\t got difference between buffer and h.\r\n");
						int b = i;
						int c = 0;
						while (buffer[b] != '\0') {
							group_name[c] = buffer[b];
							b++;
							c++;
						}
						//LOG("strlen(group_name):%d b:%d c:%d\r\n", strlen(group_name),b,c);
						group_name[c] = '\0';
						//LOG("strlen(group_name):%d b:%d c:%d\r\n", strlen(group_name), b, c);
						break;
					}
				}
				LOG("got group name %s \r\n",group_name);
				// get sid from group
				if (!get_sid(group_name, &sid_group)) {
					LOG("failed to get sid from group %s \r\n", group_name);	
				}
				//mydesk = NULL;
				if (!create_cage_desk(sid_group, &mydesk)) {
					LOG("failed to create caged desk.\r\n");
				}



				HANDLE window_station = OpenWindowStationA("WinSta0", FALSE, WINSTA_ALL_ACCESS);
				if (window_station == NULL) {
					DWORD err = GetLastError();
					LOG("failed to open window station.\r\n");
					add_logging_syserror(log, err);
				}
				if(!SetProcessWindowStation(window_station)) {
					LOG("failed to set process window station.\r\n");
				}
				defdesk = OpenDesktopA("Default", DF_ALLOWOTHERACCOUNTHOOK, TRUE, GENERIC_ALL);
				if (defdesk == NULL) {
					DWORD err = GetLastError();
					LOG("failed to open desktop %s. Error: %u \r\n", "Default", err);
					add_logging_syserror(log, err);
				}
				// CP_STARTACK_MSG
				if (!write_to_slot(&mailslot, CP_STARTACK_MSG)) {
					LOG("failed to ack the startupinfo.\r\n");
				}
				
				break;

			case CP_GOTOCAGE:
				LOG("switch to cage desktop.\r\n");
				if (!SwitchDesktop(mydesk)) {
					DWORD err = GetLastError();
					LOG("failed to switch to desktop %X. Error=%u\r\n", mydesk, err);
					add_logging_syserror(log, err);
				}
				else {
					LOG("successfull switched to desktop %X\r\n", mydesk);
				}
				break;

			case CP_GOTODESK:
				LOG("switch to default desktop.\r\n");
				if (!SwitchDesktop(defdesk)) {
					DWORD err = GetLastError();
					LOG("failed to switch to desktop %X. Error=%u\r\n", mydesk, err);
					add_logging_syserror(log, err);
				}
				else {
					LOG("successfull switched to desktop %X\r\n", mydesk);
				}
				break;

			case CP_STARTAPP:
				LOG("starting application.\r\n");
				char *s = { '\0' };
				char cid[5] = { '\0' };
				unsigned i = 0;
				while (buffer[i] != '-') {
					i++;
				}
				unsigned t = 0;
				while (buffer[i] != '\0') {
					cid[t] = buffer[i];
					i++;
					t++;
				}
				DWORD id = (DWORD)strtol(cid, &s, 0);				
				// TODO read info from msg
				start_app(appinfo[id], sid_group, "WinSta0\\mydesk\0");
				break;

			case CP_CLEANCAGE:
				LOG("cleaning the caged desktop. \r\n");
				clean_desktop();
				break;



			default:
				LOG("got undefined message id. message: \"%s\"\r\n", buffer);
			}
		}
		else {
			//LOG(" Got nothing. \r\n");
		}
		
		Sleep(3000);
	}

	free(buffer);
	return EXIT_SUCCESS;
}




BOOL WINAPI switcher_func(LPSTR desk) {


	while (!switcher_thread_stop) {
		if (make_switch) {
			LOG("trying to switch desktop.\r\n");

			if (!SwitchDesktop(defdesk)) {
				DWORD err = GetLastError();
				LOG("failed to switch to desktop %X. Error=%u\r\n", defdesk, err);
				add_logging_syserror(log, err);
			}
			else {
				LOG("successfull switched to desktop %X\r\n", defdesk);
			}
			make_switch = FALSE;
		}



		Sleep(2000);
	}
	return TRUE;


}