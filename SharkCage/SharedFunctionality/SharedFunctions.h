/*! \file SharedFunctions.h
 * \brief A few shared functions for message parsing, which are needed by multiple
 * parts of the project.
 */

#pragma once

#include "CageData.h"
#include "Messages.h"
#include <optional>

static bool BeginsWith(const std::wstring &string_to_search, const std::wstring &prefix);
static void TrimMessage(std::wstring &msg);

/*!
 * \brief The namespace containing shared functions for message parsing.
 */
namespace SharedFunctions
{
	/*!
	 * \brief Parses the config file and sets the values of the other values of the
	 * supplied CageData.
	 * @param &cage_data the cage_data containing the config to be parsed and the
	 * variables to be set.
	 * @return a boolean, true if successful, false otherwise
	 */
	DLLEXPORT bool ParseStartProcessMessage(CageData &cage_data);

	/*!
	 * \brief Parses the message supplied and determines its type.
	 * @param &msg the message received
	 * @param &message_data output paramater to receive the message, trimmed
	 * @return an optional containing the CageMessage type, or a nullopt
	 */
	DLLEXPORT std::optional<CageMessage> ParseMessage(const std::wstring &msg, std::wstring &message_data);
	
	/*!
	 * \brief A toString for CageMessage.
	 * @param msg the message
	 * @return a string representation of the CageMessage
	 */
	DLLEXPORT std::wstring MessageToString(CageMessage msg);
}