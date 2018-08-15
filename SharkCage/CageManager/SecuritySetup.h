/*! \file SecuritySetup.h
 * \brief Performs the Security Setup, including ACLs.
 */

#pragma once

#include <memory>
#include <optional>

template<typename T>
auto local_free_deleter = [&](T resource) { ::LocalFree(resource); };

/*!
 * \brief Perfoms the Security Setup of the Shark Cage.
 */
class SecuritySetup
{
public:
	/*!
	 * \brief Destructor calls the free method of the security descriptor if it exists.
	 */
	~SecuritySetup()
	{
		if (security_descriptor)
		{
			::LocalFree(security_descriptor);
		}
	}

	/*!
	 * \brief Gets the Security Attributes.
	 * @return an optional either containing the security attributes, or std::nullopt
	 */
	std::optional<SECURITY_ATTRIBUTES> GetSecurityAttributes();

private:
	std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> CreateSID();
	std::optional<PACL> CreateACL(std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> group_sid);

	PSECURITY_DESCRIPTOR security_descriptor;
};
