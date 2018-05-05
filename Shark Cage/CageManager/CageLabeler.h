#include <Windows.h>
#include <windowsx.h>
#include <algorithm> 

using namespace std;

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

class CageLabeler
{
public:
	CageLabeler(STARTUPINFO *info);
	~CageLabeler();
private:
	bool CageLabeler::showCageWindow(LPSTARTUPINFO info);
	VOID CageLabeler::initGdipPlisLib();
	VOID displayTokenInCageWindow(HWND *hwnd);
};

CageLabeler::CageLabeler(STARTUPINFO *info)
{
	initGdipPlisLib();
	showCageWindow(info);
}

CageLabeler::~CageLabeler()
{
}

VOID CageLabeler::initGdipPlisLib()
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	std::cout << msg << std::endl;
	switch (msg)
	{
	case WM_CLOSE:
		std::cout << "Close" << std::endl;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		RECT rect;
		GetWindowRect(hwnd, &rect);
		ValidateRect(hwnd, &rect);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

bool CageLabeler::showCageWindow(LPSTARTUPINFO info)
{
	const wchar_t CLASS_NAME[] = L"Token window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = NULL;
	wc.lpszClassName = CLASS_NAME;

	if (RegisterClass(&wc) == 0)
	{
		std::wcout << L"Registering of class for WindowToken failed\n" << std::endl;
		return 1;
	}

	HWND hwnd = CreateWindowEx(
		WS_EX_LEFT | WS_EX_TOPMOST,
		CLASS_NAME,
		NULL,
		(WS_POPUPWINDOW | WS_THICKFRAME | WS_VISIBLE | WS_CLIPCHILDREN),
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		500,
		500,
		NULL,
		NULL,
		NULL,
		NULL);

	if (hwnd == NULL)
	{
		std::wcout << L"Creating Window failed\n" << std::endl;
		return 1;
	}

	// Remove the window title bar
	if (SetWindowLong(hwnd, GWL_STYLE, 0) == 0)
	{
		std::wcout << L"Failed to remove the titlebar Error" << GetLastError() << std::endl;
	}

	if (ShowWindow(hwnd, SW_SHOW))
	{
		std::wcout << L"Show window failed\n" << std::endl;
		return 1;
	}

	displayTokenInCageWindow(&hwnd);

	if (UpdateWindow(hwnd))
	{
		std::wcout << L"Update window failed\n" << std::endl;
		return 1;
	}

	MSG msg = {};
	BOOL bRetVal;
	while ((bRetVal = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRetVal == -1)
		{
			std::cout << "Error encountered in message loop!" << std::endl;
			return 1;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	std::cout << "Return from token display window creation" << std::endl;
	return 0;
}


VOID CageLabeler::displayTokenInCageWindow(HWND *hwnd)
{
	std::wcout << L"starting display image\n" << std::endl;

	HDC hdc = GetDC(*hwnd);

	Graphics graphics(hdc);
	Image image(L"C:\\Users\\Juli\\segeln.jpg"); // TODO Read path from config
	Pen pen(Color(255, 255, 0, 0), 2);
	graphics.DrawImage(&image, 10, 10);

	std::wcout << L"Finished display cage\n" << std::endl;
}
