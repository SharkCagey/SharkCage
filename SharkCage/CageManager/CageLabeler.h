#include <Windows.h>
#include <algorithm> 

using namespace std;

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

#include "../CageNetwork/NetworkManager.h"
#include "../CageNetwork/MsgManager.h"
#include "../CageNetwork/MsgService.h"
#include "base64.h"

#include <optional>

static bool DisplayTokenInCageWindow(HWND *hwnd);
static bool GetBottomFromMonitor(int &monitor_bottom);
static bool ShowExitButton(HWND &hwnd);
static bool ShowConfigMetadata(HWND &hwnd);

static HWND gotodesk_button;
static HWND app_name_title;
static HWND app_name_restart_button;
static HWND additional_app_app_title;
static HWND additional_app_restart_button;
static HWND app_hash_text_title;
static HWND app_hash_text;

static std::wstring app_names;
static std::wstring app_tokens;
static std::wstring app_hashs;
static std::optional<std::wstring> additional_app_name;
static int cage_size;

static HBRUSH h_brush = ::CreateSolidBrush(RGB(255, 255, 255));

NetworkManager network_manager(ExecutableType::MANAGER);

class CageLabeler
{
public:
	CageLabeler();
	CageLabeler(
		const std::wstring &app_name,
		const std::wstring &app_token,
		const std::wstring &app_hash,
		const std::optional<std::wstring> &additional_app,
		const int &cage_sizes);
	bool Init();
private:
	bool ShowCageWindow();
	void InitGdipPlisLib();
};


CageLabeler::CageLabeler()
{
}

CageLabeler::CageLabeler(
	const std::wstring &app_name,
	const std::wstring &app_token,
	const std::wstring &app_hash,
	const std::optional<std::wstring> &additional_app,
	const int &cage_sizes)
{
	InitGdipPlisLib();
	app_names = app_name;
	cage_size = cage_sizes;
	app_tokens = app_token;
	additional_app_name = additional_app;

	// truncate hash
	const std::string hash_string(app_hash.begin(), app_hash.end());
	auto sub_hash = hash_string.substr(0, 20);
	app_hashs = std::wstring(sub_hash.begin(), sub_hash.end());
}

void CageLabeler::InitGdipPlisLib()
{
	GdiplusStartupInput gdiplus_startup_input;
	ULONG_PTR gdiplus_token;
	::GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, NULL);
}

bool CageLabeler::Init()
{
	if (!ShowCageWindow())
	{
		std::cout << "Failed to show cage window" << std::endl;
		return false;
	}
	return true;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		ShowExitButton(hwnd);
		ShowConfigMetadata(hwnd);
		break;
	}
	case WM_COMMAND:
	{
		if (l_param == (LPARAM) gotodesk_button)
		{
			std::cout << "Close window" << std::endl;
			::DestroyWindow(hwnd);
			break;
		}
		else if (l_param == (LPARAM) app_name_restart_button)
		{
			if (!network_manager.Send(ManagerMessageToString(ManagerMessage::RESTART_MAIN_APP)))
			{
				std::cout << "Failed to send restart default app to Manager" << std::endl;
				break;
			}
			return ::DefWindowProc(hwnd, msg, w_param, l_param);
		}
		else if (l_param == (LPARAM) additional_app_restart_button)
		{
			if (!network_manager.Send(ManagerMessageToString(ManagerMessage::RESTART_ADDITIONAL_APP)))
			{
				std::cout << "Failed to send restart default app to Manager" << std::endl;
				break;
			}
			return ::DefWindowProc(hwnd, msg, w_param, l_param);
		}
		else
		{
			break;
		}
	}
	case WM_CLOSE:
	{
		std::cout << "Close window" << std::endl;
		::DestroyWindow(hwnd);
		break;
	}
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_PAINT:
	{
		DisplayTokenInCageWindow(&hwnd);
		return ::DefWindowProc(hwnd, msg, w_param, l_param);
	}
	case WM_CTLCOLORSTATIC:
	{
		if (additional_app_app_title == (HWND) l_param 
			|| app_name_title == (HWND) l_param
			|| app_hash_text_title == (HWND) l_param
			|| app_hash_text == (HWND) l_param
		)
		{
			HDC hdc_static = (HDC)w_param;
			::SetTextColor(hdc_static, RGB(0, 0, 0));
			::SetBkColor(hdc_static, RGB(255, 255, 255));
			return (INT_PTR)h_brush;
		}
	}
	default:
		return ::DefWindowProc(hwnd, msg, w_param, l_param);
	}

	return EXIT_SUCCESS;
}

