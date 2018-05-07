# CageService
The Cage Service is a program that is registered to Windows as a service. It has to run in the background and listens for essages from the UI. According to the messages it starts/stops the `CageManager` or forwards messages to the `Manager`.

## Files

### Headers

#### CageService.h
Header file for the class that encapsulates starting and stopping the `CageManager`.

#### MsgService.h
This header contains the messages to be received by the `CageService`. The enum for type safety and the strings to send through network connection.

#### MsgManager.h
Contains messages to be received by the `CageManager`.

#### NetworkManager.h
Simplifies the network communication by providing a send() and listen() function. Has to be instanciated differently for each component (with provided constats).

#### StatusManager.h
Contains the statuses of the `CageService`.

### Source

#### CageService.cpp
Contains functions to start and stop the `CageManager`.

#### CageServiceMain.cpp
This file contains the entry point for the Windows service and the fields and functions that allow Windows to start and stop the service.

`_tmain()`, `ServiceMain()` and `ServiceCtrlHandler()` are template functions that have to be the same for mostly every service.

`ServiceWorkerThread()` holds the loop of the service to listen to messages coming from the UI.

`OnReceive()` parses a message, that was received in the main loop. It tests if the message begins with a certain string and does an action acording to that.

`BeginsWith()` tests if a string begins with a prefix. Returns true if the string begins with the prefix or if they are the same.
