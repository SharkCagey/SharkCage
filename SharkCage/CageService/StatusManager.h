#pragma once

enum class DeviceStatus
{
	STATE_0,  // FIXME: Make the name of the states more descriptive
	STATE_1,
	STATE_2,
	STATE_3
};

// holds the current status of hte machine
class StatusManager
{

private:
	DeviceStatus m_status = DeviceStatus::STATE_0;

public:
	// use when entering new state
	void setStatus(DeviceStatus status)
	{
		this->m_status = status;
		return;
	}

	// for retrieving info about current state
	DeviceStatus getStatus()
	{
		return m_status;
	}
};
