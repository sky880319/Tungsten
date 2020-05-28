#include "Tungsten_Motion.h"
#include <iostream>

#define SAFE_DELETE(pPtr) { delete pPtr; pPtr = NULL; }
#define TIMEOUT 20

ScRobot::ScRobot(int session)
{
	sessionIdx = session;
	makeID = 23594510;
	encStr = "0B9287F3AE9D949A7751D8C8E51A50BE46FBA406D7E9CE0B";
	connIP = "192.168.0.201";
	setting = new DLL_USE_SETTING();
	memset(setting, 0, sizeof(DLL_USE_SETTING));
}

ScRobot::~ScRobot()
{
	//StopProc();
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

	unsigned int r_addr[13] = { REG_X_AXIS, REG_Y_AXIS, REG_Z_AXIS, 
								REG_WHILE_STATUS, REG_ALLOW_RUN,
								FEATURE_LOAD_NCFILE, GET_PATH_STATUS,
								GET_LOCATION_WX, GET_LOCATION_WY, GET_LOCATION_WZ, GET_LOCATION_WC,
								8001, 8002};
	unsigned int c_addr[1] = { SET_PROG_STATUS };
	unsigned int sS_addr[1] = { GET_PROG_STATUS };

	scif_StartCombineSet(sessionIdx);
	// Synchronize non-consecutive data addresses.
	scif_cb_ReadR(SC_POLLING_CMD, sessionIdx, sizeof(r_addr) / sizeof(r_addr[0]), r_addr);
	scif_cb_ReadC(SC_POLLING_CMD, sessionIdx, sizeof(c_addr) / sizeof(c_addr[0]), c_addr);
	scif_cb_ReadS(SC_POLLING_CMD, sessionIdx, sizeof(sS_addr) / sizeof(sS_addr[0]), sS_addr);
	scif_FinishCombineSet(sessionIdx);

	// Wait for the controller accept connection.
	int time = 0;
	std::cout << "[RsRobot] Waiting for HIWIN Scara to respond...";
	while (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		if (time > TIMEOUT)
		{
			std::cout << std::endl << "[RsRobot] Data setting failed." << std::endl;
			return false;
		}

		std::cout << ".";
		Sleep(1000);
	}
	std::cout << std::endl;
	std::cout << "[RsRobot] Data setting successed." << std::endl;


	//RunProc();
	//SetProc(TgWorld(HOME_X, HOME_Y, 180.0f, 0.f));

	return true;
}

bool ScRobot::RefreshWorldLocation(TgWorld& ref)
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[TgMotion] Can't refresh world location data. (Reason: Failed to connect Scara.)" << std::endl;
		return false;
	}

	ref.set((int)scif_ReadR(GET_LOCATION_WX), (int)scif_ReadR(GET_LOCATION_WY), (int)scif_ReadR(GET_LOCATION_WZ), (int)scif_ReadR(GET_LOCATION_WC));
	//std::cout << ref << std::endl;
	return true;
}

bool ScRobot::VelocityTest()
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[TgMotion] Connection failed." << std::endl;
		return false;
	}

	if (COND_WHILE_RUN)
	{
		std::cout << "[TgMotion] Scara NC proccess is already running." << std::endl;
		return false;
	}

	// First, go to tghome point record.
	TG_WAIT(COND_PATH_STATUS(1));

	int res1 = scif_cmd_WriteR(sessionIdx, FEATURE_LOAD_NCFILE, NC_MOVE_TO_HOME);
	int res2 = scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 1);
	//Sleep(500);

	TG_WAIT(COND_PROG_START);
	TG_WAIT(COND_PROG_END);
	TG_WAIT(COND_PATH_STATUS(1));

	TgWorld home((int)scif_ReadR(GET_LOCATION_WX),
				 (int)scif_ReadR(GET_LOCATION_WY),
				 (int)scif_ReadR(GET_LOCATION_WZ),
				 (int)scif_ReadR(GET_LOCATION_WC));


	// Second, go to world location (, , ,) and counting the times.
	TgWorld location(-200.889f, 328.697f, 130.000f, 0.f);
	
	scif_cmd_WriteR(sessionIdx, REG_X_AXIS, location.x);	//X
	scif_cmd_WriteR(sessionIdx, REG_Y_AXIS, location.y);	//Y
	scif_cmd_WriteR(sessionIdx, REG_Z_AXIS, location.z);	//Z

	TG_WAIT(COND_PATH_STATUS(1));

	scif_cmd_WriteR(sessionIdx, FEATURE_LOAD_NCFILE, NC_MOVE_TO_POINT);
	scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 1);

	TgWorld current;
	time_pt start, end;
	//long long flag = 1;

	while (true)
	{
		current = TgWorld((int)scif_ReadR(GET_LOCATION_WX),
						  (int)scif_ReadR(GET_LOCATION_WY),
						  (int)scif_ReadR(GET_LOCATION_WZ),
						  (int)scif_ReadR(GET_LOCATION_WC));

		if (current == home)
		{
			start = std::chrono::steady_clock::now();
			continue;
		}
		else if (current == location)
		{
			end = std::chrono::steady_clock::now();
			break;
		}

		//long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		//if (duration >= flag * 100)
		//{
		//	float distance = home.getDistance(current);
		//	float velocity = distance / duration * 1000;
		//	std::cout << "Current:" << current << " | V ~= " << velocity << "m/s | Distance: " << distance << "mm | Duration: " << duration << "ms" << std::endl;
		//	flag++;
		//}
	}

	long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	float distance = home.getDistance(current);
	float velocity = distance / duration * 1000;
	std::cout << "Finish:" << current << " | V ~= " << velocity << "mm/s | Distance: " << distance << "mm | Duration: " << duration << "ms" << std::endl;
	Sleep(100);

	return true;
}

