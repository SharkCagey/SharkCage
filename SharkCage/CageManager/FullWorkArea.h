#include <Windows.h>
#include <iostream>

class FullWorkArea
{
public:
	~FullWorkArea();
	bool Init();
private:
	RECT rect;
	bool GetBottomFromMonitor(int *monitor_bottom);
};


bool FullWorkArea::Init()
{
	if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0))
	{
		std::cout << "Failed to access work area. Error " << ::GetLastError() << std::endl;
		return false;
	}

	int bottom;
	if (GetBottomFromMonitor(&bottom))
	{
		RECT cage_rect;
		cage_rect.left = 500;
		cage_rect.bottom = bottom;
		cage_rect.right = rect.right;
		cage_rect.top = rect.top;

		std::cout << "Set working area to" << cage_rect.left << "x" << cage_rect.bottom << "x" << cage_rect.right << "x" << cage_rect.top << std::endl;

		if (!::SystemParametersInfo(SPI_SETWORKAREA, 0, &cage_rect, SPIF_UPDATEINIFILE))
		{
			std::cout << "Failed to  work area. Error " << ::GetLastError() << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "Failed to set working area" << std::endl;
		return false;
	}

	return true;
}

FullWorkArea::~FullWorkArea()
{
	if (!::SystemParametersInfo(SPI_SETWORKAREA, 0, &rect, SPIF_UPDATEINIFILE))
	{
		std::cout << "Failed to update work area. Error " << ::GetLastError() << std::endl;
	}
}

bool FullWorkArea::GetBottomFromMonitor(int *monitor_bottom)
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

	*monitor_bottom = monitor_info.rcMonitor.bottom;

	return true;
}