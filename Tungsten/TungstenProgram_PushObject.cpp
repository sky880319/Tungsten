#include "..\Tungsten_Motion.h"
#include "TungstenProgram_PushObject.h"
#ifndef __VC
#define __VC
#include <scif.h>
#else
#include <scif.h>
#endif

bool TgProg_PushObject::RunProc()
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(m_sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[TgMotion] Connection failed." << std::endl;
		return false;
	}
	if (COND_PROG_START)
	{
		std::cout << "[TgMotion] Scara NC proccess is already running." << std::endl;
		return false;
	}

	TG_WAIT(COND_PATH_STATUS(1));

	scif_cmd_WriteR(m_sessionIdx, REG_X_AXIS, HOME_X * UNIT_TRANSFORM);	//X
	scif_cmd_WriteR(m_sessionIdx, REG_Y_AXIS, HOME_Y * UNIT_TRANSFORM);	//Y
	scif_cmd_WriteR(m_sessionIdx, REG_Z_AXIS, 180.f);	//Z
	scif_cmd_WriteR(m_sessionIdx, REG_WHILE_STATUS, 1); //Run while
	scif_cmd_WriteR(m_sessionIdx, REG_ALLOW_RUN, 0); //Not allow move

	scif_cmd_WriteR(m_sessionIdx, FEATURE_LOAD_NCFILE, m_progNum);
	int res = scif_cmd_WriteC(m_sessionIdx, SET_PROG_STATUS, 1);

	TG_WAIT(COND_PROG_START);
	return res != 0;
}

bool TgProg_PushObject::StopProc()
{
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(m_sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[TgMotion] Connection failed." << std::endl;
		return false;
	}
	if (COND_PROG_END && COND_WHILE_STOP)
	{
		std::cout << "[TgMotion] Failed to stop proccess. (Reason: Scara NC proccess is not running.)" << std::endl;
		return false;
	}

	// GO home.
	SetProc(TgWorld(HOME_X, HOME_Y, 180.0f, 0.f));

	TG_WAIT(COND_NOT_ALLOW_RUN);
	scif_cmd_WriteR(m_sessionIdx, REG_WHILE_STATUS, 0); //Stop while

	TG_WAIT(COND_PROG_END);
	Sleep(100);

	return true;
}

bool TgProg_PushObject::SetProc(const TgWorld& ref)
{
	time_pt start = std::chrono::steady_clock::now();
	if (SC_CONN_STATE_OK != scif_GetTalkMsg(m_sessionIdx, SCIF_CONNECT_STATE))
	{
		std::cout << "[TgMotion] Connection failed." << std::endl;
		return false;
	}
	if (COND_PROG_END && COND_WHILE_STOP)
	{
		std::cout << "[TgMotion] Failed to set proccess to moving scara. (Reason: Scara NC proccess is not running.)" << std::endl;
		return false;
	}

	TG_WAIT(COND_NOT_ALLOW_RUN);

	if (ref.X() >= (float)WORKING_LIMIT_X1 / UNIT_TRANSFORM && ref.X() <= (float)WORKING_LIMIT_X2 / UNIT_TRANSFORM &&
		ref.Y() >= (float)WORKING_LIMIT_Y1 / UNIT_TRANSFORM && ref.Y() <= (float)WORKING_LIMIT_Y2 / UNIT_TRANSFORM &&
		ref.Z() >= (float)WORKING_LIMIT_Z1 / UNIT_TRANSFORM && ref.Z() <= (float)WORKING_LIMIT_Z2 / UNIT_TRANSFORM)
	{
		if (0 != scif_cmd_WriteR(m_sessionIdx, REG_X_AXIS, ref.X() * UNIT_TRANSFORM) &&
			0 != scif_cmd_WriteR(m_sessionIdx, REG_Y_AXIS, ref.Y() * UNIT_TRANSFORM) &&
			0 != scif_cmd_WriteR(m_sessionIdx, REG_Z_AXIS, ref.Z() * UNIT_TRANSFORM))
		{
			scif_cmd_WriteR(m_sessionIdx, REG_ALLOW_RUN, 1); //Allow move

			TG_WAIT(COND_ALLOW_RUN);
			TG_WAIT(COND_NOT_ALLOW_RUN);

			return true;
		}

		return false;
	}

	return false;
}