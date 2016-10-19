# Tasks form profesor:

## Continous integration
- the point of continous integration is to have an automated compile and testing environment.
- Works like this: <br>
1) You make the code <br>
2) Push it to git <br>
3) CI machine will automatically compile the code, build the project, runs all the tests and sends you back the results

## Protocol development

- We will have 3 parts of application. <br>
1) Service (running in session 0) <br>
2) Process (runing in user session) <br>
3) User interface (running on the safe desktop)

- the point here is to create a way how the parts should talk to each other

## Service part (running in session 0)
- part running in operating system (necessary for creating a new process group and a token /the undocumented function menoioned by professor/ )
- this will create the process group, appropriate token and start user session process

## User session process
- this part creates the safe desktop, sets ACL in a way that it should be and grats access to the "caged" application

## User interface
- since safe desktop has nothing there (no window manager, explorer, nothing...) , this will provide basic functionality (showing the name of the app, security picture, maybe way to quit app)

## Access control list
- in this part, task is to find out how to set all the access privileges to all cage-related processes and files so the processes will have all the privileges they need, but malware will not be able access anything
