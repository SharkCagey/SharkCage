#include <Windows.h>
#include <windowsx.h>
#include <algorithm> 

using namespace std;

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

VOID DisplayTokenInCageWindow(HWND *hwnd);
int GetBottomFromMonitor();

HWND gotodesk_button;
HHOOK hHook;

class CageLabeler
{
public:
	CageLabeler();
	~CageLabeler();
	bool Init();
private:
	bool CageLabeler::ShowCageWindow();
	VOID CageLabeler::InitGdipPlisLib();
};

CageLabeler::CageLabeler()
{
	InitGdipPlisLib();
}

CageLabeler::~CageLabeler()
{
}

VOID CageLabeler::InitGdipPlisLib()
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	std::cout << msg << std::endl;
	switch (msg)
	{
	case WM_CREATE:
	{
		gotodesk_button = CreateWindowEx(
			NULL,
			L"BUTTON",
			L"Exit",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			10,
			900,
			100,
			100,
			hwnd,
			NULL,
			(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
			NULL);

		if (gotodesk_button != NULL)
		{
			SendMessage(gotodesk_button, BM_SETIMAGE, NULL, NULL);
		}
		else
		{
			std::wcout << L"Failed to create logout button Err " << GetLastError() << std::endl;
		}
		break;
	}
	case WM_COMMAND:
	{
		if (l_param == (LPARAM)gotodesk_button) {
			// Close token
		}
		else {
			break;
		}
	}
	case WM_CLOSE:
		std::cout << "Close window" << std::endl;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
	{
		DisplayTokenInCageWindow(&hwnd);
	}
	default:
		return DefWindowProc(hwnd, msg, w_param, l_param);
	}

	return EXIT_SUCCESS;
}

bool CageLabeler::ShowCageWindow()
{
	const wchar_t CLASS_NAME[] = L"Token window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = NULL;
	wc.lpszClassName = CLASS_NAME;

	if (RegisterClass(&wc) == false)
	{
		std::wcout << L"Registering of class for WindowToken failed" << std::endl;
		return false;
	}

	HWND hwnd = CreateWindowEx(
		WS_EX_LEFT | WS_EX_TOPMOST,
		CLASS_NAME,
		L"",
		(WS_POPUPWINDOW | WS_THICKFRAME | WS_VISIBLE | WS_CLIPCHILDREN),
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		500,
		GetBottomFromMonitor(),
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
	if (SetWindowLong(hwnd, GWL_STYLE, 0) == false)
	{
		std::wcout << L"Failed to remove the titlebar Error" << GetLastError() << std::endl;
	}

	if (ShowWindow(hwnd, SW_SHOW))
	{
		std::wcout << L"Show window failed\n" << std::endl;
		return false;
	}

	MSG msg = { 0 };

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	std::cout << "Return from token display window creation" << std::endl;
	return true;
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


VOID DisplayTokenInCageWindow(HWND *hwnd)
{
	std::wcout << L"starting display image\n" << std::endl;

	HDC hdc = GetDC(*hwnd);

	Graphics graphics(hdc);
	Image image(L"C:\\Users\\Juli\\segeln.jpg"); // TODO Read path from config
	Pen pen(Color(255, 255, 0, 0), 2);
	graphics.DrawImage(&image, 10, 10);

	std::wcout << L"Finished display cage\n" << std::endl;
}

// Write util to remove this dublicated function (same in FullWorkArea.h)
int GetBottomFromMonitor()
{
	HMONITOR monitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTONEAREST);
	if (monitor == NULL)
	{
		std::cout << "Failed to get MonitorFromWindow. Error " << GetLastError() << std::endl;
		return -1;
	}

	MONITORINFO monitor_info;
	monitor_info.cbSize = sizeof(monitor_info);

	if (GetMonitorInfo(monitor, &monitor_info) == false)
	{
		std::cout << "Failed to GetMonitorInfo. Error " << GetLastError() << std::endl;
		return -1;
	}

	return monitor_info.rcMonitor.bottom;
}