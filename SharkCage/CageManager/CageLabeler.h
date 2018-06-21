#pragma once

#include "CageData.h"

class CageLabeler
{
public:
	CageLabeler(
		const CageData &cage_data,
		const int &_cage_width,
		const std::wstring &_window_class_name);
	bool Init();
private:
	std::wstring window_class_name;

	bool ShowCageWindow();
	void InitGdipPlisLib();
};

