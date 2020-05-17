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
#define SET_PROG_STATUS 22
#define FEATURE_LOAD_NCFILE 17004

// NC file number:
#define NC_MOVE_TO_POINT 409

class ScRobot
{
public:
    ScRobot();
    ~ScRobot();

    bool Connect();
    short GetSessionIdx() { return sessionIdx; }

    bool RefreshWorldLocation(TgWorld& ref);
    bool Move(TgWorld& ref);
	bool RunNC(int fileIdx);

private:
    bool Initialize();
    bool Uninitialize();

    char sessionIdx;
    int makeID;
    const char* encStr;
    const char* connIP;

    DLL_USE_SETTING* setting;
};

std::ostream& operator<<(std::ostream& out, TgPoint& ref);
std::ostream& operator<<(std::ostream& out, TgWorld& ref);

#endif