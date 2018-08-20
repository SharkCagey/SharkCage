#pragma once

#include <string>

/*!
 * \brief The different messagetypes our system has.
 * Can be expanded upon if the need arises.
 */
enum class CageMessage
{
	START_PROCESS = 0,
	RESPONSE_SUCCESS,
	RESPONSE_FAILURE
};