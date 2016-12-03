#pragma once

#include <string>

class StringHelper {
public:
    StringHelper();
    ~StringHelper();


    static bool beginsWith(const std::string string, const std::string prefix);
};

