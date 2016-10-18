///
/// \file protocol.h
/// \author Richard Heininger - richmont12@gmail.com
/// shark cage - cage lib - protocol definition
///
#ifndef __protocol_h__
#define __protocol_h__

#define CP_TESTAPP			"notepad.exe"
#define CP_LABELLER			"c:\\test\\cage_labeller.exe"
#define CP_MANAGER			"c:\\test\\cage_manager.exe"


#define CP_STARTINFO		'0'
#define CP_STARTINFO_NO		"00\0"
#define CP_STARTINFO_MSG	"00_starting info -GROUP-\0"
#define CP_STARTAPP			'1'
#define CP_STARTAPP_NO		"01\0"
#define CP_STARTAPP_MSG		"01_starting application --XX\0"		// where XX is the application number
#define CP_KILLAPP			'2'
#define CP_KILLAPP_NO		"02\0"
#define CP_KILLAPP_MSG		"02_killing application --XX\0"
#define CP_KILLMANAGER		'3'
#define CP_KILLMANAGER_NO	"03\0"
#define CP_KILLMANAGER_MSG	"03_killing cage manager\0"
#define CP_GOTOCAGE			'4'
#define CP_GOTOCAGE_NO		"04\0"
#define CP_GOTOCAGE_MSG		"04_open caged desktop\0"
#define CP_GOTODESK			'5'
#define CP_GOTODESK_NO		"05\0"
#define CP_GOTODESK_MSG		"05_open default desktop\0"
#define CP_CLEANCAGE		'6'
#define CP_CLEANCAGE_NO		"06\0"
#define CP_CLEANCAGE_MSG	"06_clean caged desktop\0"
#define CP_STARTACK			'7'
#define CP_STARTACK_NO		"07\0"
#define CP_STARTACK_MSG		"07_ack for startup information\0"
#define CP_APPINFO			'8'
#define CP_APPINFO_NO		"08\0"
#define CP_APPINFO_MSG		"08_appinfo \0"// -X -Y -Z\0" // where x is the id, y the file and z the window name


#endif