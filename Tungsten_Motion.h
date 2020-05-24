#ifndef _TUNGSTEN_MOTION_H
#define _TUNGSTEN_MOTION_H
#pragma once

#ifndef __VC
    #define __VC
    #include <scif.h>
#else
    #include <scif.h>
#endif

#include <iostream>
#include "Tungsten_Define.h"

// Register Num (Memory address of Scara):
#define GET_LOCATION_WX 6321
#define GET_LOCATION_WY 6322
#define GET_LOCATION_WZ 6323
#define GET_LOCATION_WC 6326
#define GET_PATH_STATUS 6037
#define SET_PROG_STATUS 22
#define GET_PROG_STATUS 22
#define FEATURE_LOAD_NCFILE 17004

// Customize Register Num
#define REG_X_AXIS       8503
#define REG_Y_AXIS       8504
#define REG_Z_AXIS       8505
#define REG_WHILE_STATUS 8506
#define REG_ALLOW_RUN    8507

// NC file number:
#define NC_MOVE_TO_POINT 520
#define NC_MOVE_TO_SUCK  521
#define NC_MOVE_TO_HOME  522
#define NC_MAIN_PROC     523

//Wait the program finish by check S22 value become 0.
//#define WAIT_PROG_END while ((int)scif_ReadS(GET_PROG_STATUS) == 1) { Sleep(10); }
//Wait the program finish by check S22 value become 1.
//#define WAIT_PROG_START while ((int)scif_ReadS(GET_PROG_STATUS) == 0) { Sleep(10); }
//Wait the scara ready by check R6037 value become 1.
//#define WAIT_SCARA_READY while ((int)scif_ReadR(GET_PATH_STATUS) != 1) { std::cout << "[Scara] Can't refresh world location data. (Reason: Scara not ready.)" << std::endl; Sleep(10); }

//Wait "status_cond" become true.
#define TG_WAIT(status_cond) while(!(status_cond)) { Sleep(1); }
#define COND_PROG_END         ((int)scif_ReadS(GET_PROG_STATUS)  == 0)
#define COND_PROG_START       ((int)scif_ReadS(GET_PROG_STATUS)  == 1)
#define COND_PATH_STATUS(s)   ((int)scif_ReadR(GET_PATH_STATUS)  == s)
#define COND_WHILE_RUN        ((int)scif_ReadR(REG_WHILE_STATUS) == 1)
#define COND_WHILE_STOP       ((int)scif_ReadR(REG_WHILE_STATUS) == 0)
#define COND_ALLOW_RUN        ((int)scif_ReadR(REG_ALLOW_RUN)    == 1)
#define COND_NOT_ALLOW_RUN    ((int)scif_ReadR(REG_ALLOW_RUN)    == 0)
#define COND_CUST_R_VAL(r, v) ((int)scif_ReadR(r)                == v)

class ScRobot
{
public:
    ScRobot();
    ~ScRobot();

    bool Connect();
    short GetSessionIdx() { return sessionIdx; }

    bool RefreshWorldLocation(TgWorld& ref);
    bool VelocityTest();
    bool Move(const TgWorld& ref);
	bool RunNC(int fileIdx);
    bool RunProc();
    bool StopProc();
    bool SetProc(const TgWorld& ref);

private:
    bool Initialize();
    bool Uninitialize();

    char sessionIdx;
    int makeID;
    const char* encStr;
    const char* connIP;

    DLL_USE_SETTING* setting;
};

std::ostream& operator<<(std::ostream& out, const TgPoint& ref);
std::ostream& operator<<(std::ostream& out, const TgWorld& ref);

#endif