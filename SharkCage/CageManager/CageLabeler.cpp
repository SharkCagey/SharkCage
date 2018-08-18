#include "stdafx.h"

#include <Windows.h>
#include <algorithm> 

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

#include "../SharedFunctionality/NetworkManager.h"
#include "CageLabeler.h"
#include "base64.h"

#include <optional>
#include <cstring>

static bool DisplayTokenInCageWindow(HDC hdc);
static bool GetBottomFromMonitor(int &monitor_bottom);
static bool ShowExitButton(HWND &hwnd);
static bool ShowConfigMetadata(HWND &hwnd);

static HWND labeler_window;
static HWND background_window;
static HWND labeler_background_window;

static HWND gotodesk_button;
static HWND app_activate_button;
static HWND additional_app_activate_button;
static HWND debug_warning;

static COLORREF transparent_color = RGB(1, 1, 1);

static std::wstring app_name;
static std::wstring app_token;
static std::wstring app_hash;
static std::optional<std::wstring> additional_app_name;
static bool restrict_closing;
static int labeler_width;
static HANDLE activate_app;
static std::optional<HANDLE> activate_additional_app;

CageLabeler::CageLabeler(
	const CageData &cage_data,
	const int &_cage_width,
	const std::wstring &_window_class_name)
{
	InitGdipPlisLib();
	app_name = cage_data.app_name;
	labeler_width = _cage_width;
	app_token = cage_data.app_token;
	additional_app_name = cage_data.additional_app_name;
	restrict_closing = cage_data.restrict_closing;
	window_class_name = _window_class_name;
	activate_app = cage_data.activate_app;
	activate_additional_app = cage_data.activate_additional_app;

	// truncate hash
	app_hash = cage_data.app_hash.substr(0, 20);
}

void CageLabeler::InitGdipPlisLib()
{
	GdiplusStartupInput gdiplus_startup_input;
	ULONG_PTR gdiplus_token;
	::GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, nullptr);
}

bool CageLabeler::Init()
{
	if (!ShowLabelerWindow())
	{
		std::cout << "Failed to show cage window" << std::endl;
		return false;
	}
	return true;
}

LRESULT CALLBACK WndProc_background(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		if (hwnd != background_window)
		{
			break;
		}

		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hwnd, &ps);
	
		wchar_t wallpaper_path[MAX_PATH];
		auto got_wp = ::SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, wallpaper_path, 0);

		if (got_wp)
		{
			Gdiplus::Image wallpaper_image(wallpaper_path);
			Gdiplus::Graphics graphics(hdc);
			if (graphics.DrawImage(&wallpaper_image, 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN)) != Gdiplus::Status::Ok)
			{
				std::cout << "Wallpaper could not be drawn" << std::endl;
			}
		}

		::EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_WINDOWPOSCHANGING:
	{
		auto window_pos = (WINDOWPOS*)l_param;
		if (hwnd != labeler_background_window)
		{
			window_pos->flags |= SWP_NOZORDER;
		}
		else
		{
			window_pos->hwndInsertAfter = labeler_window;
		}
		return 0;
	}
	}

	return ::DefWindowProc(hwnd, msg, w_param, l_param);
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
		HWND current_hwnd = reinterpret_cast<HWND>(l_param);

		if (current_hwnd == gotodesk_button)
		{
			std::cout << "Close window" << std::endl;
			::PostQuitMessage(0);
			break;
		}
		else if (current_hwnd == app_activate_button)
		{
			// send message to manager to restart...
			if (!::SetEvent(activate_app))
			{
				std::wcout << L"Failed to send restart app signal, error: " << ::GetLastError() << std::endl;
			}
			return ::DefWindowProc(hwnd, msg, w_param, l_param);
		}
		else if (current_hwnd == additional_app_activate_button)
		{
			// send message to manager to restart...
			if(!::SetEvent(activate_additional_app.value()))
			{
				std::wcout << L"Failed to send restart additional app signal, error: " << ::GetLastError() << std::endl;
			}
			return ::DefWindowProc(hwnd, msg, w_param, l_param);
		}
		else
		{
			break;
		}
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hwnd, &ps);
		::SetBkColor(hdc, transparent_color);

		DisplayTokenInCageWindow(hdc);

		::EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_CTLCOLORSTATIC:
	{
		HWND current_hwnd = reinterpret_cast<HWND>(l_param);

		HDC hdc_static = (HDC)w_param;
		if (current_hwnd == debug_warning)
		{
			::SetTextColor(hdc_static, RGB(179, 58, 58));
		}
		else
		{
			::SetTextColor(hdc_static, RGB(0, 0, 0));
		}
		::SetBkMode(hdc_static, TRANSPARENT);

		return (LRESULT)::GetStockObject(NULL_BRUSH);
	}
	case WM_ERASEBKGND:
	{
		HDC hdc_static = (HDC)w_param;
		RECT rect;
		::GetClientRect(hwnd, &rect);
		::FillRect(hdc_static, &rect, CreateSolidBrush(transparent_color));

		return 1;
	}
	case WM_CLOSE:
	{
		::PostQuitMessage(0);
		break;
	}
	default:
		return ::DefWindowProc(hwnd, msg, w_param, l_param);
	}

	return ::DefWindowProc(hwnd, msg, w_param, l_param);
}

