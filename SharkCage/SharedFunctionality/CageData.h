/*! \file CageData.h
 * \brief Declares struct CageData for holding the loaded configuration.
 */
#pragma once

#include <string>
#include <optional>

/*!
 * \brief Contains the config path and application(s) to be used inside of the cage to be
 * used by CageManager.
 */
struct CageData
{
	/*@{*/
	const std::wstring config_path; /*!< the path to the config file */
	std::wstring app_path; /*!< the path to the chosen application */
	std::wstring app_name; /*!< the name of the chosen application */
	std::wstring app_token; /*!< the secure access token */
	std::wstring app_hash; /*!< the hash to go with the token */
	std::optional<std::wstring> additional_app_name; /*!< the name of the additional application, if chosen */
	std::optional<std::wstring> additional_app_path; /*!< the path to the additional application, if chosen */
	bool restrict_closing; /*!< TODO: whether the cage is closable */
	/*@}*/

	/*@{*/
	/*!
	 * \brief Whether an additional application has been chosen.
	 * @return a boolean, true if an additional application is to be started, false otherwise
	 */
	bool hasAdditionalAppInfo() const
	{
		return additional_app_name.has_value() && additional_app_path.has_value();
	}
	/*@}*/
};