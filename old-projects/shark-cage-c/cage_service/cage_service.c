///
/// \file unsharked.c
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage service - main module
///
#include <Windows.h>
#include <WtsApi32.h>
#include <stdio.h>
#include <AclAPI.h>
#include <Sddl.h>
#include <LM.h>
#include <strsafe.h>
#include <Psapi.h>


#include "cage_lib.h"


#define SERVICE_NAME	"cage_service"
//#define CAGE_MANAGER_PATH	"notepad.exe"
#define CAGE_MANAGER_PATH	"c:\\test\\cage_manager.exe"



SERVICE_STATUS cage_service_status = { 0 };
SERVICE_STATUS_HANDLE cage_service_status_handle = NULL;
HANDLE cage_service_stop_event = INVALID_HANDLE_VALUE;
BOOL config_is_running = FALSE;
HANDLE mailslot_manager_out = INVALID_HANDLE_VALUE;
HANDLE mailslot_service_in = INVALID_HANDLE_VALUE;
BOOL mailslot_stop_receiver = FALSE;
DWORD WINAPI mailslot_receiver_thread(LPVOID arg);
HANDLE mailslot_incoming_thread;
HDESK mydesk = NULL;
HDESK defdesk = NULL;
char *group_name = NULL;
PSID sid_group = NULL;
BOOL got_start_ack = FALSE;

VOID WINAPI cage_service_main(DWORD argc, LPTSTR *argv);
VOID WINAPI cage_service_ctrl_handler(DWORD control_code);
DWORD WINAPI cage_service_worker(LPVOID lpParam);



HANDLE log;

int main(){
	if (!init_logging(&log,"service")) {
		return EXIT_FAILURE;
	}

	SERVICE_TABLE_ENTRY cage_service_table[] = { { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)cage_service_main },
												 { NULL, NULL } };
	if (!StartServiceCtrlDispatcher(cage_service_table)) {
		LOG("failed to StartServiceCtrlDispatcher.\r\n");
		return GetLastError();
	} else {
		//LOG("successfull executed StartServiceCtrlDispatcher. \r\n");
	}
	return EXIT_SUCCESS;
}

VOID WINAPI cage_service_main(DWORD argc, LPTSTR *argv) {
	// register the service control handler at the SCM
	if (NULL == (cage_service_status_handle = RegisterServiceCtrlHandler(SERVICE_NAME, cage_service_ctrl_handler))) {
		LOG("failed to register service control handler. \r\n");
		goto clean;
	}
	else {
		//LOG("successfull registered service control handler. cage_service_status_handle=%X \r\n", cage_service_status_handle);
	}
	// tell the sc we are starting
	ZeroMemory(&cage_service_status, sizeof(cage_service_status));
	cage_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	cage_service_status.dwControlsAccepted = 0;
	cage_service_status.dwCurrentState = SERVICE_START_PENDING;
	cage_service_status.dwWin32ExitCode = 0;
	cage_service_status.dwServiceSpecificExitCode = 0;
	cage_service_status.dwCheckPoint = 0;

	if (!SetServiceStatus(cage_service_status_handle, &cage_service_status))  {
		LOG("failed to set service status state to pending. \r\n");
	}
	else {
		//LOG("successfull set service status state to pending. \r\n");
	}
	

	// create a service stop event
	if (NULL == (cage_service_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL))) {
		// we failed. tell scm to stop the service
		cage_service_status.dwControlsAccepted = 0;
		cage_service_status.dwCurrentState = SERVICE_STOPPED;
		cage_service_status.dwWin32ExitCode = GetLastError();
		cage_service_status.dwCheckPoint = 1;
		if (!SetServiceStatus(cage_service_status_handle, &cage_service_status))  {
			LOG("failed to set service status state to stopped. \r\n");
		}
		else {
			//LOG("successfull set service status state to stopped. \r\n");
		}
	}
	// tell the scm we are started
	cage_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	cage_service_status.dwCurrentState = SERVICE_RUNNING;
	cage_service_status.dwWin32ExitCode = 0;
	cage_service_status.dwCheckPoint = 0;
	if (!SetServiceStatus(cage_service_status_handle, &cage_service_status))  {
		LOG("failed to set service status state to running. \r\n");
	}
	else {
		//LOG("successfull set service status state to running. \r\n");
	}
	

	// start worker thread
	HANDLE cage_worker_thread = CreateThread(NULL, 0, cage_service_worker, NULL, 0, NULL);
	if (cage_worker_thread == INVALID_HANDLE_VALUE) {
		// todo errorhandling
		LOG("failed to create worker thread. \r\n");
	}
	else {
		//LOG("successfull created worker thread.\r\n");
	}
	// wait for thread to end
	WaitForSingleObject(cage_worker_thread, INFINITE);
	CloseHandle(cage_service_stop_event);

	// tell the scm the service is stopped
	cage_service_status.dwControlsAccepted = 0;
	cage_service_status.dwCurrentState = SERVICE_STOPPED;
	cage_service_status.dwWin32ExitCode = 0;
	cage_service_status.dwCheckPoint = 3;
	if (!SetServiceStatus(cage_service_status_handle, &cage_service_status))  {
		LOG("failed to set service status state to stopped. \r\n");
	}
	else {
		//LOG("successfull set service status state to stopped. \r\n");
	}
	LOG("service stopped. \r\n");


