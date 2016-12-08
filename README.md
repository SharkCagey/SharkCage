# Shark Cage Team project
This project contains windows programs to isolate a program in an own desktop to prevent malware (without OS privileges) to capture user input or sreenshots.

## Install the Cage Service
1. Build the project
2. Create the folder C:\sharkcage and copy "Cage service.exe" and CageManager.exe to this folder.
3. Open a CMD a administrator and add "Cage service.exe" to the services: sc create "Cage Service" binPath= "C:\sharkcage\Cage service.exe"

## Useful Links
Our [Jenkins Server](http://35.162.112.109:8080/)
