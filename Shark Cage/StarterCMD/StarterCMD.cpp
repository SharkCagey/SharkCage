// StarterCMD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Cage service/NetworkManager.h"
#include "../Cage service/MSG_Service.h"


int main() {
    NetworkManager mgr(UI);

    mgr.send(MSG_TO_SERVICE_toString(START_CM));

    return 0;
}