bool ScRobot::Move(const TgWorld& ref)
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[TgMotion] Connection failed." << std::endl;
		return false;
	}

	if (COND_WHILE_RUN)
	{
		std::cout << "[TgMotion] Scara NC proccess is already running." << std::endl;
		return false;
	}

	std::cout << "Move to: world" << ref << std::endl;

	if (ref.x >= WORKING_LIMIT_X1 && ref.x <= WORKING_LIMIT_X2 &&
		ref.y >= WORKING_LIMIT_Y1 && ref.y <= WORKING_LIMIT_Y2 &&
		ref.z >= WORKING_LIMIT_Z1 && ref.z <= WORKING_LIMIT_Z2)
	{
		scif_cmd_WriteR(sessionIdx, REG_X_AXIS, ref.x);	//X
		scif_cmd_WriteR(sessionIdx, REG_Y_AXIS, ref.y);	//Y
		scif_cmd_WriteR(sessionIdx, REG_Z_AXIS, ref.z);	//Z
		
		//if ((int)scif_ReadR(GET_PATH_STATUS) != 1)
		//{
		//	std::cout << "[TgMotion] Failed to moving scara. (Reason: Scara not ready.)" << std::endl;
		//	return false;
		//}

		TG_WAIT(COND_PATH_STATUS(1));

		scif_cmd_WriteR(sessionIdx, FEATURE_LOAD_NCFILE, NC_MOVE_TO_SUCK);
		scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 1);
		/*Sleep(2000);
		scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 0);*/

		/*Sleep(100);
		WAIT_PROG_END;*/
		TG_WAIT(COND_PROG_START);
		TG_WAIT(COND_PROG_END);
		TG_WAIT(COND_PATH_STATUS(1));
		Sleep(100);

		return true;
	}

	std::cout << "[TgMotion] Failed to moving scara. (Reason: reached the upper limit.)" << std::endl;
	return false;
}

bool ScRobot::RunNC(int fileIdx)
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[TgMotion] Connection failed." << std::endl;
		return false;
	}

	scif_cmd_WriteR(sessionIdx, FEATURE_LOAD_NCFILE, fileIdx);
	scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 1);

	TG_WAIT(COND_PROG_START);
	TG_WAIT(COND_PROG_END);

	return true;
}

bool ScRobot::RunProc()
{
	if (!m_prog)
	{
		return false;
	}

	return m_prog->RunProc();
}

bool ScRobot::StopProc()
{
	if (!m_prog)
	{
		return false;
	}

	return m_prog->StopProc();
}

bool ScRobot::SetProc(const TgWorld& ref)
{
	if (!m_prog)
	{
		return false;
	}

	return m_prog->SetProc(ref);
}

