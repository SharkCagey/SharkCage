/*! \file CageLabeler.h
 * \brief Handles the display and usage of the started CageService in the new desktop.
 * 
 */
#pragma once

#include "../SharedFunctionality/CageData.h"

/*!
 * \brief Handles display in the new desktop.
 * Holds some configuration about itself.
 */
class CageLabeler
{
public:

	/*!
	 * \brief Constructor sets options according to CageData.
	 * @param &cage_data the CageData containing the config parameters
	 * @param &_cage_width the labeler width for display
	 * @param &_window_class_name the window name
	 */
	CageLabeler(
		const CageData &cage_data,
		const int &_cage_width,
		const std::wstring &_window_class_name);
	
	/*!
	 * \brief Shows the CageLabeler window.
	 * @return true if successful, false otherwise
	 */
	bool Init();
private:
	std::wstring window_class_name;

	bool ShowLabelerWindow();
	void InitGdipPlisLib();
};

