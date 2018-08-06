#pragma once

#include <memory>
#include <optional>
#include <string>

template<typename T>
auto local_free_deleter = [&](T resource) { ::LocalFree(resource); };

class SecuritySetup
{
public:
	SecuritySetup() {};

	// these need to be customized if needed so security_descriptor also gets copied
	// (otherwise there could be use-after-frees in the destructor)
	SecuritySetup(const SecuritySetup &) = delete;
	SecuritySetup& operator= (const SecuritySetup &) = delete;

	~SecuritySetup()
	{
		if (security_descriptor)
		{
			::LocalFree(security_descriptor);
			security_descriptor = nullptr;
		}
	}

	std::optional<SECURITY_ATTRIBUTES> GetSecurityAttributes(const std::wstring &group_name);

private:
	std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> CreateSID(const std::wstring &group_name);
	std::optional<PACL> CreateACL(std::unique_ptr<PSID, decltype(local_free_deleter<PSID>)> group_sid);

	PSECURITY_DESCRIPTOR security_descriptor;
};
