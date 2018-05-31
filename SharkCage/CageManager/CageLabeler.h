#include <Windows.h>
#include <algorithm> 

using namespace std;

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

void DisplayTokenInCageWindow(HWND *hwnd);
bool GetBottomFromMonitor(int &monitor_bottom);
bool ShowExitButton(HWND &hwnd);
bool ShowConfigMetadata(HWND &hwnd);

HWND gotodesk_button;
HWND keepass_title;
HWND cryptomator_title;

static HBRUSH h_brush = ::CreateSolidBrush(RGB(255, 255, 255));

class CageLabeler
{
public:
	CageLabeler();
	bool Init();
private:
	bool ShowCageWindow();
	void InitGdipPlisLib();
};

CageLabeler::CageLabeler()
{
	InitGdipPlisLib();
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
		if (l_param == (LPARAM)gotodesk_button)
		{
			std::cout << "Close window" << std::endl;
			::DestroyWindow(hwnd);
			break;
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
		if (cryptomator_title == (HWND)l_param || keepass_title == (HWND)l_param)
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
		500,
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

void DisplayTokenInCageWindow(HWND *hwnd)
{
	std::wcout << L"starting display image" << std::endl;

	HDC hdc = ::GetDC(*hwnd);

	Graphics graphics(hdc);
	Image image(L"C:\\Users\\Juli\\segeln.jpg"); // TODO change path to a file in your env and later, read path from config

	double available_width = 500, available_height = 500;
	double image_height = (double) image.GetHeight() * (available_width / (double) image.GetWidth());
	double image_width = available_width;

	if (image_height > available_height)
	{
		image_width = image_width * (available_height / image_height);
		image_height = available_height;
	}

	graphics.DrawImage(&image, 0, 0, (int)image_width, (int)image_height);

	std::wcout << L"Finished display cage" << std::endl;
}

// Write util to remove this duplicated function (same in FullWorkArea.h)
bool GetBottomFromMonitor(int &monitor_bottom)
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

bool ShowExitButton(HWND &hwnd)
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
		-1,
		bottom - 80,
		500,
		80,
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

bool ShowConfigMetadata(HWND &hwnd)
{
	keepass_title = ::CreateWindowEx(
		NULL,
		TEXT("STATIC"),
		L"Keepass",
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		0,
		615,
		150,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	HWND keepass_button = ::CreateWindowEx(
		NULL,
		L"BUTTON",
		L"Restart",
		WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		350,
		608,
		150,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	cryptomator_title = ::CreateWindowEx(
		NULL,
		TEXT("STATIC"),
		L"Cryptomator",
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		0,
		655,
		150,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	HWND cryptomator_button = ::CreateWindowEx(
		NULL,
		L"BUTTON",
		L"Restart",
		WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		350,
		648,
		150,
		34,
		hwnd,
		NULL,
		NULL,
		NULL);

	if (keepass_title != NULL && keepass_button != NULL 
		&& cryptomator_title != NULL && cryptomator_button != NULL)
	{
		HFONT h_font = ::CreateFont(
			18,
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
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_SWISS,
			NULL);

		::SendMessage(keepass_title, WM_SETFONT, (WPARAM) h_font, TRUE);
		::SendMessage(keepass_button, BM_SETIMAGE, NULL, NULL);
		::SendMessage(cryptomator_title, WM_SETFONT, (WPARAM) h_font, TRUE);
		::SendMessage(cryptomator_button, BM_SETIMAGE, NULL, NULL);
	}
	else
	{
		std::wcout << L"Failed to create logout button Err " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}