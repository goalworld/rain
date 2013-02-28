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
struct rain_handle
{
	int rainid;
	rain_mutex mtx;
	int num_used;
	struct rain_ctx * ppctx[CTX_SET];
	int cut_index;
};
static  struct rain_handle * H = NULL;
struct rain_ctx
{
	struct rain_message_queue * msgQue;
	struct wod_array arr;
	rain_mutex mtx;
	struct rain_moudle * mod;
	rain_routine_t rid;//const
	rain_routine_t prid;
	void * arg;
	rain_recv_msg_fn recv;
	rain_recv_msg_fn recv_rsp;
	rain_link_fn link;
	rain_timeout_fn timeoutfn;
	rain_timeout_fn nexttickfn;
	bool bmain;
	int ref;
	int bdis;
	int binit;
	bool bexit;
	int exit_code;
	rain_session_t session;
};
static void _ctx_destroy( struct rain_ctx *ctx);
static void _ctx_genid(struct rain_ctx *ctx);

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
	H = malloc(sizeof(struct rain_handle));
	assert(H);
	struct rain_handle* h = H;
	memset(h->ppctx,0,sizeof(void *)*CTX_SET);
	h->num_used = 0;
	rain_mutex_init(&h->mtx);
	h->rainid =rainid;
	h->cut_index = 1;
	return 0;
}