bool CageLabeler::ShowCageWindow()
{
	const std::wstring CLASS_NAME = L"Token window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = NULL;
	wc.lpszClassName = CLASS_NAME.c_str();

	if (!::RegisterClass(&wc))
	{
		std::wcout << L"Registering of class for WindowToken failed" << std::endl;
		return false;
	}

	int bottom;
	if (!GetBottomFromMonitor(bottom))
	{
		std::wcout << L"Failed to get bottom of monitor" << std::endl;
		return false;
	}

	HWND hwnd = ::CreateWindowEx(
		WS_EX_LEFT | WS_EX_TOPMOST,
		CLASS_NAME.c_str(),
		L"",
		WS_POPUPWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		cage_size,
		bottom,
		NULL,
		NULL,
		NULL,
		NULL);

	if (hwnd == NULL)
	{
		std::wcout << L"Creating Window failed\n" << std::endl;
		return false;
	}

	// Remove the window title bar
	if (!::SetWindowLong(hwnd, GWL_STYLE, 0))
	{
		std::wcout << L"Failed to remove the titlebar Error" << ::GetLastError() << std::endl;
	}

	::ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};
	while (::GetMessage(&msg, NULL, 0, 0) > 0)
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	std::cout << "Return from token display window creation" << std::endl;
	return true;
}

static bool GetImageInputStreamFromBase64(IStream* p_stream)
{
	const std::string token_string(app_tokens.begin(), app_tokens.end());

	std::string decoded_image = base64_decode(token_string);

	DWORD image_size = decoded_image.length();

	HGLOBAL h_memory = ::GlobalAlloc(GMEM_MOVEABLE, image_size);
	if (h_memory == NULL)
	{
		std::cout << "Failed to allocate memory for token image. Error " << ::GetLastError() << std::endl;
		return false;
	}


	LPVOID p_image = ::GlobalLock(h_memory);
	if (p_image == NULL)
	{
		std::cout << "Failed to allocate memory for token image. Error " << ::GetLastError() << std::endl;
		return false;
	}

	::memcpy(p_image, decoded_image.c_str(), image_size);

	return ::CreateStreamOnHGlobal(h_memory, FALSE, &p_stream) ? S_OK : true, false;
}

static bool DisplayTokenInCageWindow(HWND *hwnd)
{
	std::wcout << L"starting display image" << std::endl;
	
	const std::string token_string(app_tokens.begin(), app_tokens.end());

	std::string decoded_image = base64_decode(token_string);

	DWORD image_size = decoded_image.length();

	HGLOBAL h_memory = ::GlobalAlloc(GMEM_MOVEABLE, image_size);
	if (h_memory == NULL)
	{
		std::cout << "Failed to allocate memory for token image. Error " << ::GetLastError() << std::endl;
		return false;
	}

	LPVOID p_image = ::GlobalLock(h_memory);
	if (p_image == NULL)
	{
		std::cout << "Failed to allocate memory for token image. Error " << ::GetLastError() << std::endl;
		return false;
	}

	::memcpy(p_image, decoded_image.c_str(), image_size);

	IStream* p_stream = NULL;
	if (::CreateStreamOnHGlobal(h_memory, FALSE, &p_stream) != S_OK)
	{
		std::cout << "Failed to create image stream from string for cage token. Error " << ::GetLastError() << std::endl;
		return false;
	}

	HDC hdc = ::GetDC(*hwnd);

	Graphics graphics(hdc);
	Image image(p_stream);

	double available_width = (double) cage_size, available_height = (double) cage_size;
	double image_height = (double) image.GetHeight() * (available_width / (double) image.GetWidth());
	double image_width = available_width;

	if (image_height > available_height)
	{
		image_width = image_width * (available_height / image_height);
		image_height = available_height;
	}

	graphics.DrawImage(&image, 0, 0, (int)image_width, (int)image_height);

	std::wcout << L"Finished display cage" << std::endl;

	return true;
}

