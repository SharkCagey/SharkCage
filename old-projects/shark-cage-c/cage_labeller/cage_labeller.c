///
/// \file cage_labeller.c
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage labeller - main module
///

#include <Windows.h>
#include <shellapi.h>
#include <stdio.h>

#include "cage_lib.h" 

BOOL LoadBitmapFromBMPFile(LPTSTR szFileName, HBITMAP *phBitmap, HPALETTE *phPalette);
LRESULT CALLBACK window_callback(HWND window, UINT message, WPARAM w_para, LPARAM l_para);

#if 0
BOOL WINAPI switcher_func(LPSTR desk);
BOOL make_switch = FALSE;
BOOL switcher_thread_stop = FALSE;
#endif

HANDLE log;
HANDLE pic_handle = NULL;
HWND gotodesk_button = NULL;
HDESK defdesk = NULL;
HDESK mydesk = NULL;

APPLICATION_INFO appinfo = { 0 };
//char pic_path[512] = { '\0' };

HANDLE mailslot_labeller_in = NULL;

DWORD WINAPI mailslot_receiver_thread(LPVOID arg);

BOOL mailslot_stop_receiver = FALSE;
BOOL got_app_info = FALSE;
BOOL got_app_pic = FALSE;


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous_instance, PWSTR command_line, int command_show) {
	init_logging(&log, "labeller");

	const char window_class_name[] = "cage_labeller";
	WNDCLASS window_class = { 0 };
	window_class.lpfnWndProc = window_callback;
	window_class.hInstance = instance;
	window_class.lpszClassName = window_class_name;
	window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 5);


	RegisterClass(&window_class);	

	HWND bar_handle = CreateWindowEx(0, window_class_name, "cage_labeller", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, instance, NULL);
	if (bar_handle == NULL) {
		DWORD err = GetLastError();
		LOG("creating the app bar window failed. err:%d\r\n",err);
		add_logging_syserror(log, err);
		goto clean;
	}


	// open incoming mailslot
	open_mailslot_in(&mailslot_labeller_in, MAILSLOT_NAME_LABELLER);

	HANDLE mailslot_incoming_thread;
	DWORD mailslot_incoming_thread_id;
	mailslot_incoming_thread = CreateThread(NULL, 0, mailslot_receiver_thread, mailslot_labeller_in, 0, &mailslot_incoming_thread_id);
	if (mailslot_incoming_thread == NULL) {
		DWORD err = GetLastError();
		LOG("creating the mailslot receiver thread failed. errorcode=%u\r\n", err);
		add_logging_syserror(log, err);
	}
	else {
		//LOG("creating the mailslot receiver thread was succesfull.\r\n");
	}
	
	// wait for application info
	while (!got_app_info) {
		Sleep(200);
	}



	// wait for secret picture or order it ?
	while (!got_app_pic) {
		Sleep(200);
		got_app_pic = TRUE;
		//printf_s(pic_path, strlen("secretpicture.bmp\0"), "secretpicture.bmp\0");
	}


	

	// make the window an appbar
	APPBARDATA bar_data;
	ZeroMemory(&bar_data,sizeof(APPBARDATA));
	bar_data.cbSize = sizeof(APPBARDATA);
	bar_data.hWnd = bar_handle;
	if (!SHAppBarMessage(ABM_NEW, &bar_data)) {
		LOG("creating the app bar failed. \r\n");
		//goto clean;
	}
	bar_data.uEdge = ABE_TOP;
	bar_data.rc.left = 0;
	bar_data.rc.top = 0;
	int sys_width = GetSystemMetrics(SM_CXSCREEN);
	int sys_height = GetSystemMetrics(SM_CYSCREEN);
	bar_data.rc.right = sys_width;
	bar_data.rc.bottom = 200;	
	LOG("sys_width:%d sys_height:%d \r\n",sys_width,sys_height);
	// get position of appbar
	if (!SHAppBarMessage(ABM_QUERYPOS, &bar_data)) {
		LOG("querying the app bar position failed. \r\n");
		//goto clean;
	}
	LOG("1left=%4d top=%4d right=%4d bottom=%4d \r\n", bar_data.rc.left, bar_data.rc.top, bar_data.rc.right, bar_data.rc.bottom);
	if (!SHAppBarMessage(ABM_SETPOS, &bar_data)) {
		LOG("setting the app bar position failed. \r\n");
		//goto clean;
	}
	LOG("2left=%4d top=%4d right=%4d bottom=%4d \r\n", bar_data.rc.left, bar_data.rc.top, bar_data.rc.right, bar_data.rc.bottom);
	// move and resize window to appbar position
	if (!MoveWindow(bar_data.hWnd, bar_data.rc.left, bar_data.rc.top, bar_data.rc.right, bar_data.rc.bottom, TRUE)) {
		DWORD err = GetLastError();
		LOG("moving the window failed. err=%u\r\n",err);
		add_logging_syserror(log, err);
	}
	

	defdesk = OpenDesktopA("Default", DF_ALLOWOTHERACCOUNTHOOK, TRUE, GENERIC_ALL);
	if (defdesk == NULL) {
		DWORD err = GetLastError();
		LOG("failed to open desktop %s. Error: %u \r\n", "Default", err);
		add_logging_syserror(log, err);
	}
	mydesk = GetThreadDesktop(GetCurrentThreadId());
	if(mydesk==NULL) {
		LOG("failed to open mydesk.\r\n");
	}

