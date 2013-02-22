/*
 * rain_ctx.c
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */
#include "rain_context.h"
#include "wod_array.h"
#include "rain_lifequeue.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "rain_loger.h"
#include "rain_msgqueue.h"
#include "rain_mutex.h"
#include "rain_module.h"
#define CTX_SET 10240
static inline int
hash_func(int handle)
{
	return handle%CTX_SET;
}
#define RAIN_ID(rid) ((rid)>>16)
#define LOCAL_ID(rid) ((rid)&0x0000ffff)
#define CREATE_ID(rainid,localid)((rainid)<<16|((localid)&0x0000ffff))
#define IS_FULL(h) (((h)->num_used) == CTX_SET)
struct rainHandle
{
	int rainid;
	rainMutex mtx;
	int num_used;
	struct rainContext * ppctx[CTX_SET];
	int cut_index;
};
static  struct rainHandle * H = NULL;
struct rainContext
{
	struct rainMsgQueue * msgQue;
	struct wodArray arr;
	rainMutex mtx;
	struct rainModule * mod;
	rainRoutine rid;//const
	rainRoutine prid;
	void * arg;
	rainRecvFn recv;
	rainRecvFn recv_rsp;
	rainLinkFn link;
	rainTiemoutFn timeoutfn;
	rainTiemoutFn nexttickfn;
	bool bmain;
	int ref;
	int bdis;
	int binit;
	bool bexit;
	int exit_code;
	rainSession session;
};
static void _ctx_destroy( struct rainContext *ctx);
static void _ctx_genid(struct rainContext *ctx);

static inline void
_unregist_handle(hash)
{
	H->ppctx[hash] = NULL;
	--H->num_used;
}

static inline void
_time_exit()
{
	rainMutexLock(&H->mtx);
	if(H->num_used == 0){
		RAIN_LOG(0,"%s","all routine exit");
		exit(0);
	}
	rainMutexUnLock(&H->mtx);
}

int
rainContextInit(int rainid)
{
	H = malloc(sizeof(struct rainHandle));
	assert(H);
	struct rainHandle* h = H;
	memset(h->ppctx,0,sizeof(void *)*CTX_SET);
	h->num_used = 0;
	rainMutexInit(&h->mtx);
	h->rainid =rainid;
	h->cut_index = 1;
	return 0;
}

