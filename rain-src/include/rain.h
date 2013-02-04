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
int rainSpawn(rainContext * ctx,const char * mod,const char *args,rainRoutine * rid);
rainRoutine rainRoutineId(rainContext *ctx);//获取id
rainRoutine rainPRoutineId(rainContext *ctx);//获取 parent id
//bcopy rain_copy_e request 发送消息。回应消息
int rainSend(rainContext * ctx,rainRoutine dest, struct rainMsg msg,int copy,rainSession * se/*in out*/);
int rainResponce(rainContext *ctx,rainRoutine dest, struct rainMsg msg,int copy,rainSession se);
int rainLink(rainContext *ctx,rainRoutine rid);//link rid的退出
int rainNextTick(rainContext *ctx,void *user_data);//给自身发送一个消息-完成循环调用。
int rainTimeout(rainContext *ctx,double timeout,void *user_data);

int rainKill(rainContext *ctx,rainRoutine rid,int code);//kill某个routine
int rainRegistName(rainContext *ctx,const char *name);//注册名字unimp
int rainQueryName(rainContext *ctx,const char *name,rainRoutine *out);//查询名字unimp
int rainExit(rainContext *ctx,int code);//退出routine
int rainDebug(rainContext *ctx,const char *fmt,...);

//注册消息处理
#define RAIN_CALLBACK(ctx,recv,responce,linkfn,timeoutfn,next_tickfn)\
		do{	rainSetRecvfn((ctx),(recv));\
		rainSetRspFn((ctx),(responce));\
		rainSetLinkFn((ctx),(linkfn));\
		rainSetTimeoutFn((ctx),(timeoutfn));\
		rainSetNextTickFn((ctx),(next_tickfn));\
		}while(0)
//implement at rain_ctx
int rainSetRecvfn(rainContext *ctx,rainRecvFn req);
int rainSetRspFn(rainContext *ctx, rainRecvFn rsp);
int rainSetLinkFn(rainContext *ctx,rainLinkFn linkfn);
int rainSetTimeoutFn(rainContext *ctx,rainTiemoutFn timeoutfn);
int rainSetNextTickFn(rainContext *ctx,rainTiemoutFn next_tickfn);
#ifdef __cplusplus
}
#endif
#endif /* RAIN_H_ */
