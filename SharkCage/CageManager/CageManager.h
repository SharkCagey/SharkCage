#pragma once

#pragma comment(lib, "netapi32.lib")

template<typename T>
auto local_free_deleter = [&](T resource) { ::LocalFree(resource); };

class CageManager
{
public:
	void StartCageLabeler(
		const CageData &cage_data,
		const int work_area_width,
		const std::wstring &labeler_window_class_name);
};