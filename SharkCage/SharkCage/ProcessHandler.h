#pragma once
class ProcessHandler
{
public:
	/*
	 * Creates and starts a new process.
	 */
	void CreateProcess(const std::wstring &desktop_name /*, SECURITY_DESCRIPTOR sd */);	// Add argument to specify programm to be started
};
