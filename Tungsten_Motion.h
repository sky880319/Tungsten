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

    short sessionIdx;
    int makeID;
    const char* encStr;
    const char* connIP;

    DLL_USE_SETTING* setting;
};

std::ostream& operator<<(std::ostream& out, TgPoint& ref);
std::ostream& operator<<(std::ostream& out, TgWorld& ref);

#endif