bool CageLabeler::ShowLabelerWindow()
{
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = nullptr;
	wc.lpszClassName = window_class_name.c_str();
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;

	if (!::RegisterClass(&wc))
	{
		std::wcout << L"Registering of class for WindowToken failed" << std::endl;
		return false;
	}

	std::wstring bg_class = window_class_name; 
	bg_class.append(L"_bg");
	WNDCLASS wc_bg = {};
	wc_bg.hInstance = nullptr;
	wc_bg.lpfnWndProc = WndProc_background;
	wc_bg.lpszClassName = bg_class.c_str();
	wc_bg.hCursor = ::LoadCursor(nullptr, IDC_ARROW);

	if (!::RegisterClass(&wc_bg))
	{
		std::wcout << L"Registering of class for background failed" << std::endl;
		return false;
	}

	std::wstring overlay_class = window_class_name;
	overlay_class.append(L"_ov");
	WNDCLASS wc_overlay = {};
	wc_overlay.hInstance = nullptr;
	wc_overlay.lpfnWndProc = WndProc_background;
	wc_overlay.lpszClassName = overlay_class.c_str();
	wc_overlay.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc_overlay.hbrBackground = (HBRUSH)(CreateSolidBrush(RGB(0, 0, 0)));

	if (!::RegisterClass(&wc_overlay))
	{
		std::wcout << L"Registering of class for background overlay failed" << std::endl;
		return false;
	}

	std::wstring background_class = window_class_name;
	background_class.append(L"_lb_bg");
	WNDCLASS wc_lb_background = {};
	wc_lb_background.hInstance = nullptr;
	wc_lb_background.lpfnWndProc = WndProc_background;
	wc_lb_background.lpszClassName = background_class.c_str();
	wc_lb_background.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc_lb_background.hbrBackground = (HBRUSH)(CreateSolidBrush(RGB(255, 255, 255)));

	if (!::RegisterClass(&wc_lb_background))
	{
		std::wcout << L"Registering of class for labeler background failed" << std::endl;
		return false;
	}

	background_window = ::CreateWindowEx(
		WS_EX_LEFT | WS_EX_TOOLWINDOW,
		bg_class.c_str(),
		L"",
		(WS_POPUPWINDOW | WS_CLIPCHILDREN) & ~WS_BORDER,
		0,
		0,
		::GetSystemMetrics(SM_CXSCREEN),
		::GetSystemMetrics(SM_CYSCREEN),
		nullptr,
		nullptr,
		nullptr,
		nullptr);

	auto hwnd_overlay = ::CreateWindowEx(
		WS_EX_LEFT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
		overlay_class.c_str(),
		L"",
		(WS_POPUPWINDOW | WS_CLIPCHILDREN) & ~WS_BORDER,
		0,
		0,
		::GetSystemMetrics(SM_CXSCREEN),
		::GetSystemMetrics(SM_CYSCREEN),
		nullptr,
		nullptr,
		nullptr,
		nullptr);

	labeler_background_window = ::CreateWindowEx(
		WS_EX_LEFT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
		background_class.c_str(),
		L"",
		(WS_POPUPWINDOW | WS_CLIPCHILDREN) & ~WS_BORDER,
		0,
		0,
		labeler_width,
		::GetSystemMetrics(SM_CYSCREEN),
		nullptr,
		nullptr,
		nullptr,
		nullptr);

	if (background_window && hwnd_overlay && labeler_background_window)
	{
		double transparency_over = 50;
		::SetLayeredWindowAttributes(hwnd_overlay, 0, static_cast<BYTE>(((100 - transparency_over) / 100) * 255), LWA_ALPHA);
		::SetLayeredWindowAttributes(labeler_background_window, 0, static_cast<BYTE>(((100 - transparency_over) / 100) * 255), LWA_ALPHA);

		::SetWindowPos(background_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		::SetWindowPos(hwnd_overlay, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		::SetWindowPos(labeler_background_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		::ShowWindow(background_window, SW_SHOW);
		::ShowWindow(hwnd_overlay, SW_SHOW);
		::ShowWindow(labeler_background_window, SW_SHOW);
	}
	else
	{
		std::cout << "Creating background windows failed" << std::endl;
	}

	int bottom;
	if (!GetBottomFromMonitor(bottom))
	{
		std::wcout << L"Failed to get bottom of monitor" << std::endl;
		return false;
	}

	labeler_window = ::CreateWindowEx(
		WS_EX_LEFT | WS_EX_TOPMOST | WS_EX_LAYERED,
		window_class_name.c_str(),
		L"",
		(WS_POPUPWINDOW | WS_CLIPCHILDREN) & ~WS_BORDER,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		labeler_width,
		bottom,
		nullptr,
		nullptr,
		nullptr,
		nullptr);

	if (labeler_window == nullptr)
	{
		std::wcout << L"Creating window failed\n" << std::endl;
		return false;
	}

	::SetLayeredWindowAttributes(labeler_window, transparent_color, 255, LWA_COLORKEY);

	// Remove the window title bar
	if (!::SetWindowLong(labeler_window, GWL_STYLE, 0))
	{
		std::wcout << L"Failed to remove the titlebar, error: " << ::GetLastError() << std::endl;
	}

	::ShowWindow(labeler_window, SW_SHOW);

	MSG msg = {};
	while (::GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	std::cout << "Return from token display window creation" << std::endl;
	return true;
}

static bool DisplayTokenInCageWindow(HDC hdc)
{
	std::wcout << L"starting display image" << std::endl;

	const std::string token_string(app_token.begin(), app_token.end());

	std::string decoded_image = base64_decode(token_string);

	DWORD image_size = decoded_image.length();

	HGLOBAL h_memory = ::GlobalAlloc(GMEM_MOVEABLE, image_size);
	if (h_memory == nullptr)
	{
		std::cout << "Failed to allocate memory for token image. Error " << ::GetLastError() << std::endl;
		return false;
	}

	LPVOID p_image = ::GlobalLock(h_memory);
	if (p_image == nullptr)
	{
		std::cout << "Failed to allocate memory for token image. Error " << ::GetLastError() << std::endl;

		::GlobalFree(h_memory);

		return false;
	}

	std::memcpy(p_image, decoded_image.c_str(), image_size);

	IStream* p_stream = nullptr;
	if (::CreateStreamOnHGlobal(h_memory, FALSE, &p_stream) != S_OK)
	{
		std::cout << "Failed to create image stream from string for cage token. Error " << ::GetLastError() << std::endl;

		::GlobalUnlock(h_memory);
		::GlobalFree(h_memory);

		return false;
	}

	Gdiplus::Graphics graphics(hdc);
	Gdiplus::Image image(p_stream);

	double available_width = (double)labeler_width, available_height = (double)labeler_width;
	double image_height = (double)image.GetHeight() * (available_width / (double)image.GetWidth());
	double image_width = available_width;
	if (image_height > available_height)
	{
		image_width = image_width * (available_height / image_height);
		image_height = available_height;
	}

	if (graphics.DrawImage(&image, 0, 0, (int)image_width, (int)image_height) != Gdiplus::Status::Ok)
	{
		std::wcout << L"Failed to draw token in cage" << std::endl;

		p_stream->Release();
		::GlobalUnlock(h_memory);
		::GlobalFree(h_memory);

		return false;
	}

	p_stream->Release();
	::GlobalUnlock(h_memory);
	::GlobalFree(h_memory);

	return true;
}

// Write util to remove this duplicated function (same in FullWorkArea.h)
static bool GetBottomFromMonitor(int &monitor_bottom)
{
	HMONITOR monitor = ::MonitorFromWindow(nullptr, MONITOR_DEFAULTTONEAREST);
	if (monitor == nullptr)
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
		WS_VISIBLE | WS_CHILD,
		10,
		bottom - 44,
		labeler_width - 23,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);


	if (gotodesk_button != nullptr)
	{
		::SendMessage(gotodesk_button, BM_SETIMAGE, NULL, NULL);
	}
	else
	{
		std::wcout << L"Failed to create logout button, error: " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}

static bool ShowConfigMetadata(HWND &hwnd)
{
#ifdef _DEBUG
	debug_warning = ::CreateWindow(
		L"STATIC",
		L"!!! DEBUG VERSION - NOT SECURE !!!",
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		labeler_width,
		300,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);
#endif

	auto app_title = ::CreateWindow(
		L"STATIC",
		L"Programs:",
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		labeler_width + 50,
		150,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);

	auto app_name_title = ::CreateWindow(
		L"STATIC",
		app_name.c_str(),
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		labeler_width + 79,
		150,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);

	app_activate_button = ::CreateWindow(
		L"BUTTON",
		L"Activate",
		WS_VISIBLE | WS_CHILD,
		labeler_width - 112,
		labeler_width + 79,
		100,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);

	HWND additional_app_app_title = nullptr;
	if (additional_app_name.has_value())
	{
		additional_app_app_title = ::CreateWindow(
			L"STATIC",
			additional_app_name.value().c_str(),
			SS_LEFT | WS_VISIBLE | WS_CHILD,
			10,
			labeler_width + 128,
			150,
			34,
			hwnd,
			nullptr,
			nullptr,
			nullptr);

		additional_app_activate_button = ::CreateWindow(
			L"BUTTON",
			L"Activate",
			WS_VISIBLE | WS_CHILD,
			labeler_width - 112,
			labeler_width + 128,
			100,
			34,
			hwnd,
			nullptr,
			nullptr,
			nullptr);
	}

	auto app_hash_text_title = ::CreateWindow(
		L"STATIC",
		L"Binary hashes (SHA-512, truncated):",
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		labeler_width + 200,
		labeler_width - 20,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);

	std::wstring app_hash_string = L"App - " + app_hash;
	auto app_hash_text = ::CreateWindow(
		L"STATIC",
		app_hash_string.c_str(),
		SS_LEFT | WS_VISIBLE | WS_CHILD,
		10,
		labeler_width + 225,
		labeler_width - 30,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);

	HWND closing_restricted_text = nullptr;
	if (restrict_closing)
	{
		int bottom;
		if (GetBottomFromMonitor(bottom))
		{
			closing_restricted_text = ::CreateWindow(
				L"STATIC",
				L"Closing is restricted to exit button!",
				SS_CENTER | WS_VISIBLE | WS_CHILD,
				10,
				bottom - 80,
				labeler_width - 23,
				34,
				hwnd,
				nullptr,
				nullptr,
				nullptr);
		}
		else
		{
			std::wcout << L"Failed to get bottom of monitor, can not display exit restriction text" << std::endl;
		}
	}

	if (app_name_title != nullptr && app_activate_button != nullptr)
	{
		HFONT default_font = ::CreateFont(
			16,
			0,
			0,
			0,
			FW_MEDIUM,
			FALSE,
			FALSE,
			FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_NATURAL_QUALITY,
			DEFAULT_PITCH | FF_SWISS,
			L"Segoi");

		HFONT bold_font = ::CreateFont(
			16,
			0,
			0,
			0,
			FW_SEMIBOLD,
			FALSE,
			FALSE,
			FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_NATURAL_QUALITY,
			DEFAULT_PITCH | FF_SWISS,
			L"Segoi");

#ifdef _DEBUG
		::SendMessage(debug_warning, WM_SETFONT, (WPARAM)bold_font, TRUE);
#endif
		::SendMessage(app_title, WM_SETFONT, (WPARAM)bold_font, TRUE);
		::SendMessage(app_name_title, WM_SETFONT, (WPARAM)default_font, TRUE);
		::SendMessage(app_activate_button, BM_SETIMAGE, NULL, NULL);
		::SendMessage(app_hash_text_title, WM_SETFONT, (WPARAM)bold_font, TRUE);
		::SendMessage(app_hash_text, WM_SETFONT, (WPARAM)default_font, TRUE);

		if (additional_app_name.has_value())
		{
			::SendMessage(additional_app_app_title, WM_SETFONT, (WPARAM)default_font, TRUE);
			::SendMessage(additional_app_activate_button, BM_SETIMAGE, NULL, NULL);
		}

		if (restrict_closing)
		{
			::SendMessage(closing_restricted_text, WM_SETFONT, (WPARAM)bold_font, TRUE);
		}
	}
	else
	{
		std::wcout << L"Failed to create logout button. Error: " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}