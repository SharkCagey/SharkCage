#pragma once

#include "Windows.h"

class FullWorkArea
{
public:
	FullWorkArea(const int &cage_width);
	~FullWorkArea();
	bool Init();
private:
	RECT rect = { 0 };
	bool GetBottomFromMonitor(int &monitor_bottom);
	int cage_width;
};