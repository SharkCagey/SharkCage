# Shark Cage

[![Build status](https://ci.appveyor.com/api/projects/status/jxhrl395fvtjhwc6?svg=true)](https://ci.appveyor.com/project/SharkCagey/htwg-shark-cage)

This software is provided "as is", without warranty of any kind. This software is also still under development. Use entirely at your own risk. Contributions through PRs are highly appreciated.

This project contains Windows programs to isolate a specific program in its own desktop to prevent malware (without OS privileges) to capture user input or screenshots.
After the installation of Shark Cage, create a config using the `CageConfigurator` containing the program (e.g. Firefox for online banking) which should run in a secure environment, optionally an additional programm (e.g. Keepass to retrieve the password for the online banking) and an icon which is later used to signal the execution in a secure environment to the user.

All programs running in the Shark Cage will be started on a second, isolated desktop which malware without administrator privilieges can not access.

## Components

This project consists of five sub-programs:

* `CageService`
* `CageManager`
* `CageChooser`
* `CageConfigurator`
* `SharkCageInstaller`

Three of them (Service, Manager, Chooser) interact with each other via messages using a TCP connection.

### CageService

The `CageService` implements a Windows service running in the background. It receives messages from the `CageChooser` and sends messages to the `CageManager`.

### CageManager

The `CageManager` creates a new desktop and starts the program and optionally the additional program according to the config received from the `CageChooser` over the `CageService`. In addition, the token image and some additional information is displayed. Using the displayed "Activate"-button(s) the program(s) can be restarted or brought back into the foreground.

<details><summary markdown="span"><code>Expand to see example</code></summary>

<img width="1680" alt="Screenshot CageManager" src="https://user-images.githubusercontent.com/1786772/43678602-914198ec-9816-11e8-8a75-0ac3032368e4.png">

</details>

### CageChooser

The `CageChooser` is a user interface which displays in a list all available configs on the system by iterating over the registry entries at the following path: `Computer\HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\SharkCage\Configs`. By selecting a config and pressing the "Start"-button (or the enter key), the `CageManager` will be started. The creation of the CageManager is done implicitely when receiving a `START_PROCESS` message which means the Chooser will only send one message with the config path and then everything else happens automatically.

<details><summary markdown="span"><code>Expand to see example</code></summary>

<img width="500" alt="Screenshot CageChooser" src="https://user-images.githubusercontent.com/1786772/43733677-c50f6b72-99b5-11e8-9831-69556fd33246.png">

</details>

### CageConfigurator

The `CageConfigurator` provides a graphical user interface to create a config file including a token image to be displayed on the secure desktop, the program which should be started and optionally an additional application. The additional program can be chosen out of a list of "trustworthy" applications.
 As soon as the config file has been saved, a link to the config which is stored at `C:\Users\Public\Documents\SharkCage\` will be saved in the Registry under the following path: `Computer\HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\SharkCage\Configs`.
The config contains json data and its access rights restrict anyone except the administrator group from accessing the file in any way.

<details><summary markdown="span"><code>Expand to see example</code></summary>

<img width="1680" alt="Screenshot CageConfigurator" src="https://user-images.githubusercontent.com/1786772/43733731-e88c2e50-99b5-11e8-84a5-860351997f55.png">

</details>

### SharkCageInstaller

The `SharkCageInstaller` is used to install all project applications, starting the `CageService` and setting some keys in the registry.

## Installation

1. The `SharkCageInstaller` is used to install all programs for this project (`CageService`, `CageManager`, `CageChooser` and `CageConfigurator`) and is hosted on Github. Follow this link and download the latest version: [Github-Releases](https://github.com/SharkCagey/HTWG_shark_cage/releases).
1. Please make sure the `SharkCageInstaller` is signed using the certificate issued to the HTWG Konstanz with the following fingerprint: `ADBE74BD39789DD111815DE59C60D715143E4620` to avoid any unnecessary security risks.
1. Execute the installer and follow the instructions. For installing the service, the `SharkCageInstaller` needs to run with admininistration privileges. Please make sure that the "User Account Control"-dialog shows the HTWG Konstanz as the verified publisher.
   <details><summary markdown="span"><code>Expand to see example</code></summary>

   <img width="500" alt="Screenshot User Account Control dialog" src="https://user-images.githubusercontent.com/1786772/43678105-71343f90-980d-11e8-89f5-9a77c63b86fa.png">

   </details>

## Building from source

1. Clone or download this repository
1. Build the project
1. If using **debug build** you can just start the `CageChooser` and a powershell script with on-screen instructions will correctly configure your system.   
If you want to use the **release build** the easiest solution is to run the included (built) installer and follow the instructions.
