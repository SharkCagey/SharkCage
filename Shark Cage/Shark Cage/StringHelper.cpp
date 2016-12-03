#include "stdafx.h"
#include "StringHelper.h"


StringHelper::StringHelper() {
}


StringHelper::~StringHelper() {
}

bool StringHelper::beginsWith(const std::string string, const std::string prefix) {
    if (string.length() < prefix.length()) {
        return false;
    } else {
        if (string.compare(0, prefix.length(), prefix) == 0) {
            return true;
        } else {
            return false;
        }
    }
}
