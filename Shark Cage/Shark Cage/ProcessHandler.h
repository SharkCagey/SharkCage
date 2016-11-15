#pragma once
class ProcessHandler
{
public:
	ProcessHandler();
	~ProcessHandler();

	/*
	 * Creates and starts a new process.
	 */
	void createProcess();	// Add argument to specify programm to be started
};

