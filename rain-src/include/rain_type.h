/*
 * rain_type.h
 *
 *  Created on: 2013-1-16
 *      Author: wd
 */

#ifndef RAIN_TYPE_H_
#define RAIN_TYPE_H_
#include <stdint.h>
typedef int32_t routine_t;
typedef int32_t session_t;
typedef struct rain_ctx_s rain_ctx_t;
enum e_rain_ret
{
	RAIN_OK,
	RAIN_ERROR
};
//以下三个函数千万不能调用阻塞函数。
struct rain_ctx_s;
typedef void *(*rain_init_fn)(struct rain_ctx_s*ctx,const char *args);
typedef void(*rain_destroy_fn)(void *env,int code);


typedef struct rain_msg_s
{
	void *data;
	int sz;
	short type;
}rain_msg_t;

typedef void(*rain_recv_fn)(void *env,routine_t src,rain_msg_t msg,session_t session);
typedef void (*rain_link_fn)(void *env,routine_t exitid,int code);
typedef void(*rain_timerout_fn)(void *env,void* user_data);
typedef void(*rain_nexttick_fn)(void *env,void* user_data);
#endif /* RAIN_TYPE_H_ */
