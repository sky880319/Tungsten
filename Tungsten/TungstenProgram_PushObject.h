#ifndef _TUNGSTENPROGRAM_PUSHOBJECT_H
#define _TUNGSTENPROGRAM_PUSHOBJECT_H
#pragma once

#include "..\Tungsten_Define.h"

// Customize Register Num
#define REG_X_AXIS       8503
#define REG_Y_AXIS       8504
#define REG_Z_AXIS       8505
#define REG_WHILE_STATUS 8506
#define REG_ALLOW_RUN    8507

#define NC_PUSH_PROC     528

class TgProg_PushObject : public TungstenProgram_Base
{
public:
	TgProg_PushObject() : m_progNum(0), m_sessionIdx(0) {  }
	TgProg_PushObject(int progNum, int sessionIdx) : m_progNum(progNum), m_sessionIdx(sessionIdx) {  }
	~TgProg_PushObject() { StopProc(); }

	virtual bool RunProc();
	virtual bool StopProc();
	virtual bool SetProc(const TgWorld& ref);
	virtual int  getProgNum() { return m_progNum; }

private:
	int m_progNum;
	int m_sessionIdx;
};

#endif