#if 0
	BOOL switcher_thread_return;
	HANDLE switcher_thread = CreateThread(NULL,0,switcher_func,"test",0,&switcher_thread_return);
#endif


	ShowWindow(bar_handle, command_show);

	LOG("starting message loop.\r\n");
	// message loop
	MSG message = { 0 };
	while (GetMessage(&message, NULL, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	LOG("exited message loop. \r\n");


#if 0
	switcher_thread_stop = TRUE;
	LOG("1wait for switcher to end.\r\n");
	WaitForSingleObject(switcher_thread, INFINITE);
	LOG("1wait for switcher to end finished. st_return:%b \r\n", switcher_thread_return);
#endif

clean:
	mailslot_stop_receiver = TRUE;
	LOG("starting to waiting for mailslot_incoming_thread to end.\r\n");
	WaitForSingleObject(mailslot_incoming_thread, INFINITE);
	LOG("waiting for mailslot_incoming_thread to end is done.\r\n");
	// TODO write to slot that labeller is finished -> manager
	LOG("todo: write to manager that we are done.\r\n");
	if (!SHAppBarMessage(ABM_REMOVE, &bar_data)) {
		LOG("removing the app bar failed. \r\n");
	}

	if (log)
		CloseHandle(log);
	return EXIT_SUCCESS;
}



LRESULT CALLBACK window_callback(HWND window, UINT message, WPARAM w_para, LPARAM l_para) {
	HDC hdc, hdc_mem;
	PAINTSTRUCT ps;
	ZeroMemory(&ps, sizeof(PAINTSTRUCT));

	BITMAP pic;
	ZeroMemory(&pic, sizeof(BITMAP));
	LPCSTR pic_path = { "secretpicture.bmp\0" };

	switch (message) {
	case WM_CREATE:
		LOG("got wm_create.\r\n");
		gotodesk_button = CreateWindowA("button", "Go to default desktop.", WS_CHILD | WS_VISIBLE, 300,10, 200, 50, window, NULL, ((LPCREATESTRUCT)l_para)->hInstance, NULL);
		
		pic_handle = LoadImageA(0, pic_path, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		if (pic_handle == NULL) {
			DWORD err = GetLastError();
			LOG("failed to load image %s. err=%u\r\n", pic_path,err);
			add_logging_syserror(log,err);
			MessageBox(window,"Bild konnte nicht geladen werden.","Error",0);
			return EXIT_FAILURE;
		}
		
		break;

	case WM_COMMAND:
		LOG("got wm command.\r\n");

		if (l_para == (LPARAM)gotodesk_button) {
			if (HIWORD(w_para) == BN_CLICKED) {
				LOG("gotodesk button got clicked.\r\n");				
				if (!SwitchDesktop(defdesk)) {
					DWORD err = GetLastError();
					LOG("failed to switch to desktop %X. Error=%u\r\n", defdesk, err);
					add_logging_syserror(log, err);
				}
				else {
					LOG("successfull switched to desktop %X\r\n", defdesk);
					// find open windows on this desk
					
					char **window_list = alloc_list(MAX_WINDOW_ENTRIES,MAX_WINDOW_NAME);

					if (!EnumDesktopWindows(mydesk, enum_window_callback, (LPARAM)window_list)) {
						LOG("failed to enumerate all desktop windows.\r\n");
					}
					LOG("Window List for mydesk:\r\n");
					for (int i = 0; i < MAX_WINDOW_ENTRIES; i++) {
						if (window_list[i][0] != '\0') {
							LOG("\twindow: %s\r\n",window_list[i]);
							if (strncmp(appinfo.window_class_name, window_list[i], strlen(appinfo.window_class_name)) == 0) {
								LOG("\t\tfound matching entry. %s == %s when n:%d\r\n", appinfo.window_class_name, window_list[i], strlen(appinfo.window_class_name));
							}
						}
					}
					free_list(MAX_WINDOW_ENTRIES, window_list);
					HWND app = FindWindow(appinfo.window_class_name, NULL);
					if (app == NULL) {
						DWORD err = GetLastError();
						LOG("failed to find window %s. err:%d\r\n", appinfo.window_class_name, err);
						add_logging_syserror(log, err);
					}
					LOG("sending close message to window %x.\r\n",app);
					SendMessage(app, WM_SYSCOMMAND, SC_CLOSE, 0);

					LOG("sending close message to own window.\r\n");
					SendMessageA(window, WM_SYSCOMMAND, SC_CLOSE, 0);
				}
			}
		}
		return EXIT_SUCCESS;

	case WM_DESTROY:
		DeleteObject(pic_handle);
		PostQuitMessage(0);
		break;

	case WM_PAINT:
		NOP_FUNCTION;

		hdc = BeginPaint(window, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 2));

		TextOutA(hdc, GetSystemMetrics(SM_CXSCREEN)/2, 100, appinfo.name,6);
		hdc_mem = CreateCompatibleDC(hdc);
		HGDIOBJ pic_old = SelectObject(hdc_mem, pic_handle);
		GetObject(pic_handle, sizeof(BITMAP), &pic);		
		BitBlt(hdc, 0, 0, pic.bmWidth, pic.bmHeight, hdc_mem, 0, 0, SRCCOPY);
		SelectObject(hdc_mem,pic_old);
		DeleteDC(hdc_mem);

		EndPaint(window, &ps);
		break;

	default:
		return DefWindowProc(window, message, w_para, l_para);
	}
	return EXIT_SUCCESS;
}



// https://support.microsoft.com/en-us/kb/158898/de
BOOL LoadBitmapFromBMPFile(LPTSTR szFileName, HBITMAP *phBitmap, HPALETTE *phPalette) {

	BITMAP  bm;

	*phBitmap = NULL;
	*phPalette = NULL;

	// Use LoadImage() to get the image loaded into a DIBSection
	*phBitmap = (HBITMAP)LoadImage(NULL, szFileName, IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE);
	if (*phBitmap == NULL)
		return FALSE;

	// Get the color depth of the DIBSection
	GetObject(*phBitmap, sizeof(BITMAP), &bm);
	// If the DIBSection is 256 color or less, it has a color table
	if ((bm.bmBitsPixel * bm.bmPlanes) <= 8)
	{
		HDC           hMemDC;
		HBITMAP       hOldBitmap;
		RGBQUAD       rgb[256];
		LPLOGPALETTE  pLogPal;
		WORD          i;

		// Create a memory DC and select the DIBSection into it
		hMemDC = CreateCompatibleDC(NULL);
		hOldBitmap = (HBITMAP)SelectObject(hMemDC, *phBitmap);
		// Get the DIBSection's color table
		GetDIBColorTable(hMemDC, 0, 256, rgb);
		// Create a palette from the color tabl
		pLogPal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE)+(256 * sizeof(PALETTEENTRY)));
		pLogPal->palVersion = 0x300;
		pLogPal->palNumEntries = 256;
		for (i = 0; i<256; i++)
		{
			pLogPal->palPalEntry[i].peRed = rgb[i].rgbRed;
			pLogPal->palPalEntry[i].peGreen = rgb[i].rgbGreen;
			pLogPal->palPalEntry[i].peBlue = rgb[i].rgbBlue;
			pLogPal->palPalEntry[i].peFlags = 0;
		}
		*phPalette = CreatePalette(pLogPal);
		// Clean up
		free(pLogPal);
		SelectObject(hMemDC, hOldBitmap);
		DeleteDC(hMemDC);
	}
	else   // It has no color table, so use a halftone palette
	{
		HDC    hRefDC;

		hRefDC = GetDC(NULL);
		*phPalette = CreateHalftonePalette(hRefDC);
		ReleaseDC(NULL, hRefDC);
	}
	return TRUE;

}

