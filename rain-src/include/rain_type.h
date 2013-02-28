/*
 * rain_type.h
 *
 *  Created on: 2013-1-16
 *      Author: wd
 */

#ifndef RAIN_TYPE_H_
#define RAIN_TYPE_H_
#include <stdint.h>
typedef int32_t rain_routine_t;
typedef int32_t rain_session_t;
enum
{
	RAIN_OK,
	RAIN_ERROR
};
//以下三个函数千万不能调用阻塞函数。
struct rain_ctx;
typedef void *(*rain_new_fn)(struct rain_ctx*ctx,const char *args);
typedef void(*rain_del_fn)(void *env,int code);


struct rainMsg
{
	void *data;
	int sz;
	short type;
};

typedef void(*rain_recv_msg_fn)(void *env,rain_routine_t src,struct rainMsg msg,rain_session_t session);
typedef void (*rain_link_fn)(void *env,rain_routine_t exitid,int code);
typedef void(*rain_timeout_fn)(void *env,void* user_data);
#endif /* RAIN_TYPE_H_ */
