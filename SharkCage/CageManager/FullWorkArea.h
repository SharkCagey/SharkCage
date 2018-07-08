#pragma once

#include "Windows.h"

class FullWorkArea
{
public:
	FullWorkArea(const int &cage_width);
	~FullWorkArea();
	bool Init();
private:
	RECT rect;
	bool GetBottomFromMonitor(int &monitor_bottom);
	int cage_width;
};