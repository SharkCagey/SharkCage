#pragma once
#pragma once

#include <Windows.h>
#include <string>
#include <optional>
#include <bcrypt.h>

class ValidateBinary
{
public:
	static bool ValidateCertificate(const std::wstring &app_path);
	static bool ValidateHash(const std::wstring &app_path, const std::wstring &app_hash);
private:
	static void BytesToHexString(unsigned char const *bytes, std::size_t bytesLength, char *dest);
	static bool CompareHashes(const std::wstring &hash_1, char const *hash_2);
	static void CleanupHash(
		const BCRYPT_ALG_HANDLE &algorithm,
		const BCRYPT_HASH_HANDLE &hash_handle,
		const PBYTE &hash_object,
		const PUCHAR &hash);
};