static void
_ctx_genid(struct rain_ctx *ctx)
{
	assert(!IS_FULL(H));
	struct rain_handle* h = H;
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
struct rain_ctx *
rain_ctx_new(rain_routine_t prid, const char * mod_name,const char *args)
{
	if(IS_FULL(H)){
		return NULL;
	}
	struct rain_moudle *mod = rain_module_query(mod_name);
	if(!mod){
		RAIN_LOG(0,"MODE_QUERY：modname:%s",mod_name);
		return NULL;
	}
	//INIT
	struct rain_ctx *ctx = malloc(sizeof(struct rain_ctx));
	ctx->mod = mod;
	ctx->bdis = 0;
	ctx->recv = NULL;
	ctx->recv_rsp = NULL;
	ctx->link = NULL;
	ctx->session = 0;
	ctx->timeoutfn = NULL;
	ctx->nexttickfn = NULL;
	rain_mutex_init(&ctx->mtx);
	wod_array_init(&ctx->arr,sizeof(rain_routine_t));
	ctx->msgQue = rain_message_queue_new();
	ctx->bmain = false;
	_ctx_genid(ctx);
	ctx->ref = 1;
	ctx->bexit = 0;
	ctx->prid = prid;
	ctx->arg = rain_module_instance_init(mod,ctx,args);
	//EXEC;
	if(ctx->arg == NULL){
		RAIN_LOG(0,"RAIN_MAIN_FIALED：modname:%s args:%s",mod_name,args);
		rain_ctx_unref(ctx);
		return NULL;
	}
	__sync_bool_compare_and_swap(&ctx->bmain,false,true);
	if(rain_message_queue_size(ctx->msgQue) > 0){
		if(__sync_bool_compare_and_swap(&ctx->bdis,0,1)){
			rain_life_queue_push(ctx->rid);
		}
	}
	RAIN_LOG(0,"LAUNCH.ctx(%x.%s).arguments:%s",ctx->rid,mod_name,args);
	return ctx;
}
const char *
rain_ctx_mod_name(struct rain_ctx *ctx)
{
	assert(ctx);
	return rain_module_get_name(ctx->mod);
}
rain_routine_t
rain_ctx_get_id(struct rain_ctx *ctx)
{
	assert(ctx);
	return ctx->rid;
}
rain_routine_t
rain_ctx_get_pid(struct rain_ctx *ctx)
{
	assert(ctx);
	return ctx->prid;
}
int
rain_ctx_add_link(struct rain_ctx *ctx,rain_routine_t rid)
{
	assert(ctx);
	rain_mutex_lock(&ctx->mtx);
	int sz = wod_array_size(&ctx->arr);
	for(;sz>0;sz--){
		rain_routine_t tmpid;
		wod_array_at(&ctx->arr,sz-1,&tmpid);
		if(tmpid == rid){
			RAIN_LOG(0,"function<rain_ctx_addlink>:ctx(%d) Is already linked by ctx(%d).",ctx->rid,rid);
			rain_mutex_unlock(&ctx->mtx);
			return RAIN_ERROR;
		}
	}
	wod_array_push(&ctx->arr,&rid);
	rain_mutex_unlock(&ctx->mtx);
	return RAIN_OK;
}
rain_session_t
rain_ctx_genter_session(struct rain_ctx *ctx)
{
	return __sync_add_and_fetch(&ctx->session,1);
}
int
rain_ctx_run(struct rain_ctx *ctx)
{
	struct rain_ctx_message msg;
	int ret = rain_message_queue_pop(ctx->msgQue,&msg);
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
		if(rain_message_queue_size(ctx->msgQue) == 0){
			__sync_val_compare_and_swap(&ctx->bdis,1,0);
			return RAIN_ERROR;
		}
	}else{
		__sync_val_compare_and_swap(&ctx->bdis,1,0);
	}
	return ret;
}
static inline void
_time_to_life(struct rain_ctx *ctx)
{
	if(__sync_bool_compare_and_swap(&ctx->bmain,false,false)){
		return ;
	}
	if(__sync_bool_compare_and_swap(&ctx->bdis,0,1)){
		rain_life_queue_push(ctx->rid);
	}
}
int
rain_ctx_push_message(struct rain_ctx *ctx,struct rain_ctx_message msg)
{
	assert(ctx);
	if( __sync_bool_compare_and_swap(&ctx->bexit,false,false) ){
		rain_message_queue_push(ctx->msgQue,msg);
		_time_to_life(ctx);
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_next_tick(struct rain_ctx *ctx,void *user_data)
{
	assert(ctx);
	if( !ctx || !ctx->nexttickfn){
		return RAIN_ERROR;
	}
	struct rain_ctx_message msg;
	msg.u_data.tick_data = user_data;
	msg.type = RAIN_MSG_NEXTTICK;
	return rain_ctx_push_message(ctx,msg);
}
void
rain_ctx_ref(struct rain_ctx *ctx)
{
	__sync_add_and_fetch(&ctx->ref,1);
}

int
rain_handle_get_localid(rain_routine_t rid)
{
	if( RAIN_ID(rid) == H->rainid ){
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
struct rain_ctx *
rain_handle_query_ctx(rain_routine_t rid,bool blog)
{
	struct rain_handle* h = H;
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
	struct rain_ctx *ctx =  h->ppctx[hash];
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
rain_ctx_unref(struct rain_ctx *ctx)
{
	if(__sync_sub_and_fetch(&ctx->ref,1) == 0){
		int hash = hash_func(LOCAL_ID(ctx->rid));
		struct rain_handle* h = H;
		rain_mutex_lock(&h->mtx);
		_unregist_handle(hash);
		rain_mutex_unlock(&h->mtx);
		_ctx_destroy(ctx);
	}
}
static void
_del_msg(struct rain_ctx_message *rmsg)
{
	if((rmsg->type & RAIN_MSG_REQ) || (rmsg->type & RAIN_MSG_REQ)){
		free(rmsg->u_data.msg);
	}
}
static void
_ctx_destroy(struct rain_ctx *ctx)
{
	rain_module_init_destroy(ctx->mod,ctx->arg,ctx->exit_code);
	int size = wod_array_size(&ctx->arr);
	if(size == 0){
		wod_array_destroy(&ctx->arr);
	}else{
		struct rain_ctx_message rmsg;
		rain_routine_t rids[size];
		rmsg.src = ctx->rid;
		rmsg.type = RAIN_MSG_EXIT;
		rmsg.u_sz.exitcode = ctx->exit_code;
		wod_array_earse(&ctx->arr,0,size,rids);
		wod_array_destroy(&ctx->arr);
		int i=0;
		for(i=0; i<size; i++){
			rain_handle_push_message(rids[i],rmsg);
		}
	}
	rain_message_queue_delete(ctx->msgQue,_del_msg);
	RAIN_LOG(0,"EXIT.ctx(%x.%s)",ctx->rid,rain_module_get_name(ctx->mod));
	_time_exit();
}
int
rain_handle_kill(rain_routine_t rid,int code)
{
	struct rain_handle* h = H;
	int hash = hash_func(LOCAL_ID(rid));
	if( hash >= CTX_SET){
		return RAIN_ERROR;
	}
	rain_mutex_lock(&h->mtx);
	struct rain_ctx *ctx =  h->ppctx[hash];
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
rain_handle_push_message(rain_routine_t dest, struct rain_ctx_message msg)
{
	if(rain_handle_get_localid(dest) == RAIN_OK){
		struct rain_ctx * destctx = rain_handle_query_ctx(dest,true);
		if(destctx){
			int ret = rain_ctx_push_message(destctx,msg);
			rain_ctx_unref(destctx);
			return ret;
		}
		return RAIN_ERROR;
	}else{
		return RAIN_ERROR;
	}
}
int
rain_set_recvfn(struct rain_ctx *ctx,rain_recv_msg_fn req)
{
	if(!ctx->recv){
		ctx->recv = req;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_rspfn(struct rain_ctx *ctx, rain_recv_msg_fn rsp)
{
	if(!ctx->recv_rsp){
		ctx->recv_rsp = rsp;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_linkfn(struct rain_ctx *ctx,rain_link_fn linkfn)
{
	if(!ctx->link){
		ctx->link = linkfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_timeoutfn(struct rain_ctx *ctx,rain_timeout_fn timeoutfn)
{
	if(!ctx->timeoutfn){
		ctx->timeoutfn = timeoutfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_set_nexttickfn(struct rain_ctx *ctx,rain_timeout_fn next_tickfn)
{
	if(!ctx->nexttickfn){
		ctx->nexttickfn = next_tickfn;
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
int
rain_exit(struct rain_ctx *ctx,int code)
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