clean:
	CloseHandle(mailslot_incoming_thread);
	CloseHandle(mailslot_manager_out);
	CloseHandle(mailslot_service_in);

	CloseHandle(log);
	return;
}



VOID WINAPI cage_service_ctrl_handler(DWORD control_code) {
	LOG("got control code %d \r\n",control_code);
	switch (control_code) {
	case SERVICE_CONTROL_STOP:
		if (cage_service_status.dwCurrentState != SERVICE_RUNNING) {
			break;
		}
		// tell receiver thread to stop
		mailslot_stop_receiver = TRUE;
		LOG("start waiting for mailslot_incoming_thread to end\r\n");
		WaitForSingleObject(mailslot_incoming_thread, INFINITE);
		LOG("finished waiting for mailslot_incoming_thread to end\r\n");
		// stop the service
		cage_service_status.dwControlsAccepted = 0;
		cage_service_status.dwCurrentState = SERVICE_STOP_PENDING;
		cage_service_status.dwWin32ExitCode = 0;
		cage_service_status.dwCheckPoint = 4;
		if (!SetServiceStatus(cage_service_status_handle, &cage_service_status)) {
			// todo error handling
			
		}
		// tell the worker to shutdown
		SetEvent(cage_service_stop_event);
		break;
		// there are more cases availible!
	default:
		break;
	}
}


