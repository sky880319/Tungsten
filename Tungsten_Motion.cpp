#include "Tungsten_Motion.h"
#include <iostream>
#include <windows.h>

#define SAFE_DELETE(pPtr) { delete pPtr; pPtr = NULL; }
#define TIMEOUT 20

ScRobot::ScRobot()
{
	sessionIdx = 0;
	makeID = 23594510;
	encStr = "0B9287F3AE9D949A7751D8C8E51A50BE46FBA406D7E9CE0B";
	connIP = "192.168.0.201";
	setting = new DLL_USE_SETTING();
	memset(setting, 0, sizeof(DLL_USE_SETTING));
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

	return true;
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
	if (!Initialize())
	{
		return false;
	}
	// SC_DEFAULT_CMD: Will run with SC_POLLING_CMD and be used to monitor the multiple controllers simultaneously.
		// SC_POLLING_CMD: Used to synchronize the data from controller and PC.
		// SC_DIRECT_CMD:  It will process the one-time command with the highest priority.
		// 
		// Example.
		// unsigned int addr[3] = { 3001,3003,3005 };
		// scif_cb_ReadR(SC_POLLING_CMD, 0, 3, addr);		// Synchronize non-consecutive data addresses.
		//scif_cmd_ReadR(SC_POLLING_CMD, sessionIdx, 8503, 3);	// Synchronize consecutive data addresses.

	unsigned int r_addr[8] = { 8503, 8504, 8505, 
								FEATURE_LOAD_NCFILE,
								GET_LOCATION_WX, 
								GET_LOCATION_WY, 
								GET_LOCATION_WZ, 
								GET_LOCATION_WC };
	unsigned int c_addr[1] = { 22 };

	scif_StartCombineSet(sessionIdx);
	// Synchronize non-consecutive data addresses.
	scif_cb_ReadR(SC_POLLING_CMD, sessionIdx, sizeof(r_addr) / sizeof(r_addr[0]), r_addr);
	scif_cb_ReadC(SC_POLLING_CMD, sessionIdx, sizeof(c_addr) / sizeof(c_addr[0]), c_addr);
	scif_FinishCombineSet(sessionIdx);

	// Wait for the controller accept connection.
	int time = 0;
	std::cout << "[RsRobot] Waiting for HIWIN Scara to respond...";
	while (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		if (time <= TIMEOUT)
		{
			std::cout << std::endl << "[RsRobot] Data setting failed." << std::endl;
			return false;
		}

		std::cout << ".";
		Sleep(1000);
	}
	std::cout << std::endl;
	std::cout << "[RsRobot] Data setting successed." << std::endl;

	return true;
}

bool ScRobot::RefreshWorldLocation(TgWorld& ref)
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[Scara] Can't refresh world location data." << std::endl;
		return false;
	}

	ref.set((int)scif_ReadR(GET_LOCATION_WX), (int)scif_ReadR(GET_LOCATION_WY), (int)scif_ReadR(GET_LOCATION_WZ), (int)scif_ReadR(GET_LOCATION_WC));
	std::cout << ref << std::endl;
	return true;
}

bool ScRobot::Move(TgWorld& ref)
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[Scara] Connection failed." << std::endl;
		return false;
	}

	std::cout << "Move to: world" << ref << std::endl;
	scif_cmd_WriteR(sessionIdx, 8503, ref.x);	//X
	scif_cmd_WriteR(sessionIdx, 8504, ref.y);	//Y
	scif_cmd_WriteR(sessionIdx, FEATURE_LOAD_NCFILE, NC_MOVE_TO_POINT);
	scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 1);
	Sleep(5000);
	scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 0);

	return true;
}

bool ScRobot::RunNC(int fileIdx)
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[Scara] Connection failed." << std::endl;
		return false;
	}

	scif_cmd_WriteR(sessionIdx, FEATURE_LOAD_NCFILE, fileIdx);
	scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 1);
	Sleep(5000);
	scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 0);

	return true;
}

std::ostream& operator<<(std::ostream& out, TgPoint& ref)
{
	out << "Point("
		<< ref.X()
		<< ", " << ref.Y()
		<< ", " << ref.Z()
		<< ")";
	return out;
}

std::ostream& operator<<(std::ostream& out, TgWorld& ref)
{
	out << "World("
		<< ref.X()
		<< ", " << ref.Y()
		<< ", " << ref.Z()
		<< ", " << ref.C()
		<< ")";
	return out;
}

//// Set point value of each axis for NC-Prog.
//scif_cmd_WriteR(sessionIdx, 8503, 100);	//X
//scif_cmd_WriteR(sessionIdx, 8504, 100);	//Y
//scif_cmd_WriteR(sessionIdx, 8505, 50);	//C
//
//// Open NC-prog file
//scif_cmd_WriteR(sessionIdx, 17004, 300);
//
//// Run NC-prog
//scif_cmd_WriteC(sessionIdx, 22, 1);