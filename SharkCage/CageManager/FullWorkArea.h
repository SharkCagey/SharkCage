/*! \file FullWorkArea.h
 * \brief Sets and resets the FullWorkArea when the Shark Cage is created or ended.
 */

#pragma once

#include "Windows.h"

/*!
 * \brief Calculates the total available work area for the CageLabeler, taking into
 * account the given cage_width and monitor size.
 */
class FullWorkArea
{
public:
	/*!
	 * \brief Constructor saves the cage width.
	 * @param &cage_width the specified cage width
	 */
	FullWorkArea(const int &cage_width);
	
	/*!
	 * \brief Deconstructor tries to reset the work area.
	 */
	~FullWorkArea();
	
	/*!
	 * \brief Sets the work area according to cage width and monitor size.
	 * @return true if successful, false otherwise
	 */
	bool Init();
private:
	RECT rect;
	bool GetBottomFromMonitor(int &monitor_bottom);
	int cage_width;
};