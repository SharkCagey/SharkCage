#include <Windows.h>
#include <iostream>

class FullWorkArea
{
public:
	FullWorkArea();
	~FullWorkArea();
private:
	RECT rect;
	int getBottomFromMonitor();
};


FullWorkArea::FullWorkArea()
{
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0) == false)
	{
		std::cout << "Failed to access work area. Error " << GetLastError() << std::endl;
	}

	int bottom;
	if ((bottom = getBottomFromMonitor()) > 0) 
	{
		RECT cage_rect;
		cage_rect.left = 500;
		cage_rect.bottom = bottom;
		cage_rect.right = rect.right;
		cage_rect.top = rect.top;

		std::cout << "Set working area to" << cage_rect.left << "x" << cage_rect.bottom << "x" << cage_rect.right << "x" << cage_rect.top << std::endl;

		if (SystemParametersInfo(SPI_SETWORKAREA, 0, &cage_rect, SPIF_UPDATEINIFILE) == false)
		{
			std::cout << "Failed to  work area. Error " << GetLastError() << std::endl;
		}
	}
	else
	{
		std::cout << "Failed to set working area" << std::endl;
	}
}

FullWorkArea::~FullWorkArea()
{
	if (SystemParametersInfo(SPI_SETWORKAREA, 0, &rect, SPIF_UPDATEINIFILE) == false)
	{
		std::cout << "Failed to update work area. Error " << GetLastError() << std::endl;
	}
}

int FullWorkArea::getBottomFromMonitor()
{
	HMONITOR monitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTONEAREST);
	if (monitor == NULL)
	{
		std::cout << "Failed to get MonitorFromWindow. Error " << GetLastError() << std::endl;
		return -1;
	}

	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);

	if (GetMonitorInfo(monitor, &monitorInfo) == false)
	{
		std::cout << "Failed to GetMonitorInfo. Error " << GetLastError() << std::endl;
		return -1;
	}

	return monitorInfo.rcMonitor.bottom;
}