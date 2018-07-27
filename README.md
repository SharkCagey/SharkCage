# Shark Cage Team project

[![Build status](https://ci.appveyor.com/api/projects/status/jxhrl395fvtjhwc6?svg=true)](https://ci.appveyor.com/project/SharkCagey/htwg-shark-cage)

This software is provided "as is", without warranty of any kind. This software is also still under development. Use entirely at your own risk.

This project contains windows programs to isolate a program in an own desktop to prevent malware (without OS privileges) to capture user input or sreenshots.
It consists of 4 sub-programs: The `CageService`, `CageManager`, `StarterCmd`, and `ImageSelectDialog`. The 3 actual parts (Service, Manager and StarterCmd) communicate over a TCP connection to interchange messages.

## Components

### CageService

The `CageService` implements a Windows service running the background. It receives messages from the UI and sends messages to the `CageManager`.

### CageManager

The `CageManager` creates a new desktop and processes in it, according to the messages received by the `CageService`.

### StarterCmd

A user interface for starting the `CageManager` and an application in it. Directly communicating with the `CageService`.

### ImageSelectDialog

A user interface to select an icon to be displayed on the user's desktop. `config.txt` will be created in `C:\sharkcage` with a line containing `picture: ` followed by the index of the icon. The `CageService` reads this configuration file and searches for that line and opens the dialog if the file contains no such line.

## Installation

1. Build the project
1. Create the folder `C:\sharkcage` and copy `CageService.exe`, `CageManager.exe` and `ImageSelectDialog.exe` to this folder.
1. Open `cmd` or `powershell` as administrator and add `CageService.exe` to the services: `sc create "shark-cage-service" binPath="C:\sharkcage\CageService.exe" obj= "NT AUTHORITY\SYSTEM"`
1. Configure and start the `CageService`: `sc config "shark-cage-service" start=auto | sc start "shark-cage-service"`

## Useful Links

Our [Jenkins Server](http://35.162.112.109:8080/)