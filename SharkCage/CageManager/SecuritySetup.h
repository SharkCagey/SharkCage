#pragma once

#include <memory>
#include <optional>
#include <string>

template<typename T>
auto local_free_deleter = [&](T *resource) { ::LocalFree(resource); };

typedef void Sid;
typedef std::unique_ptr<Sid, decltype(local_free_deleter<Sid>)> SidPointer;

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
	SidPointer CreateSID(const std::wstring &group_name);
	std::optional<PACL> CreateACL(SidPointer group_sid);

	PSECURITY_DESCRIPTOR security_descriptor;
};
