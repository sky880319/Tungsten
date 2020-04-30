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
#define UNIT_TRANSFORM 100000

struct world {
public:
	friend class ScRobot;
	world() : x(0), y(0), z(0), c(0) {}
	world(int x, int y, int z, int c) { set(x, y, z, c); }
	world(float x, float y, float z, float c) { set(x, y, z, c); }

	void set(int x, int y, int z, int c)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->c = c;
	}
	void set(float x, float y, float z, float c)
	{
		this->x = (int)(x * UNIT_TRANSFORM);
		this->y = (int)(y * UNIT_TRANSFORM);
		this->z = (int)(z * UNIT_TRANSFORM);
		this->c = (int)(c * UNIT_TRANSFORM);
	}
	void setX(float x) { this->x = (int)(x * UNIT_TRANSFORM); }
	void setY(float y) { this->y = (int)(y * UNIT_TRANSFORM); }
	void setZ(float z) { this->z = (int)(z * UNIT_TRANSFORM); }
	void setC(float c) { this->c = (int)(c * UNIT_TRANSFORM); }

	float X() { return (float)x / UNIT_TRANSFORM; }
	float Y() { return (float)y / UNIT_TRANSFORM; }
	float Z() { return (float)z / UNIT_TRANSFORM; }
	float C() { return (float)c / UNIT_TRANSFORM; }

	world& operator=(const world& ref)
	{
		this->x = ref.x;
		this->y = ref.y;
		this->z = ref.z;
		this->c = ref.c;

		return *this;
	}

private:
	int x;
	int y;
	int z;
	int c;
};

class ScRobot
{
public:
    ScRobot();
    ~ScRobot();

    bool Connect();
    short GetSessionIdx() { return sessionIdx; }

    bool RefreshWorldLocation(world& ref);
    bool Move(world& ref);
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

std::ostream& operator<<(std::ostream& out, world& ref);

#endif