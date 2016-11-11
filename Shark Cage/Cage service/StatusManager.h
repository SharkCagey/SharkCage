#pragma once

#ifndef STAT_MNGR
#define STAT_MNGR

enum DeviceStatus
{
	STATE_0,
	STATE_1,
	STATE_2,
	STATE_3
};

//holds the current status of hte machine
class StatusManager{

private:
	DeviceStatus m_status = STATE_0;
	
public:
	//use when entering new state
	void setStatus(DeviceStatus status) {
		this->m_status = status;
		return;
	}

	//for retrieving info about current state
	DeviceStatus getStatus() {
		return m_status;
	}
};


#endif