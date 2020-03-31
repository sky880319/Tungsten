#ifndef _TUNGSTEN_MOTION_H
#define _TUNGSTEN_MOTION_H
#pragma once

#ifndef __VC
    #define __VC
    #include <scif.h>
#else
    #include <scif.h>
#endif

class ScRobot
{
public:
    ScRobot();
    ~ScRobot();

    bool Connect();

private:
    bool Initialize();
    bool Uninitialize();

    short sessionIdx;
    int makeID;
    const char* encStr;
    const char* connIP;

    DLL_USE_SETTING* setting;
};

#endif