static void
_ctx_genid(struct rainContext *ctx)
{
	assert(!IS_FULL(H));
	struct rainHandle* h = H;
	rainMutexLock(&h->mtx);
	int hash;
	for(;;){
		int handle = h->cut_index++;
		hash = hash_func(handle);
		if(!h->ppctx[hash]){
			break;
		}
	}
	assert(hash != -1);
	h->ppctx[hash] = ctx;
	ctx->rid =CREATE_ID(h->rainid,hash);
	++h->num_used;
	rainMutexUnLock(&h->mtx);
}
struct rainContext *
rainContextNew(rainRoutine prid, const char * mod_name,const char *args)
{
	if(IS_FULL(H)){
		return NULL;
	}
	struct rainModule *mod = rainModuleQuery(mod_name);
	if(!mod){
		RAIN_LOG(0,"MODE_QUERY：modname:%s",mod_name);
		return NULL;
	}
	//INIT
	struct rainContext *ctx = malloc(sizeof(struct rainContext));
	ctx->mod = mod;
	ctx->bdis = 0;
	ctx->recv = NULL;
	ctx->recv_rsp = NULL;
	ctx->link = NULL;
	ctx->session = 0;
	ctx->timeoutfn = NULL;
	ctx->nexttickfn = NULL;
	rainMutexInit(&ctx->mtx);
	wodArrayInit(&ctx->arr,sizeof(rainRoutine));
	ctx->msgQue = rainMsgQueueNew();
	ctx->bmain = false;
	_ctx_genid(ctx);
	ctx->ref = 1;
	ctx->bexit = 0;
	ctx->prid = prid;
	ctx->arg = rainModuleInstNew(mod,ctx,args);
	//EXEC;
	if(ctx->arg == NULL){
		RAIN_LOG(0,"RAIN_MAIN_FIALED：modname:%s args:%s",mod_name,args);
		rainContextUnRef(ctx);
		return NULL;
	}
	__sync_bool_compare_and_swap(&ctx->bmain,false,true);
	if(rainMsgQueueSize(ctx->msgQue) > 0){
		if(__sync_bool_compare_and_swap(&ctx->bdis,0,1)){
			rainLifeQueuePush(ctx->rid);
		}
	}
	RAIN_LOG(0,"LAUNCH.ctx(%x.%s).arguments:%s",ctx->rid,mod_name,args);
	return ctx;
}
const char *
rainContextModName(struct rainContext *ctx)
{
	assert(ctx);
	return rainModuleName(ctx->mod);
}
rainRoutine
rainContextGetId(struct rainContext *ctx)
{
	assert(ctx);
	return ctx->rid;
}
rainRoutine
rainContextGetPId(struct rainContext *ctx)
{
	assert(ctx);
	return ctx->prid;
}
int
rainContextAddLink(struct rainContext *ctx,rainRoutine rid)
{
	assert(ctx);
	rainMutexLock(&ctx->mtx);
	int sz = wodArraySize(&ctx->arr);
	for(;sz>0;sz--){
		rainRoutine tmpid;
		wodArrayAt(&ctx->arr,sz-1,&tmpid);
		if(tmpid == rid){
			RAIN_LOG(0,"function<rain_ctx_addlink>:ctx(%d) Is already linked by ctx(%d).",ctx->rid,rid);
			rainMutexUnLock(&ctx->mtx);
			return RAIN_ERROR;
		}
	}
	wodArrayPush(&ctx->arr,&rid);
	rainMutexUnLock(&ctx->mtx);
	return RAIN_OK;
}
rainSession
rainContextSession(struct rainContext *ctx)
{
	return __sync_add_and_fetch(&ctx->session,1);
}
int
rainContextRun(struct rainContext *ctx)
{
	struct rainCtxMsg msg;
	int ret = rainMsgQueuePop(ctx->msgQue,&msg);
	if(ret == 0){
		if(msg.type & RAIN_MSG_REQ){
			if(ctx->recv){
				struct rainMsg tmpmsg;
				tmpmsg.data = msg.u_data.msg;
				tmpmsg.sz = msg.u_sz.sz;
				tmpmsg.type = msg.type & 0x0000ffff;
				ctx->recv(ctx->arg,msg.src,tmpmsg,msg.session);
			}else{
				RAIN_LOG(0,"Rid:%d,no register recv",ctx->rid);
				free(msg.u_data.msg);
			}
		}else if(msg.type & RAIN_MSG_RSP){
			if(ctx->recv_rsp){
				struct rainMsg tmpmsg;
				tmpmsg.data = msg.u_data.msg;
				tmpmsg.sz = msg.u_sz.sz;
				tmpmsg.type = msg.type & 0x0000ffff;
				ctx->recv_rsp(ctx->arg,msg.src,tmpmsg,msg.session);
			}else{
				RAIN_LOG(0,"Rid:%d,no register recv_responce",ctx->rid);
				free(msg.u_data.msg);
			}
		}else if(msg.type & RAIN_MSG_TIMER){
			if(ctx->timeoutfn){
				ctx->timeoutfn(ctx->arg,msg.u_data.time_data);
			}else{
				RAIN_LOG(0,"Rid:%d,no register timeout",ctx->rid);
			}
		}else if(msg.type & RAIN_MSG_NEXTTICK){
			if(ctx->nexttickfn){
				ctx->nexttickfn(ctx->arg,msg.u_data.tick_data);
			}else{
				RAIN_LOG(0,"Rid:%d,no register nexttick",ctx->rid);
			}
		}else if(msg.type & RAIN_MSG_EXIT){
			if(ctx->link){
				ctx->link(ctx->arg,msg.src,msg.u_sz.exitcode);
			}else{
				RAIN_LOG(0,"Rid:%d,no register link",ctx->rid);
			}
		}else{
			RAIN_LOG(0,"Rid:%d,Unkonw Message TYPE%x",ctx->rid,msg.type);
		}
		if(rainMsgQueueSize(ctx->msgQue) == 0){
			__sync_val_compare_and_swap(&ctx->bdis,1,0);
			return RAIN_ERROR;
		}
	}else{
		__sync_val_compare_and_swap(&ctx->bdis,1,0);
	}
	return ret;
}
static inline void
_time_to_life(struct rainContext *ctx)
{
	if(__sync_bool_compare_and_swap(&ctx->bmain,false,false)){
		return ;
	}
	if(__sync_bool_compare_and_swap(&ctx->bdis,0,1)){
		rainLifeQueuePush(ctx->rid);
	}
}
int
rainContextPushMsg(struct rainContext *ctx,struct rainCtxMsg msg)
{
	assert(ctx);
	if( __sync_bool_compare_and_swap(&ctx->bexit,false,false) ){
		rainMsgQueuePush(ctx->msgQue,msg);
		_time_to_life(ctx);
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rainNextTick(struct rainContext *ctx,void *user_data)
{
	assert(ctx);
	if( !ctx || !ctx->nexttickfn){
		return RAIN_ERROR;
	}
	struct rainCtxMsg msg;
	msg.u_data.tick_data = user_data;
	msg.type = RAIN_MSG_NEXTTICK;
	return rainContextPushMsg(ctx,msg);
}
void
rainContextRef(struct rainContext *ctx)
{
	__sync_add_and_fetch(&ctx->ref,1);
}

int
rainHandleLocal(rainRoutine rid)
{
	if( RAIN_ID(rid) == H->rainid ){
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
struct rainContext *
rainHandleQuery(rainRoutine rid,bool blog)
{
	struct rainHandle* h = H;
	if(h->rainid != RAIN_ID(rid)){
		if(blog){
			RAIN_LOG(0,"HANDLE_QUERY_FAILED：not local rid,rid:%x",rid);
		}
		return NULL;
	}
	int hash = hash_func(LOCAL_ID(rid));
	if( hash >= CTX_SET){
		if(blog){
			RAIN_LOG(0,"HANDLE_QUERY_FAILED:,invailed rid:%x",rid);
		}
		return NULL;
	}
	rainMutexLock(&h->mtx);
	struct rainContext *ctx =  h->ppctx[hash];
	if(!ctx || ctx->rid != rid){
		if(blog){
			RAIN_LOG(0,"HANDLE_QUERY_FAILED:,routine is not exist:%x",rid);
		}
		rainMutexUnLock(&h->mtx);
		return NULL;
	}
	__sync_add_and_fetch(&ctx->ref,1);
	rainMutexUnLock(&h->mtx);
	return ctx;
}
void
rainContextUnRef(struct rainContext *ctx)
{
	if(__sync_sub_and_fetch(&ctx->ref,1) == 0){
		int hash = hash_func(LOCAL_ID(ctx->rid));
		struct rainHandle* h = H;
		rainMutexLock(&h->mtx);
		_unregist_handle(hash);
		rainMutexUnLock(&h->mtx);
		_ctx_destroy(ctx);
	}
}
static void
_del_msg(struct rainCtxMsg *rmsg)
{
	if((rmsg->type & RAIN_MSG_REQ) || (rmsg->type & RAIN_MSG_REQ)){
		free(rmsg->u_data.msg);
	}
}
static void
_ctx_destroy(struct rainContext *ctx)
{
	rainModuleInstDel(ctx->mod,ctx->arg,ctx->exit_code);
	int size = wodArraySize(&ctx->arr);
	if(size == 0){
		wodArrayDestroy(&ctx->arr);
	}else{
		struct rainCtxMsg rmsg;
		rainRoutine rids[size];
		rmsg.src = ctx->rid;
		rmsg.type = RAIN_MSG_EXIT;
		rmsg.u_sz.exitcode = ctx->exit_code;
		wodArrayErase(&ctx->arr,0,size,rids);
		wodArrayDestroy(&ctx->arr);
		int i=0;
		for(i=0; i<size; i++){
			rainHandlePushMsg(rids[i],rmsg);
		}
	}
	rainMsgQueueDelete(ctx->msgQue,_del_msg);
	RAIN_LOG(0,"EXIT.ctx(%x.%s)",ctx->rid,rainModuleName(ctx->mod));
	_time_exit();
}
int
rainHandleKill(rainRoutine rid,int code)
{
	struct rainHandle* h = H;
	int hash = hash_func(LOCAL_ID(rid));
	if( hash >= CTX_SET){
		return RAIN_ERROR;
	}
	rainMutexLock(&h->mtx);
	struct rainContext *ctx =  h->ppctx[hash];
	if( !ctx ||  ctx->rid != rid ){
		RAIN_LOG(0,"rain_ctx_handle_kill:,invailed rid:%x",rid);
		rainMutexUnLock(&h->mtx);
		return RAIN_ERROR;
	}
	bool bexit = false;
	if(__sync_bool_compare_and_swap(&ctx->bexit,false,true)){
		ctx->exit_code = code;
		if(__sync_sub_and_fetch(&ctx->ref,1) == 0){
			bexit = true;
			_unregist_handle(hash);
		}
	}
	rainMutexUnLock(&h->mtx);
	if(bexit){
		_ctx_destroy(ctx);
	}
	return RAIN_OK;
}

int
rainHandlePushMsg(rainRoutine dest, struct rainCtxMsg msg)
{
	if(rainHandleLocal(dest) == RAIN_OK){
		struct rainContext * destctx = rainHandleQuery(dest,true);
		if(destctx){
			int ret = rainContextPushMsg(destctx,msg);
			rainContextUnRef(destctx);
			return ret;
		}
		return RAIN_ERROR;
	}else{
		return RAIN_ERROR;
	}
}
int
rainSetRecvfn(struct rainContext *ctx,rainRecvFn req)
{
	if(!ctx->recv){
		ctx->recv = req;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rainSetRspFn(struct rainContext *ctx, rainRecvFn rsp)
{
	if(!ctx->recv_rsp){
		ctx->recv_rsp = rsp;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rainSetLinkFn(struct rainContext *ctx,rainLinkFn linkfn)
{
	if(!ctx->link){
		ctx->link = linkfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rainSetTimeoutFn(struct rainContext *ctx,rainTiemoutFn timeoutfn)
{
	if(!ctx->timeoutfn){
		ctx->timeoutfn = timeoutfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rainSetNextTickFn(struct rainContext *ctx,rainTiemoutFn next_tickfn)
{
	if(!ctx->nexttickfn){
		ctx->nexttickfn = next_tickfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rainExit(struct rainContext *ctx,int code)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	if(__sync_bool_compare_and_swap(&ctx->bmain,true,true)){
		if(__sync_bool_compare_and_swap(&ctx->bexit,false,true)){
			ctx->exit_code = code;
			rainContextUnRef(ctx);
			return RAIN_OK;
		}
	}
	return RAIN_ERROR;
}

