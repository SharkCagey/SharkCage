#include <Windows.h>
#include <windowsx.h>
#include <algorithm> 

using namespace std;

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

VOID displayTokenInCageWindow(HWND *hwnd);

class CageLabeler
{
public:
	CageLabeler(STARTUPINFO *info);
	~CageLabeler();
private:
	bool CageLabeler::showCageWindow(LPSTARTUPINFO info);
	VOID CageLabeler::initGdipPlisLib();
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
	case WM_CREATE:
	{
		break;
	}
	case WM_CLOSE:
		std::cout << "Close" << std::endl;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
	{
		displayTokenInCageWindow(&hwnd);

		RECT rect;
		GetWindowRect(hwnd, &rect);
		ValidateRect(hwnd, &rect);

		break;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return EXIT_SUCCESS;
}

bool CageLabeler::showCageWindow(LPSTARTUPINFO info)
{
	const wchar_t CLASS_NAME[] = L"Token window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = NULL;
	wc.lpszClassName = CLASS_NAME;

	if (RegisterClass(&wc) == false)
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
	if (SetWindowLong(hwnd, GWL_STYLE, 0) == false)
	{
		std::wcout << L"Failed to remove the titlebar Error" << GetLastError() << std::endl;
	}

	if (ShowWindow(hwnd, SW_SHOW))
	{
		std::wcout << L"Show window failed\n" << std::endl;
		return 1;
	}

	if (UpdateWindow(hwnd))
	{
		std::wcout << L"Update window failed\n" << std::endl;
		return 1;
	}

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	std::cout << "Return from token display window creation" << std::endl;
	return 0;
}


VOID displayTokenInCageWindow(HWND *hwnd)
{
	std::wcout << L"starting display image\n" << std::endl;

	HDC hdc = GetDC(*hwnd);

	Graphics graphics(hdc);
	Image image(L"C:\\Users\\Juli\\segeln.jpg"); // TODO Read path from config
	Pen pen(Color(255, 255, 0, 0), 2);
	graphics.DrawImage(&image, 10, 10);

	std::wcout << L"Finished display cage\n" << std::endl;
}
