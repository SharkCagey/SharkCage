#pragma once

#include "tokenManipulation.h"

namespace tokenLib {
	/**
	* Creates a new local group with a name groupName and returns it´s SID. To deallocate returned sid, use destroySid() fucntion.
	* @param groupName string literal representing the name of the group (IN)
	* @param sid reference to the new group sid, set to NULL if function fails (OUT)
	* @return true if success
	**/
	DLLEXPORT bool createLocalGroup(LPWSTR groupName, PSID &sid);

	/**
	* Deletes a local group named groupName
	* @param groupName name of the group to be deleted
	* @return true if success
	**/
	DLLEXPORT bool deleteLocalGroup(LPWSTR groupName);

	/**
	* Deallocates an SID returned by createLocalGroup() function and sets the pointer to NULL;
	* @param sid pointer to sid alllocated by createLocalGroup()
	* @return true if success
	**/
	DLLEXPORT bool destroySid(PSID &sid);

}