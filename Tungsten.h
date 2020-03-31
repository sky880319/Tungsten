#ifndef _TUNGSTEN_H
#define _TUNGSTEN_H
#pragma once

#include "Tungsten_Vision.h"
#include "Tungsten_Motion.h"

namespace ts
{
	ScRobot* g_scrbt;
	RsCamera* g_rscam;
	void Rs_StartProc();
	void Sc_StartProc();
}

#endif