//
//bool ScRobot::RunProc()
//{
//	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
//	{
//		std::cout << "[TgMotion] Connection failed." << std::endl;
//		return false;
//	}
//	if (COND_PROG_START)
//	{
//		std::cout << "[TgMotion] Scara NC proccess is already running." << std::endl;
//		return false;
//	}
//
//	TG_WAIT(COND_PATH_STATUS(1));
//
//	scif_cmd_WriteR(sessionIdx, REG_X_AXIS, HOME_X * UNIT_TRANSFORM);	//X
//	scif_cmd_WriteR(sessionIdx, REG_Y_AXIS, HOME_Y * UNIT_TRANSFORM);	//Y
//	scif_cmd_WriteR(sessionIdx, REG_Z_AXIS, HOME_Z * UNIT_TRANSFORM);	//Z
//	scif_cmd_WriteR(sessionIdx, REG_WHILE_STATUS, 1); //Run while
//	scif_cmd_WriteR(sessionIdx, REG_ALLOW_RUN, 0); //Not allow move
//
//	scif_cmd_WriteR(sessionIdx, FEATURE_LOAD_NCFILE, NC_MAIN_PROC);
//	int res = scif_cmd_WriteC(sessionIdx, SET_PROG_STATUS, 1);
//
//	TG_WAIT(COND_PROG_START);
//	return res != 0;
//}
//
//bool ScRobot::StopProc()
//{
//	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
//	{
//		std::cout << "[TgMotion] Connection failed." << std::endl;
//		return false;
//	}
//	if (COND_PROG_END && COND_WHILE_STOP)
//	{
//		std::cout << "[TgMotion] Failed to stop proccess. (Reason: Scara NC proccess is not running.)" << std::endl;
//		return false;
//	}
//	
//	// GO home.
//	SetProc(TgWorld(HOME_X, HOME_Y, 180.0f, 0.f));
//
//	TG_WAIT(COND_NOT_ALLOW_RUN);
//	scif_cmd_WriteR(sessionIdx, REG_WHILE_STATUS, 0); //Stop while
//
//	TG_WAIT(COND_PROG_END);
//	Sleep(100);
//
//	return true;
//}
//
//bool ScRobot::SetProc(const TgWorld& ref)
//{
//	time_pt start = std::chrono::steady_clock::now();
//	if (SC_CONN_STATE_OK != scif_GetTalkMsg(sessionIdx, SCIF_CONNECT_STATE))
//	{
//		std::cout << "[TgMotion] Connection failed." << std::endl;
//		return false;
//	}
//	if (COND_PROG_END && COND_WHILE_STOP)
//	{
//		std::cout << "[TgMotion] Failed to set proccess to moving scara. (Reason: Scara NC proccess is not running.)" << std::endl;
//		return false;
//	}
//
//	TG_WAIT(COND_NOT_ALLOW_RUN);
//
//	if (ref.x >= WORKING_LIMIT_X1 && ref.x <= WORKING_LIMIT_X2 &&
//		ref.y >= WORKING_LIMIT_Y1 && ref.y <= WORKING_LIMIT_Y2 &&
//		ref.z >= WORKING_LIMIT_Z1 && ref.z <= WORKING_LIMIT_Z2)
//	{
//		if (0 != scif_cmd_WriteR(sessionIdx, REG_X_AXIS, ref.x) && 
//			0 != scif_cmd_WriteR(sessionIdx, REG_Y_AXIS, ref.y) && 
//			0 != scif_cmd_WriteR(sessionIdx, REG_Z_AXIS, ref.z))
//		{
//			scif_cmd_WriteR(sessionIdx, REG_ALLOW_RUN, 1); //Allow move
//
//			TG_WAIT(COND_ALLOW_RUN);
//
//			//time_pt start = std::chrono::steady_clock::now();
//			/*TG_WAIT(COND_CUST_R_VAL(8001, 1));
//			long long dur1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();*/
//			//TG_WAIT(COND_CUST_R_VAL(8002, 1));
//		 //   long long dur2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
//			//long long total_dur = dur2 - dur1;
//			//float dist = ref.getDistance(TgWorld(HOME_X, HOME_Y, 180.0f, 0.f));
//			//std::cout << "移動距離：" << dist << "mm ，通訊時間：" << dur1 << "ms ，移動時間：" << total_dur << "ms ，平均速度：" << dist / ((float)total_dur / 1000) << std::endl;
//
//			//std::cout << "通訊時間：" << dur1 << "ms" << std::endl;
//
//			TG_WAIT(COND_NOT_ALLOW_RUN);
//
//			return true;
//		}
//
//		return false;
//	}
//
//	return false;
//}

std::ostream& operator<<(std::ostream& out, const TgPoint& ref)
{
	out << "Point("
		<< ref.X()
		<< ", " << ref.Y()
		<< ", " << ref.Z()
		<< ")";
	return out;
}

std::ostream& operator<<(std::ostream& out, const TgWorld& ref)
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