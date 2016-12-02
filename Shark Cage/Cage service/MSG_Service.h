#pragma once

#include <vector>
#include <string>

typedef enum E_MSG_TO_SERVICE {
    START_TP = 0,
    STOP_TP,
    NUMBEROFMESSAGES // Value that contains the number of messages - NOT A MESSAGE ITSELF !!!
} MSG_TO_SERVICE;

// Names for the message constants to be convertible to string
static const std::string MSG_TO_SERVICE_STRINGS[] = {
    "START",
    "STOP"
};

// To test if there are names for all messages (at compile time)
static_assert((sizeof(MSG_TO_SERVICE_STRINGS)/sizeof(std::string)) == NUMBEROFMESSAGES, "Error within MessageTypes");

std::string MSG_TO_SERVICE_toString(MSG_TO_SERVICE value) {
    return MSG_TO_SERVICE_STRINGS[value];
}
