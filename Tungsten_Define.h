#ifndef _TUNGSTEN_DEFINE_H
#define _TUNGSTEN_DEFINE_H
#pragma once

#include <ctime>
#include <chrono>
#include <queue>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <condition_variable>
#include <windows.h>

#define _PUSH_MODE_
#define UNIT_TRANSFORM    100000
//#define PEXEL2MM_FACTOR_X -0.552219354839f
//#define PEXEL2MM_FACTOR_Y -0.551948051948f
#define PEXEL2MM_FACTOR_X -0.4728690791f
#define PEXEL2MM_FACTOR_Y -0.4783414610f
#define VISION_COOR_RAD   -0.020271658648131453f
#define VISION_COOR_X			-448.f
#define VISION_COOR_Y           -3.04f
#define VISION_COOR_Z			 337.f
#define HOME_X               -143.800f
#define HOME_Y                392.758f
#define HOME_Z                 74.955f
#define OBJECT_SPEED           156.25f
#define ROBOT_SPEED       150.7494232f
#define WORKING_LIMIT_X1     -29780200
#define WORKING_LIMIT_Y1      25065400
#define WORKING_LIMIT_X2      31539700
#define WORKING_LIMIT_Y2      51085500
#define WORKING_LIMIT_Z1       9000000
#define WORKING_LIMIT_Z2      18500000

struct TgWorld;
struct TgPoint {
public:
	TgPoint() : x(0), y(0), z(0) {}
	TgPoint(int x, int y, int z) { set(x, y, z); }
	TgPoint(const TgPoint& p) { set(p); }

	void set(int x, int y, int z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	void set(const TgPoint& ref)
	{
		this->x = ref.x;
		this->y = ref.y;
		this->z = ref.z;
	}
	void setX(int x) { this->x = x; }
	void setY(int y) { this->y = y; }
	void setZ(int z) { this->z = z; }

	const int X() const { return x; }
	const int Y() const { return y; }
	const int Z() const { return z; }

	TgPoint& operator=(const TgPoint& ref)
	{
		set(ref);
		return *this;
	}

protected:
	int x;
	int y;
	int z;
};

struct TgWorld : TgPoint {
public:
	friend class ScRobot;
	friend class TungstenProgram_Base;
	TgWorld() : TgPoint(0, 0, 0), c(0) {}

	// Construct TgWorld by 4-axis absolute coordinate by integer.
	// [In] x, y, z, c: These values will NOT do unit transform.
	TgWorld(int x, int y, int z, int c) { set(x, y, z, c); }

	// Construct TgWorld by 4-axis absolute coordinate by float.
	// [In] x, y, z, c: These values will do unit transform (x100000) to integer.
	TgWorld(float x, float y, float z, float c) { set(x, y, z, c); }

	TgWorld(const TgWorld& p) { set(p); }

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
	void set(const TgWorld& ref)
	{
		this->x = ref.x;
		this->y = ref.y;
		this->z = ref.z;
		this->c = ref.c;
	}
	void setX(float x) { this->x = (int)(x * UNIT_TRANSFORM); }
	void setY(float y) { this->y = (int)(y * UNIT_TRANSFORM); }
	void setZ(float z) { this->z = (int)(z * UNIT_TRANSFORM); }
	void setC(float c) { this->c = (int)(c * UNIT_TRANSFORM); }

	const float X() const { return (float)x / UNIT_TRANSFORM; }
	const float Y() const { return (float)y / UNIT_TRANSFORM; }
	const float Z() const { return (float)z / UNIT_TRANSFORM; }
	const float C() const { return (float)c / UNIT_TRANSFORM; }

	const float getDistance(const TgWorld& ref) const
	{
		return (float)std::sqrt(std::pow(ref.x - this->x, 2) + std::pow(ref.y - this->y, 2) + std::pow(ref.z - this->z, 2)) / UNIT_TRANSFORM;
	}

	TgWorld& operator=(const TgWorld& ref)
	{
		set(ref);
		return *this;
	}

	bool operator==(const TgWorld& ref)
	{
		return (this->x / 10000 == ref.x / 10000 &&
				this->y / 10000 == ref.y / 10000 &&
				this->z / 10000 == ref.z / 10000 &&
				this->c / 10000 == ref.c / 10000);
	}
	bool operator!=(const TgWorld& ref)
	{
		return !(*this == ref);
	}

private:
	int c;
};

TgPoint parseVisionCoordinate(const cv::Point& img_point, const cv::Mat& img, float meter_depth);
TgWorld parseWorldCoordinate(TgPoint& vision_position);

enum TgObject_Type
{
	Suck = 1,
	Push = 2,
};
enum Side
{
	None,
	Left,
	Center,
	Right
};

typedef std::chrono::time_point<std::chrono::steady_clock> time_pt;
struct TgObject
{
	TgObject(unsigned long oid, TgObject_Type type, const TgWorld& center, const TgWorld& box_pt1, const TgWorld& box_pt2) : 
		oid(oid), 
		type(type),
		side(Side::None),
		time(std::chrono::steady_clock::now()), 
		vision_point(center),
		box_pt1(box_pt1),
		box_pt2(box_pt2) {  }

	TgObject(unsigned long oid, TgObject_Type type, Side side, time_pt time, const TgWorld& center, const TgWorld& box_pt1, const TgWorld& box_pt2) :
		oid(oid),
		type(type),
		side(side),
		time(time),
		vision_point(center),
		box_pt1(box_pt1),
		box_pt2(box_pt2) {  }

	template <class T>
	long long getDuration(time_pt now)
	{
		return std::chrono::duration_cast<T>(now - time).count();
	}

	TgWorld getExceptLocation(time_pt now)
	{
		TgWorld exp_location = vision_point;
		exp_location.setX(vision_point.X() + (float)getDuration<std::chrono::milliseconds>(now) * OBJECT_SPEED / 1000);
		return exp_location;
	}

	const unsigned long oid;
	const TgObject_Type type;
	const Side			side;
	const time_pt		time;
	TgWorld				vision_point;
	TgWorld				box_pt1;
	TgWorld				box_pt2;
};

typedef std::queue<TgObject*> ObjectQueue;

class TungstenProgram_Base
{
public:
	TungstenProgram_Base() : m_progNum(0), m_sessionIdx(0) {  }
	TungstenProgram_Base(int progNum, int sessionIdx) : m_progNum(progNum), m_sessionIdx(sessionIdx) {  }
	~TungstenProgram_Base() { StopProc(); }

	virtual bool RunProc() { return false; }
	virtual bool StopProc() { return false; }
	virtual bool SetProc(const TgWorld& ref) { return false; }
	virtual int  getProgNum() { return m_progNum; }

private:
	int m_progNum;
	int m_sessionIdx;
};

#endif