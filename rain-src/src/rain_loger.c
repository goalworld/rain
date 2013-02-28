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
rain_log_init()
{
	return RAIN_OK;
}
void
rain_log(const char *filename,int line,const char *fmt,...)
{
	char buf[8096];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buf,sizeof(buf),fmt,args);
	va_end(args);
	printf("[RAIN-SYS-LOG]\t(%s@%d)\r\n\t\t[$CODE]====>(%s)\n",filename,line,buf);
}
int
rain_debug(struct rain_ctx *ctx,const char *fmt,...)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	char buf[8096];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buf,sizeof(buf),fmt,args);
	va_end(args);
	printf("[RAIN-USR-DBG]\t[CTX(%x.%s)]\r\n\t\t[$CODE::%s]\n",rain_ctx_get_id(ctx),rain_ctx_mod_name(ctx),buf);
	return RAIN_OK;
}
