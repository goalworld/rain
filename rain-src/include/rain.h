/*
 * rain.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_H_
#define RAIN_H_
#include <stdbool.h>
#include <stdint.h>
#include <rain_type.h>
#ifdef __cplusplus
extern "C"{
#endif
#define RAIN_INVALID_ID -1
#define RAIN_INVALID_SESSION -1
#define RAIN_VERSION "0.0"
enum rain_io_type
{
	RAIN_EV_NONE=0X00,
	RAIN_EV_IO_READ=0X01,
	RAIN_EV_IO_WRITE=0X02
};
enum rain_copy_e
{
	RAIN_COPY,
	RAIN_NOCOPY
};

/**
 * 启动一个routine-process。
 * @mod routine-process程序文件（.so)。//libjsv8.so  mod:jsv8
 */
int rain_spawn(rain_ctx_t * ctx,const char * mod,const char *args,routine_t * rid);
routine_t rain_routineid(rain_ctx_t *ctx);//获取id
routine_t rain_parent_routineid(rain_ctx_t *ctx);//获取 parent id
//bcopy rain_copy_e request 发送消息。回应消息
int rain_send(rain_ctx_t * ctx,routine_t dest, rain_msg_t msg,int copy,session_t * se/*in out*/);
int rain_responce(rain_ctx_t *ctx,routine_t dest, rain_msg_t msg,int copy,session_t se);
int rain_link(rain_ctx_t *ctx,routine_t rid);//link rid的退出
int rain_kill(rain_ctx_t *ctx,routine_t rid,int code);//kill某个routine
int rain_regist_name(rain_ctx_t *ctx,const char *name);//注册名字unimp
int rain_query_name(rain_ctx_t *ctx,const char *name,routine_t *out);//查询名字unimp
int rain_exit(rain_ctx_t *ctx,int code);//退出routine
int rain_debug(rain_ctx_t *ctx,const char *fmt,...);
int rain_next_tick(rain_ctx_t *ctx,void *user_data);//给自身发送一个消息-完成循环调用。
//---
int rain_timeout(rain_ctx_t *ctx,double timeout,void *user_data);
//注册消息处理
#define RAIN_CALLBACK(ctx,recv,responce,linkfn,timeoutfn,next_tickfn)\
		do{	rain_set_recvfn((ctx),(recv));\
			rain_set_recvrspfn((ctx),(responce));\
			rain_set_linkfn((ctx),(linkfn));\
			rain_set_timeoutfn((ctx),(timeoutfn));\
			rain_set_next_tickfun((ctx),(next_tickfn));\
			}while(0)
//implement at rain_ctx
int rain_set_recvfn(rain_ctx_t *ctx,rain_recv_fn req);
int rain_set_recvrspfn(rain_ctx_t *ctx, rain_recv_fn rsp);
int rain_set_linkfn(rain_ctx_t *ctx,rain_link_fn linkfn);
int rain_set_timeoutfn(rain_ctx_t *ctx,rain_timerout_fn timeoutfn);
int rain_set_next_tickfun(rain_ctx_t *ctx,rain_timerout_fn next_tickfn);
#ifdef __cplusplus
}
#endif
#endif /* RAIN_H_ */
