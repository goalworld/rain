/*
 * rain_type.h
 *
 *  Created on: 2013-1-16
 *      Author: wd
 */

#ifndef RAIN_TYPE_H_
#define RAIN_TYPE_H_
#include <stdint.h>
typedef int32_t rainRoutine;
typedef int32_t rainSession;
enum
{
	RAIN_OK,
	RAIN_ERROR
};
//以下三个函数千万不能调用阻塞函数。
struct rainContext;
typedef void *(*rainNewFn)(struct rainContext*ctx,const char *args);
typedef void(*rainDeleteFn)(void *env,int code);


struct rainMsg
{
	void *data;
	int sz;
	short type;
};

typedef void(*rainRecvFn)(void *env,rainRoutine src,struct rainMsg msg,rainSession session);
typedef void (*rainLinkFn)(void *env,rainRoutine exitid,int code);
typedef void(*rainTiemoutFn)(void *env,void* user_data);
#endif /* RAIN_TYPE_H_ */
