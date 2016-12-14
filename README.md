# Shark Cage Team project
This project contains windows programs to isolate a program in an own desktop to prevent malware (without OS privileges) to capture user input or sreenshots.
It consists of 3 sub-programs: The Cage Service, Cage Manager and user interface. The 3 parts communicate over a TCP connection to interchange messages.

## Cage Service
The Cage Service implements a Windows service running the background. It receives messages from the UI and sends messages to the Cage Manager.

### Install the Cage Service
1. Build the project
2. Create the folder C:\sharkcage and copy "Cage service.exe" and CageManager.exe to this folder.
3. Open a CMD a administrator and add "Cage service.exe" to the services: ```sc create "Cage Service" binPath= "C:\sharkcage\Cage service.exe"```

## Cage Manager
The Cage Manager creates a new desktop and processes in it, according to the messages received by the Cage Service.

## StarterCMD
A user interface for starting the Cage Manager and an application in it. Directly communicating with the Cage Service.

## Useful Links
- Our [Jenkins Server](http://35.162.112.109:8080/)
- Unit Testing Help (https://msdn.microsoft.com/en-us/library/hh419385.aspx#sameProject)
