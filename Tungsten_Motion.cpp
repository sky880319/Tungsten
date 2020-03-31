#include "Tungsten_Motion.h"
#include <iostream>
#include <windows.h>

#define SAFE_DELETE(pPtr) { delete pPtr; pPtr = NULL; }

ScRobot::ScRobot()
{
	sessionIdx = 0;
	makeID = 23594510;
	encStr = "0B9287F3AE9D949A7751D8C8E51A50BE46FBA406D7E9CE0B";
	connIP = "10.0.0.31";
	setting = new DLL_USE_SETTING();
	memset(setting, 0, sizeof(DLL_USE_SETTING));

	Initialize();
}

ScRobot::~ScRobot()
{
	Uninitialize();
	SAFE_DELETE(setting);
}

bool ScRobot::Initialize()
{
	setting->SoftwareType = 5;
	setting->TalkInfoNum = 10;
	setting->MemSizeI = I_NUM;
	setting->MemSizeO = O_NUM;
	setting->MemSizeC = C_NUM;
	setting->MemSizeS = S_NUM;
	setting->MemSizeA = A_NUM;
	setting->MemSizeR = R_NUM;
	setting->MemSizeF = F_NUM;
	setting->MemSizeTT = 0;
	setting->MemSizeCT = 0;
	setting->MemSizeTS = 0;
	setting->MemSizeTV = 0;
	setting->MemSizeCS = 0;
	setting->MemSizeCV = 0;

	if (100 != scif_Init(setting, makeID, (char*)encStr))
	{
		std::cout << "[ScRobot] DLL initialization failed!" << std::endl;
		return false;
	}
	if (1 != scif_LocalConnectIP(sessionIdx, (char*)connIP))
	{
		std::cout << "[ScRobot] Connection failed!" << std::endl;
		return false;
	}

	return false;
}

bool ScRobot::Uninitialize()
{
	// Cleanup API
	scif_Disconnect(sessionIdx);
	scif_Destroy();
	std::cout << "[ScRobot] Cleanup successed!" << std::endl;

	return true;
}

bool ScRobot::Connect()
{
	// SC_DEFAULT_CMD: Will run with SC_POLLING_CMD and be used to monitor the multiple controllers simultaneously.
		// SC_POLLING_CMD: Used to synchronize the data from controller and PC.
		// SC_DIRECT_CMD:  It will process the one-time command with the highest priority.
		// 
		// Example.
		// unsigned int addr[] = { 3001,3003,3005 };
		// scif_cb_ReadR(SC_POLLING_CMD, 0, 3000, addr);	// Synchronize non-consecutive data addresses.

	scif_StartCombineSet(sessionIdx);
	scif_cmd_ReadR(SC_POLLING_CMD, sessionIdx, 8503, 3);		// Synchronize consecutive data addresses.
	scif_FinishCombineSet(sessionIdx);

	// Wait for the controller accept connection.
	while (1)
	{
		std::cout << "[RsRobot] Waiting for HIWIN Scara to respond..." << std::endl;
		if (SC_CONN_STATE_OK == scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
		{
			std::cout << "[RsRobot] Data setting successed." << std::endl;
			break;
		}
		Sleep(1000);
	}

	return false;
}

//// Set point value of each axis for NC-Prog.
//scif_cmd_WriteR(sessionIdx, 8503, 100);	//X
//scif_cmd_WriteR(sessionIdx, 8503, 100);	//Y
//scif_cmd_WriteR(sessionIdx, 8503, 50);	//C
//
//// Open NC-prog file
//scif_cmd_WriteR(sessionIdx, 17004, 300);
//
//// Run NC-prog
//scif_cmd_WriteC(sessionIdx, 22, 1);