#if 0
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
#endif



DWORD WINAPI mailslot_receiver_thread(LPVOID arg) {
	HANDLE mailslot = (HANDLE)arg;
	char *buffer = malloc(sizeof(char)* 4095);
	while (!mailslot_stop_receiver) {

		ZeroMemory(buffer, sizeof(char)* 4095);
		buffer[0] = '\0';
		memset(buffer,'\0',sizeof(char)*4095);

		if (!read_from_slot(&mailslot, &buffer)) {
			LOG("getting message from cage service failed.\r\n");
			Sleep(3000);
			continue;
		}

		if (buffer[0] != '\0') {
			LOG(" Got %s .\r\n", buffer);
			char msg_id = buffer[1];

			switch (msg_id) {
			case CP_APPINFO:
				LOG("received appinfo message.\r\n");		
				//DWORD appid;
				char appidchar[5];
				char appname[80];
				char windowname[80];

				int i = 0;
				while(buffer[i] != '-') {
					//LOG("buffer[%d]:%c A\r\n",i,buffer[i]);
					i++;
				}
				i++;
				//LOG("buffer[%d]:%c B\r\n", i, buffer[i]);
				int t = 0;
				while (buffer[i] != ' ') {
					//LOG("buffer[%d]:%c C\r\n", i, buffer[i]);
					appidchar[t] = buffer[i];
					i++;
					t++;
				}
				appidchar[t] = '\0';
				//i++;
				//LOG("buffer[%d]:%c appidchar:%s \r\n",i,buffer[i],appidchar);				
				while(buffer[i] != '-') {
					i++;
					//LOG("buffer[%d]:%c D\r\n", i, buffer[i]);
				}
				i++;
				//LOG("buffer[%d]:%c E\r\n", i, buffer[i]);
				t = 0;
				while (buffer[i] != ' ') {
					//LOG("buffer[%d]:%c F\r\n", i, buffer[i]);
					appname[t] = buffer[i];
					i++;
					t++;
				}
				appname[t] = '\0';
				//LOG("buffer[%d]:%c G\r\n", i, buffer[i]);
				i++;
				//LOG("buffer[%d]:%c appidchar:%s appname:%s\r\n",i,buffer[i],appidchar,appname);
				while(buffer[i] != '-') {
					//LOG("buffer[%d]:%c H\r\n", i, buffer[i]);
					i++;
				}
				i++;
				//LOG("buffer[%d]:%c I\r\n", i, buffer[i]);
				t = 0;
				while (buffer[i] != '\0') {
					//LOG("%d buffer[%d]:%cJ\r\n",buffer[i], i, buffer[i]);
					windowname[t] = buffer[i];
					i++;
					t++;
				}
				windowname[t] = '\0';
				//LOG("buffer[%d]:%c appidchar:%s appname:%s windowname:%s\r\n",i,buffer[i],appname,windowname);
				//LOG("windowname:%s\r\n",windowname);char *stop = { '\0' };
				char *s = {'\0'};
				appinfo.id = (DWORD)strtol(appidchar, &s, 0);
				sprintf_s(appinfo.name, strlen(appname) + 1, appname);
				sprintf_s(appinfo.window_class_name,strlen(windowname)+1,windowname);
				LOG("received app ainfo.\r\n");
				got_app_info = TRUE;

#if 0
				char h[80] = { '\0' };
				unsigned j = 0;
				unsigned c = 0;
				for (unsigned i = 15; i < strlen(buffer); i++,j++) {
					if (buffer[i] != ' ' && buffer[i] != '-') {
						LOG("h:%s\r\n",h);
						h[j] = buffer[i];
						
					}
					else {
						j--;
					}
					LOG("i:%2d j:%2d c:%d h:%s \r\n",i,j,c,h);
					if (buffer[i + 1] == ' ' && buffer[i + 2] == '-'&& c == 0) {
						char *stop = { '\0' };
						appinfo.id = (DWORD)strtol(h, &stop, 0);
						for (unsigned k = 0; k < j; k++) {
							h[k] = '\0';
						}
						c++;
						j = 0;
						LOG("reseted stuff.\r\n");
					} else if (buffer[i + 1] == ' ' && buffer[i + 2] == '-'&& c == 1) {
						LOG("buffer:%s i:%2d %s\r\n",buffer,i,&buffer[i]);
						for (unsigned k = 0; k < j; k++) {
							appinfo.name[k] = h[k];
							h[k] = '\0';
							//LOG("appinof.name[%d]:%c \r\n", k, appinfo.name[k]);
						}
						c++;
						j = 0;
					} else if (c == 2 && buffer[i] == '\0') {
						for (unsigned k = 0; k < j; k++) {
							appinfo.window_class_name[k] = h[k];
							h[k] = '\0';
						}
						c++;
						j = 0;
					}					
				}
				got_app_info = TRUE;
				LOG("%d %s %s\r\n",appinfo.id,appinfo.name,appinfo.window_class_name);
#endif
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
