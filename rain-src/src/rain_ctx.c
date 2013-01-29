/*
 * rain_ctx.c
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */
#include "rain_ctx.h"
#include "rain_queue.h"
#include "rain_module.h"
#include "rain_mutex.h"
#include "rain_lifequeue.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "rain_array.h"
#include "rain_loger.h"

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
typedef struct rain_handle_s
{
	int rainid;
	rain_mutex_t mtx;
	int num_used;
	rain_ctx_t * ppctx[CTX_SET];
	int cut_index;
}rain_handle_t;
static  rain_handle_t * H = NULL;
struct rain_ctx_s
{
	rain_queue_t mq;
	rain_array_t arr;
	rain_mutex_t mtx;
	rain_module_t * mod;
	routine_t rid;//const
	routine_t prid;
	void * arg;
	rain_recv_fn recv;
	rain_recv_fn recv_rsp;
	rain_link_fn link;
	rain_timerout_fn timeoutfn;
	rain_nexttick_fn nexttickfn;
	bool bmain;
	int ref;
	int bdis;
	int binit;
	bool bexit;
	int exit_code;
	session_t session;
};
static void _ctx_destroy( rain_ctx_t *ctx);
static void _ctx_genid(rain_ctx_t *ctx);

static inline void
_unregist_handle(hash)
{
	H->ppctx[hash] = NULL;
	--H->num_used;
}

static inline void
_time_exit()
{
	rain_mutex_lock(&H->mtx);
	if(H->num_used == 0){
		RAIN_LOG(0,"%s","all routine exit");
		exit(0);
	}
	rain_mutex_unlock(&H->mtx);
}

int
rain_ctx_init(int rainid)
{
	H = malloc(sizeof(rain_handle_t));
	assert(H);
	rain_handle_t* h = H;
	memset(h->ppctx,0,sizeof(void *)*CTX_SET);
	h->num_used = 0;
	rain_mutex_init(&h->mtx);
	h->rainid =rainid;
	h->cut_index = 1;
	return 0;
}

