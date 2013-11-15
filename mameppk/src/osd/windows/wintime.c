// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  wintime.c - Win32 OSD core timing functions
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

// MAME headers
#include "osdcore.h"
#ifdef KAILLERA
#include "ui_temp.h"
#endif /* KAILLERA */



//============================================================
//  GLOBAL VARIABLES
//============================================================

static osd_ticks_t ticks_per_second = 0;
static osd_ticks_t suspend_ticks = 0;
static BOOL using_qpc = TRUE;

#ifdef KAILLERA
#if 0
struct KCLOCK
{
	osd_ticks_t integer;
	unsigned int decimal;
};
#endif

static osd_ticks_t	adder = 0;
osd_ticks_t	KailleraWaitSubTick = 0;
osd_ticks_t	KailleraAdder = 0;
osd_ticks_t	KailleraWaitTick = 0;
osd_ticks_t	KailleraMaxWait = 0;
struct KCLOCK		Kailleraclock;

osd_ticks_t GetClockAdder(void)
{
	return adder;
}

void addClock(osd_ticks_t a)
{
	adder += a;
}
#endif /* KAILLERA */



//============================================================
//  osd_ticks
//============================================================

osd_ticks_t osd_ticks(void)
{
	LARGE_INTEGER performance_count;

	// if we're suspended, just return that
	if (suspend_ticks != 0)
		return suspend_ticks;

	// if we have a per second count, just go for it
	if (ticks_per_second != 0)
	{
		// QueryPerformanceCounter if we can
		if (using_qpc)
		{
			QueryPerformanceCounter(&performance_count);
#ifdef KAILLERA
			return (osd_ticks_t)performance_count.QuadPart - suspend_ticks - adder;
#else
			return (osd_ticks_t)performance_count.QuadPart - suspend_ticks;
#endif /* KAILLERA */
		}

		// otherwise, fall back to timeGetTime
		else
			return (osd_ticks_t)timeGetTime() - suspend_ticks;
	}

	// if not, we have to determine it
	using_qpc = QueryPerformanceFrequency(&performance_count) && (performance_count.QuadPart != 0);
	if (using_qpc)
		ticks_per_second = (osd_ticks_t)performance_count.QuadPart;
	else
		ticks_per_second = 1000;

#ifdef KAILLERA
	adder = 0;
	KailleraWaitSubTick = 0;
	KailleraAdder = 0;
	KailleraWaitTick = 0;
	KailleraMaxWait = (osd_ticks_t)ticks_per_second>>2;
#endif /* KAILLERA */

	// call ourselves to get the first value
	return osd_ticks();
}


//============================================================
//  osd_ticks_per_second
//============================================================

osd_ticks_t osd_ticks_per_second(void)
{
	if (ticks_per_second == 0)
		osd_ticks();
	return ticks_per_second;
}


//============================================================
//  osd_sleep
//============================================================

void osd_sleep(osd_ticks_t duration)
{
	DWORD msec;

	// make sure we've computed ticks_per_second
	if (ticks_per_second == 0)
		(void)osd_ticks();

	// convert to milliseconds, rounding down
	msec = (DWORD)(duration * 1000 / ticks_per_second);

	// only sleep if at least 2 full milliseconds
	if (msec >= 2)
	{
		HANDLE current_thread = GetCurrentThread();
		int old_priority = GetThreadPriority(current_thread);

		// take a couple of msecs off the top for good measure
		msec -= 2;

		// bump our thread priority super high so that we get
		// priority when we need it
		SetThreadPriority(current_thread, THREAD_PRIORITY_TIME_CRITICAL);
		Sleep(msec);
		SetThreadPriority(current_thread, old_priority);
	}
}
