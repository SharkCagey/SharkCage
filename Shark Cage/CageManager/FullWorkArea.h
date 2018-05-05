#include <Windows.h>
#include <iostream>

class FullWorkArea
{
public:
	FullWorkArea();
	~FullWorkArea();
private:
	RECT rect;
};


FullWorkArea::FullWorkArea()
{
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0) == 0)
	{
		std::cout << "Failed to access work area. Error " << GetLastError() << std::endl;
	}

	RECT cage_rect;
	cage_rect.left = 500;
	cage_rect.bottom = rect.bottom + 20; // use MonitorFromWindow and GetMonitorInfo to set to "full screen"
	cage_rect.right = rect.right;
	cage_rect.top = rect.top;

	if (SystemParametersInfo(SPI_SETWORKAREA, 0, &cage_rect, SPIF_UPDATEINIFILE) == 0)
	{
		std::cout << "Failed to update work area. Error " << GetLastError() << std::endl;
	}
}

FullWorkArea::~FullWorkArea()
{
	if (SystemParametersInfo(SPI_SETWORKAREA, 0, &rect, SPIF_UPDATEINIFILE) == 0)
	{
		std::cout << "Failed to update work area. Error " << GetLastError() << std::endl;
	}
}