static void
_ctx_genid(rain_ctx_t *ctx)
{
	assert(!IS_FULL(H));
	rain_handle_t* h = H;
	rain_mutex_lock(&h->mtx);
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
	rain_mutex_unlock(&h->mtx);
}
rain_ctx_t *
rain_ctx_new(routine_t prid, const char * mod_name,const char *args)
{
	if(IS_FULL(H)){
		return NULL;
	}
	rain_module_t *mod = rain_module_query(mod_name);
	if(!mod){
		RAIN_LOG(0,"MODE_QUERY：modname:%s",mod_name);
		return NULL;
	}
	//INIT
	rain_ctx_t *ctx = malloc(sizeof(rain_ctx_t));
	ctx->mod = mod;
	ctx->bdis = 0;
	ctx->recv = NULL;
	ctx->recv_rsp = NULL;
	ctx->link = NULL;
	ctx->session = 0;
	ctx->timeoutfn = NULL;
	ctx->nexttickfn = NULL;
	rain_mutex_init(&ctx->mtx);
	rain_array_init(&ctx->arr,sizeof(routine_t));
	rain_queue_init(&ctx->mq,sizeof(rain_ctxmsg_t));
	ctx->bmain = false;
	_ctx_genid(ctx);
	ctx->ref = 1;
	ctx->bexit = 0;
	ctx->prid = prid;
	ctx->arg = rain_module_inst_init(mod,ctx,args);
	//EXEC;
	if(ctx->arg == NULL){
		RAIN_LOG(0,"RAIN_MAIN_FIALED：modname:%s args:%s",mod_name,args);
		rain_ctx_unref(ctx);
		return NULL;
	}
	__sync_bool_compare_and_swap(&ctx->bmain,false,true);
	if(rain_queue_size(&ctx->mq) > 0){
		if(__sync_bool_compare_and_swap(&ctx->bdis,0,1)){
			rain_lifequeue_push(ctx->rid);
		}
	}
	RAIN_LOG(0,"LAUNCH.ctx(%x.%s).arguments:%s",ctx->rid,mod_name,args);
	return ctx;
}
const char *
rain_ctx_mod_name(rain_ctx_t *ctx)
{
	assert(ctx);
	return rain_module_name(ctx->mod);
}
routine_t
rain_ctx_getid(rain_ctx_t *ctx)
{
	assert(ctx);
	return ctx->rid;
}
routine_t
rain_ctx_getparentid(rain_ctx_t *ctx)
{
	assert(ctx);
	return ctx->prid;
}
int
rain_ctx_addlink(rain_ctx_t *ctx,routine_t rid)
{
	assert(ctx);
	rain_mutex_lock(&ctx->mtx);
	int sz = rain_array_size(&ctx->arr);
	for(;sz>0;sz--){
		routine_t tmpid;
		rain_array_at(&ctx->arr,sz-1,&tmpid);
		if(tmpid == rid){
			RAIN_LOG(0,"function<rain_ctx_addlink>:ctx(%d) Is already linked by ctx(%d).",ctx->rid,rid);
			rain_mutex_unlock(&ctx->mtx);
			return RAIN_ERROR;
		}
	}
	rain_array_pushback(&ctx->arr,&rid);
	rain_mutex_unlock(&ctx->mtx);
	return RAIN_OK;
}
session_t
rain_ctx_session(rain_ctx_t *ctx)
{
	return __sync_add_and_fetch(&ctx->session,1);
}
int
rain_ctx_run(rain_ctx_t *ctx)
{
	rain_ctxmsg_t msg;
	int ret = rain_queue_pop(&ctx->mq,&msg);
	if(ret == 0){
		if(msg.type & RAIN_MSG_REQ){
			if(ctx->recv){
				rain_msg_t tmpmsg;
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
				rain_msg_t tmpmsg;
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
		if(rain_queue_size(&ctx->mq) == 0){
			__sync_val_compare_and_swap(&ctx->bdis,1,0);
			return RAIN_ERROR;
		}
	}else{
		__sync_val_compare_and_swap(&ctx->bdis,1,0);
	}
	return ret;
}
static inline void
_time_to_life(rain_ctx_t *ctx)
{
	if(__sync_bool_compare_and_swap(&ctx->bmain,false,false)){
		return ;
	}
	if(__sync_bool_compare_and_swap(&ctx->bdis,0,1)){
		rain_lifequeue_push(ctx->rid);
	}
}
int
rain_ctx_pushmsg(rain_ctx_t *ctx,rain_ctxmsg_t msg)
{
	assert(ctx);
	if( __sync_bool_compare_and_swap(&ctx->bexit,false,false) ){
		rain_queue_push(&ctx->mq,&msg);
		_time_to_life(ctx);
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_next_tick(rain_ctx_t *ctx,void *user_data)
{
	assert(ctx);
	if( !ctx || !ctx->nexttickfn){
		return RAIN_ERROR;
	}
	rain_ctxmsg_t msg;
	msg.u_data.tick_data = user_data;
	msg.type = RAIN_MSG_NEXTTICK;
	return rain_ctx_pushmsg(ctx,msg);
}
void
rain_ctx_ref(rain_ctx_t *ctx)
{
	__sync_add_and_fetch(&ctx->ref,1);
}

int
rain_ctx_handle_local(routine_t rid)
{
	if( RAIN_ID(rid) == H->rainid ){
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
rain_ctx_t *
rain_ctx_handle_query(routine_t rid,bool blog)
{
	rain_handle_t* h = H;
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
	rain_mutex_lock(&h->mtx);
	rain_ctx_t *ctx =  h->ppctx[hash];
	if(!ctx || ctx->rid != rid){
		if(blog){
			RAIN_LOG(0,"HANDLE_QUERY_FAILED:,routine is not exist:%x",rid);
		}
		rain_mutex_unlock(&h->mtx);
		return NULL;
	}
	__sync_add_and_fetch(&ctx->ref,1);
	rain_mutex_unlock(&h->mtx);
	return ctx;
}
void
rain_ctx_unref(rain_ctx_t *ctx)
{
	if(__sync_sub_and_fetch(&ctx->ref,1) == 0){
		int hash = hash_func(LOCAL_ID(ctx->rid));
		rain_handle_t* h = H;
		rain_mutex_lock(&h->mtx);
		_unregist_handle(hash);
		rain_mutex_unlock(&h->mtx);
		_ctx_destroy(ctx);
	}
}
static void
_del_msg(void *data)
{
	rain_ctxmsg_t *rmsg = (rain_ctxmsg_t *)data;
	if((rmsg->type & RAIN_MSG_REQ) || (rmsg->type & RAIN_MSG_REQ)){
		free(rmsg->u_data.msg);
	}
}
static void
_ctx_destroy(rain_ctx_t *ctx)
{
	rain_module_inst_destroy(ctx->mod,ctx->arg,ctx->exit_code);
	int size = rain_array_size(&ctx->arr);
	if(size == 0){
		rain_array_destroy(&ctx->arr);
	}else{
		rain_ctxmsg_t rmsg;
		routine_t rids[size];
		rmsg.src = ctx->rid;
		rmsg.type = RAIN_MSG_EXIT;
		rmsg.u_sz.exitcode = ctx->exit_code;
		rain_array_erase(&ctx->arr,0,size,rids);
		rain_array_destroy(&ctx->arr);
		int i=0;
		for(i=0; i<size; i++){
			rain_ctx_handle_pushmsg(rids[i],rmsg);
		}
	}
	rain_queue_destroy(&ctx->mq,_del_msg);
	RAIN_LOG(0,"EXIT.ctx(%x.%s)",ctx->rid,rain_module_name(ctx->mod));
	_time_exit();
}
int
rain_ctx_handle_kill(routine_t rid,int code)
{
	rain_handle_t* h = H;
	int hash = hash_func(LOCAL_ID(rid));
	if( hash >= CTX_SET){
		return RAIN_ERROR;
	}
	rain_mutex_lock(&h->mtx);
	rain_ctx_t *ctx =  h->ppctx[hash];
	if( !ctx ||  ctx->rid != rid ){
		RAIN_LOG(0,"rain_ctx_handle_kill:,invailed rid:%x",rid);
		rain_mutex_unlock(&h->mtx);
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
	rain_mutex_unlock(&h->mtx);
	if(bexit){
		_ctx_destroy(ctx);
	}
	return RAIN_OK;
}

int
rain_ctx_handle_pushmsg(routine_t dest, rain_ctxmsg_t msg)
{
	if(rain_ctx_handle_local(dest) == RAIN_OK){
		rain_ctx_t * destctx = rain_ctx_handle_query(dest,true);
		if(destctx){
			int ret = rain_ctx_pushmsg(destctx,msg);
			rain_ctx_unref(destctx);
			return ret;
		}
		return RAIN_ERROR;
	}else{
		return RAIN_ERROR;
	}
}
int
rain_set_recvfn(rain_ctx_t *ctx,rain_recv_fn req)
{
	if(!ctx->recv){
		ctx->recv = req;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_recvrspfn(rain_ctx_t *ctx, rain_recv_fn rsp)
{
	if(!ctx->recv_rsp){
		ctx->recv_rsp = rsp;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_linkfn(rain_ctx_t *ctx,rain_link_fn linkfn)
{
	if(!ctx->link){
		ctx->link = linkfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_timeoutfn(rain_ctx_t *ctx,rain_timerout_fn timeoutfn)
{
	if(!ctx->timeoutfn){
		ctx->timeoutfn = timeoutfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_next_tickfun(rain_ctx_t *ctx,rain_timerout_fn next_tickfn)
{
	if(!ctx->nexttickfn){
		ctx->nexttickfn = next_tickfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_exit(rain_ctx_t *ctx,int code)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	if(__sync_bool_compare_and_swap(&ctx->bmain,true,true)){
		if(__sync_bool_compare_and_swap(&ctx->bexit,false,true)){
			ctx->exit_code = code;
			rain_ctx_unref(ctx);
			return RAIN_OK;
		}
	}
	return RAIN_ERROR;
}

