#ifndef _TUNGSTEN_H
#define _TUNGSTEN_H
#pragma once

#include "Tungsten_Vision.h"
#include "Tungsten_Motion.h"

namespace ts
{
	ScRobot* g_scrbt;
	RsCamera* g_rscam;

	std::mutex g_mutex;
	std::condition_variable g_objQueue_cond;

	void Rs_StartProc();
	void ErrorMonter();
	void Sc_StartProc(ObjectQueue* objQueue);
	void ProcessObjectQueue(ObjectQueue* objQueue/*, const TgWorld& current*/);
	bool evalObjectTracking(TgObject& trace_obj/*, const TgWorld& current_location*/, TgWorld& except_location, int& delay);
}

#endif