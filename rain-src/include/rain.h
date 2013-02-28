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
enum
{
	RAIN_EV_NONE=0X00,
	RAIN_EV_IO_READ=0X01,
	RAIN_EV_IO_WRITE=0X02
};
enum
{
	RAIN_COPY,
	RAIN_NOCOPY
};

/**
 * 启动一个routine-process。
 * @mod routine-process程序文件（.so)。//libjsv8.so  mod:jsv8
 */
int rain_spawn(struct rain_ctx * ctx,const char * mod,const char *args,rain_routine_t * rid);
rain_routine_t rain_routine_id(struct rain_ctx *ctx);//获取id
rain_routine_t rain_routine_pid(struct rain_ctx *ctx);//获取 parent id
//bcopy rain_copy_e request 发送消息。回应消息
int rain_send(struct rain_ctx * ctx,rain_routine_t dest, struct rainMsg msg,int copy,rain_session_t * se/*in out*/);
int rain_responce(struct rain_ctx *ctx,rain_routine_t dest, struct rainMsg msg,int copy,rain_session_t se);
int rain_link(struct rain_ctx *ctx,rain_routine_t rid);//link rid的退出
int rain_next_tick(struct rain_ctx *ctx,void *user_data);//给自身发送一个消息-完成循环调用。
int rain_timeout(struct rain_ctx *ctx,int ms,void *user_data);

int rain_kill(struct rain_ctx *ctx,rain_routine_t rid,int code);//kill某个routine
int rain_regist_name(struct rain_ctx *ctx,const char *name);//注册名字unimp
int rain_query_name(struct rain_ctx *ctx,const char *name,rain_routine_t *out);//查询名字unimp
int rain_exit(struct rain_ctx *ctx,int code);//退出routine
int rain_debug(struct rain_ctx *ctx,const char *fmt,...);

//注册消息处理
#define RAIN_CALLBACK(ctx,recv,responcefn,linkfn,timeoutfn,next_tickfn)\
		do{	rain_set_recvfn((ctx),(recv));\
		rain_set_rspfn((ctx),(responcefn));\
		rain_set_linkfn((ctx),(linkfn));\
		rain_set_timeoutfn((ctx),(timeoutfn));\
		rain_set_nexttickfn((ctx),(next_tickfn));\
		}while(0)
//implement at rain_ctx
int rain_set_recvfn(struct rain_ctx *ctx,rain_recv_msg_fn req);
int rain_set_rspfn(struct rain_ctx *ctx, rain_recv_msg_fn rsp);
int rain_set_linkfn(struct rain_ctx *ctx,rain_link_fn linkfn);
int rain_set_timeoutfn(struct rain_ctx *ctx,rain_timeout_fn timeoutfn);
int rain_set_nexttickfn(struct rain_ctx *ctx,rain_timeout_fn next_tickfn);
#ifdef __cplusplus
}
#endif
#endif /* RAIN_H_ */