// Write util to remove this duplicated function (same in FullWorkArea.h)
static bool GetBottomFromMonitor(int &monitor_bottom)
{
	HMONITOR monitor = ::MonitorFromWindow(NULL, MONITOR_DEFAULTTONEAREST);
	if (monitor == NULL)
	{
		std::cout << "Failed to get MonitorFromWindow. Error " << ::GetLastError() << std::endl;
		return false;
	}

	MONITORINFO monitor_info;
	monitor_info.cbSize = sizeof(monitor_info);

	if (!::GetMonitorInfo(monitor, &monitor_info))
	{
		std::cout << "Failed to GetMonitorInfo. Error " << ::GetLastError() << std::endl;
		return false;
	}

	monitor_bottom = monitor_info.rcMonitor.bottom;

	return true;
}

static bool ShowExitButton(HWND &hwnd)
{
	int bottom;
	if (!GetBottomFromMonitor(bottom))
	{
		std::wcout << L"Failed to get bottom of monitor" << std::endl;
		return false;
	}

	gotodesk_button = ::CreateWindowEx(
		NULL,
		L"BUTTON",
		L"Exit",
		WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10,
		bottom - 44,
		cage_size - 23,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);


	if (gotodesk_button != NULL)
	{
		::SendMessage(gotodesk_button, BM_SETIMAGE, NULL, NULL);
	}
	else
	{
		std::wcout << L"Failed to create logout button Err " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}

static bool ShowConfigMetadata(HWND &hwnd)
{
	app_name_title = ::CreateWindowEx(
		NULL,
		TEXT("STATIC"),
		app_names.c_str(),
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		cage_size + 115,
		150,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	app_name_restart_button = ::CreateWindowEx(
		NULL,
		L"BUTTON",
		L"Restart",
		WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		cage_size - 112,
		cage_size + 106,
		100,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	if (additional_app_name.has_value())
	{
		additional_app_app_title = ::CreateWindowEx(
			NULL,
			TEXT("STATIC"),
			additional_app_name.value().c_str(),
			SS_LEFT | WS_VISIBLE | WS_CHILD,
			10,
			cage_size + 155,
			150,
			34,
			hwnd,
			NULL,
			NULL,
			NULL);

		additional_app_restart_button = ::CreateWindowEx(
			NULL,
			L"BUTTON",
			L"Restart",
			WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			cage_size - 112,
			cage_size + 148,
			100,
			34,
			hwnd,
			NULL,
			NULL,
			NULL);
	}

	app_hash_text_title = ::CreateWindowEx(
		NULL,
		TEXT("STATIC"),
		L"App SHA-512 (truncated):",
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		cage_size + 250,
		cage_size - 20,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	app_hash_text = ::CreateWindowEx(
		NULL,
		TEXT("STATIC"),
		app_hashs.c_str(),
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		cage_size + 275,
		cage_size - 30,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	if (app_name_title != NULL && app_name_restart_button != NULL)
	{
		HFONT default_font = ::CreateFont(
			17,
			0,
			0,
			0,
			FW_MEDIUM,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DRAFT_QUALITY,
			DEFAULT_PITCH | FF_SWISS,
			NULL);

		HFONT italic_font = ::CreateFont(
			17,
			0,
			0,
			0,
			FW_MEDIUM,
			TRUE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_SWISS,
			NULL);

		::SendMessage(app_name_title, WM_SETFONT, (WPARAM) default_font, TRUE);
		::SendMessage(app_name_restart_button, BM_SETIMAGE, NULL, NULL);
		::SendMessage(app_hash_text_title, WM_SETFONT, (WPARAM) default_font, TRUE);
		::SendMessage(app_hash_text, WM_SETFONT, (WPARAM) italic_font, TRUE);

		if (additional_app_name.has_value())
		{
			::SendMessage(additional_app_app_title, WM_SETFONT, (WPARAM)default_font, TRUE);
			::SendMessage(additional_app_restart_button, BM_SETIMAGE, NULL, NULL);
		}
	}
	else
	{
		std::wcout << L"Failed to create logout button Err " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}