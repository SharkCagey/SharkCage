#ifndef _BASE64_H_
#define _BASE64_H_
#include <string>
std::string base64_encode(const std::string &bytes_to_encode, unsigned int in_len);
std::string base64_decode(std::string const& s);
#endif #pragma once
