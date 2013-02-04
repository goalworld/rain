/*
 * rain_utils.c
 *
 *  Created on: 2013-1-17
 *      Author: wd
 */

#include "rain_utils.h"
#include <sys/time.h>

void
rainSleep(double delay)
{
	struct timeval tv;
	tv.tv_sec  = (time_t)delay;
	tv.tv_usec = (long)((delay - (double)(tv.tv_sec)) * 1e6);
	select (0, 0, 0, 0, &tv);
}
 double
rainGetTime()
{
	struct timeval time;
	gettimeofday(&time,(void *)0);
	return time.tv_sec + time.tv_usec * 1E-6;
}
