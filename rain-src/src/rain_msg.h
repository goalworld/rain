/*
 * rain_msg.h
 *
 *  Created on: 2013-1-16
 *      Author: wd
 */

#ifndef RAIN_MSG_H_
#define RAIN_MSG_H_
#include "rain_type.h"
enum{
	RAIN_MSG_REQ=		0x010000,
	RAIN_MSG_RSP=		0x020000,
	RAIN_MSG_TIMER=		0X040000,
	RAIN_MSG_NEXTTICK=	0X080000,
	RAIN_MSG_EXIT	=	0X100000,
};

struct rain_ctx_message{
	rain_routine_t src;
	union{
		void *msg;
		void *tick_data;
		void *time_data;
	}u_data;
	union{
		int sz;
		int fd;
		int timerid;
		int exitcode;
	}u_sz;
	int type;
	rain_session_t session;
};

#endif /* RAIN_MSG_H_ */