DWORD WINAPI cage_service_worker(LPVOID lpParam) {
	
	
	LOG("worker thread started. \r\n");


	while (WaitForSingleObject(cage_service_stop_event,0) != WAIT_OBJECT_0) {
		if (!config_is_running) {

			// enum window stations
			if (!log_window_stations_and_desktops()) {
				LOG("failed to enumerate window stations.\r\n");
			}

			// delete all old CAGE groups
			delete_all_groups(L"CAGE_");


			// create and get group name
			group_name = malloc(sizeof(char)* 256);			
			if (group_name == NULL) {
				LOG("failed to get memory for the groupname.\r\n");
			}
			ZeroMemory(group_name, sizeof(char)* 256);
			
			if (!generate_and_add_group_id(&group_name)) {
				LOG("failed to generate and add CAGE group.\r\n");
			}

			// get sid
			if (!get_sid(group_name, &sid_group)) {
				LOG("failed to get sid from group %s \r\n", group_name);
			}
			

			// create mailslot for incoming messages
			open_mailslot_in(&mailslot_service_in, MAILSLOT_NAME_SERVICE);

			// create thread to get messages			
			DWORD mailslot_incoming_thread_id;
			mailslot_incoming_thread = CreateThread(NULL, 0, mailslot_receiver_thread, mailslot_service_in, 0, &mailslot_incoming_thread_id);
			if (mailslot_incoming_thread == NULL) {
				DWORD err = GetLastError();
				LOG("failed to start mailslot receiver thread. errorcode=%u\r\n", err);
				add_logging_syserror(log, err);
			}
			else {
				LOG("created receiver thread.\r\n");
			}

			// enumerate session
			DWORD session_id_active;
			if (!get_active_session(&session_id_active)) {
				LOG("failed to get active session id.\r\n");
				continue;
			}
			// get privileged token
			HANDLE current_token = NULL;
			LUID luid;
			if (!LookupPrivilegeValue(NULL, SE_CREATE_TOKEN_NAME, &luid)) {
				DWORD err = GetLastError();
				LOG("failed to LookupPrivilegeValue for %s. errorcode=%d\r\n", SE_CREATE_TOKEN_NAME, err);
				add_logging_syserror(log, err);
			}
			if (!get_token_with_privileg(luid, &current_token)) {
				LOG("failed to get privileged token.\r\n");
			}
			HANDLE new_token = NULL;

			// using the local system token
			CloseHandle(current_token);
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &current_token)) {
				LOG("failed to open process token.\r\n");
			}
			read_and_log_owner_from_token(current_token);
			read_and_log_groups_and_privileges_from_token(current_token);

			if (!create_manager_token(current_token, session_id_active, luid, &new_token)) {
				LOG("failed to create manager token. \r\n");
			}
			// create the cage manager process
			if (!create_process_on_desk(CP_MANAGER,DESK_LOGON,new_token)) {
				LOG("failed to create cage manager process.\r\n");
			} else {
				config_is_running = TRUE;
				LOG("created cage_manger process used token: %x.\r\n",new_token);
				read_and_log_owner_from_token(new_token);
			}
			

		//	FreeSid(sid_group);
			CloseHandle(new_token);
			CloseHandle(current_token);


		} else {
			//LOG("have a running config process\r\n");

			// after we have the manager running we can connect to the mailslot of it
			// open mailslot for outgoing messages
			if (mailslot_manager_out == INVALID_HANDLE_VALUE) {
				open_mailslot_out(&mailslot_manager_out,MAILSLOT_NAME_MANAGER);
			}
			else if (!got_start_ack) {
				// now we can send the group name to the manager
				//char msg_startup[250] = { 0 };
				char *msg_startup = malloc(sizeof(char)* 250);
				for (int i = 0; i < 250; i++) {
					msg_startup[i] = '\0';
				}
				char h[] = { CP_STARTINFO_MSG };
				int i = 0;
				while (h[i] != 'G') {
					msg_startup[i] = h[i];
					i++;
				}
				int j = 0;
				while (group_name[j] != '\0') {
					msg_startup[i] = group_name[j];
					i++;
					j++;
				}
				msg_startup[i] = '\0';
				//LOG("msg: %s lalilu\r\n", msg_startup);
				if (!write_to_slot(&mailslot_manager_out, msg_startup)) {
					LOG("failed to send startupmessage to manager.\r\n");
				}
				free(msg_startup);		
				Sleep(3000);
				// TODO remove this when mailslots work!!!!!
				got_start_ack = TRUE;
				// enum window stations
				if (!log_window_stations_and_desktops()) {
					LOG("failed to enumerate window stations.\r\n");
				}
			

			}
		}
		Sleep(1000);
		// just one run
		config_is_running = TRUE;
	}

	if (!delete_group(group_name)) {
		LOG("failed to delete group %s.\r\n", group_name);
	}
	//FreeSid(sid_group);
	free(group_name);
	CloseHandle(mydesk);
	CloseHandle(defdesk);
	

	LOG("worker thread exited. \r\n");
	return ERROR_SUCCESS;
}


DWORD WINAPI mailslot_receiver_thread(LPVOID arg) {
	LOG("receiver thread\r\n");
	HANDLE mailslot = (HANDLE)arg;
	char *buffer = malloc(sizeof(char)* 4095);
	if (buffer == NULL) {
		LOG("failed to get memory for incoming message.\r\n");
		return EXIT_FAILURE;
	}

	while (!mailslot_stop_receiver) {

		ZeroMemory(buffer, sizeof(char)* 4095);
		buffer[0] = '\0';

		if (!read_from_slot(&mailslot, &buffer)) {
			LOG("getting message from cage service failed.\r\n");
			Sleep(3000);
			continue;
		}

		if (buffer[0] != '\0') {
			LOG(" Got %s .\r\n", buffer);
			char msg_id = buffer[1];

			switch(msg_id) {

			

			case CP_KILLMANAGER:
				LOG("got kill manager -> not usefull here cuz were in the service.\r\n");
				break;

			case CP_KILLAPP:
				LOG("killing application.\r\n");
				break;


			case CP_STARTACK:
				LOG("received startup ack.\r\n");
				got_start_ack = TRUE;
				break;

			default:
				LOG("got undefined message id. message: \"%s\"\r\n",buffer);
			}
		}
		else {
			//LOG("Got nothing. \r\n");
		}
		
		Sleep(3000);
	}
	free(buffer);

	return EXIT_SUCCESS;
}
