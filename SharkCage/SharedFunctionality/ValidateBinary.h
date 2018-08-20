#pragma once

#include <string>
#include <vector>

class ValidateBinary
{
public:
	static bool ValidateCertificate(const std::wstring &app_path);
	static bool ValidateHash(const std::wstring &app_path, const std::wstring &app_hash);
private:
	static std::vector<char> BytesToHexString(unsigned char const *bytes, std::size_t length);
	static bool CompareHashes(const std::wstring &hash_1, const std::string &hash_2);
};
