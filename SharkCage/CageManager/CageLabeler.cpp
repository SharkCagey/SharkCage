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

static bool DisplayTokenInCageWindow(HWND *hwnd);
static bool GetBottomFromMonitor(int &monitor_bottom);
static bool ShowExitButton(HWND &hwnd);
static bool ShowConfigMetadata(HWND &hwnd);

static HWND gotodesk_button;
static HWND app_title;
static HWND app_name_title;
static HWND app_name_restart_button;
static HWND additional_app_app_title;
static HWND additional_app_restart_button;
static HWND app_hash_text_title;
static HWND app_hash_text;
static HWND closing_restricted_text;

static std::wstring app_name;
static std::wstring app_token;
static std::wstring app_hash;
static std::optional<std::wstring> additional_app_name;
static bool restrict_closing;
static int labeler_width;

static HBRUSH h_brush = ::CreateSolidBrush(RGB(255, 255, 255));

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
		else if (current_hwnd == app_name_restart_button)
		{
			// send message to manager to restart...
			return ::DefWindowProc(hwnd, msg, w_param, l_param);
		}
		else if (current_hwnd == additional_app_restart_button)
		{
			// send message to manager to restart...
			return ::DefWindowProc(hwnd, msg, w_param, l_param);
		}
		else
		{
			break;
		}
	}
	case WM_PAINT:
	{
		DisplayTokenInCageWindow(&hwnd);
		return ::DefWindowProc(hwnd, msg, w_param, l_param);
	}
	case WM_CTLCOLORSTATIC:
	{
		HWND current_hwnd = reinterpret_cast<HWND>(l_param);
		if (app_title == current_hwnd
			|| additional_app_app_title == current_hwnd
			|| app_name_title == current_hwnd
			|| app_hash_text_title == current_hwnd
			|| app_hash_text == current_hwnd
			|| closing_restricted_text == current_hwnd)
		{
			HDC hdc_static = (HDC)w_param;
			::SetTextColor(hdc_static, RGB(0, 0, 0));
			::SetBkMode(hdc_static, TRANSPARENT);
			return (INT_PTR)h_brush;
		}
	}
	case WM_CLOSE:
	{
		::PostQuitMessage(0);
		break;
	}
	default:
		return ::DefWindowProc(hwnd, msg, w_param, l_param);
	}

	return EXIT_SUCCESS;
}

bool CageLabeler::ShowLabelerWindow()
{
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = nullptr;
	wc.lpszClassName = window_class_name.c_str();
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);

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
		window_class_name.c_str(),
		L"",
		WS_POPUPWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		labeler_width,
		bottom,
		nullptr,
		nullptr,
		nullptr,
		nullptr);

	if (hwnd == nullptr)
	{
		std::wcout << L"Creating window failed\n" << std::endl;
		return false;
	}

	// Remove the window title bar
	if (!::SetWindowLong(hwnd, GWL_STYLE, 0))
	{
		std::wcout << L"Failed to remove the titlebar, error: " << ::GetLastError() << std::endl;
	}

	::ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};
	while (::GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	std::cout << "Return from token display window creation" << std::endl;
	return true;
}

static bool DisplayTokenInCageWindow(HWND *hwnd)
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

	HDC hdc = ::GetDC(*hwnd);

	Graphics graphics(hdc);
	Image image(p_stream);

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
		WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
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
	app_title = ::CreateWindow(
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

	app_name_title = ::CreateWindow(
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

	app_name_restart_button = ::CreateWindow(
		L"BUTTON",
		L"Restart",
		WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		labeler_width - 112,
		labeler_width + 79,
		100,
		34,
		hwnd,
		nullptr,
		nullptr,
		nullptr);

	if (additional_app_name.has_value())
	{
		additional_app_app_title = ::CreateWindow(
			TEXT("STATIC"),
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

		additional_app_restart_button = ::CreateWindow(
			L"BUTTON",
			L"Restart",
			WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			labeler_width - 112,
			labeler_width + 128,
			100,
			34,
			hwnd,
			nullptr,
			nullptr,
			nullptr);
	}

	app_hash_text_title = ::CreateWindow(
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
	app_hash_text = ::CreateWindow(
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

	if (app_name_title != nullptr && app_name_restart_button != nullptr)
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
			nullptr);

		HFONT bold_font = ::CreateFont(
			17,
			0,
			0,
			0,
			FW_BOLD,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DRAFT_QUALITY,
			DEFAULT_PITCH | FF_SWISS,
			nullptr);

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
			nullptr);

		::SendMessage(app_title, WM_SETFONT, (WPARAM)bold_font, TRUE);
		::SendMessage(app_name_title, WM_SETFONT, (WPARAM)default_font, TRUE);
		::SendMessage(app_name_restart_button, BM_SETIMAGE, NULL, NULL);
		::SendMessage(app_hash_text_title, WM_SETFONT, (WPARAM)bold_font, TRUE);
		::SendMessage(app_hash_text, WM_SETFONT, (WPARAM)italic_font, TRUE);

		if (additional_app_name.has_value())
		{
			::SendMessage(additional_app_app_title, WM_SETFONT, (WPARAM)default_font, TRUE);
			::SendMessage(additional_app_restart_button, BM_SETIMAGE, NULL, NULL);
		}

		if (restrict_closing)
		{
			::SendMessage(closing_restricted_text, WM_SETFONT, (WPARAM)bold_font, TRUE);
		}
	}
	else
	{
		std::wcout << L"Failed to create logout button Err " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}