/*
 * rain_loger.c
 *
 *  Created on: 2012-11-17
 *      Author: goalworld
 */

#include "rain_loger.h"
#include <stdarg.h>
#include <stdio.h>
#include "rain_context.h"
int
rainLogInit()
{
	return RAIN_OK;
}
void
rainLogError(const char *filename,int line,const char *fmt,...)
{
	char buf[8096];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buf,sizeof(buf),fmt,args);
	va_end(args);
	printf("[RAIN-SYS-LOG]\t(%s@%d)\r\n\t\t[$CODE]====>(%s)\n",filename,line,buf);
}
int
rainDebug(struct rainContext *ctx,const char *fmt,...)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	char buf[8096];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buf,sizeof(buf),fmt,args);
	va_end(args);
	printf("[RAIN-USR-DBG]\t[CTX(%x.%s)]\r\n\t\t[$CODE::%s]\n",rainContextGetId(ctx),rainContextModName(ctx),buf);
	return RAIN_OK;
}
