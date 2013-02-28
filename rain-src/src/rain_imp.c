/*
 * rain_imp.c
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */
#include "rain.h"
#include "rain_msg.h"
#include "rain_context.h"
#include "rain_rpc.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
int
rain_spawn(struct rain_ctx * ctx,const char * mod,const char *args,rain_routine_t * rid)
{
	assert(ctx);
	if(!ctx){
		return RAIN_ERROR;
	}
	struct rain_ctx * new_ctx = rain_ctx_new(rain_ctx_get_id(ctx),mod,args);
	if(new_ctx != NULL){
		if(rid){
			*rid = rain_ctx_get_id(new_ctx);
		}
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
rain_routine_t
rain_routine_id(struct rain_ctx *ctx)
{
	if(ctx == NULL){
		return  RAIN_INVALID_ID;
	}else{
		return rain_ctx_get_id(ctx);
	}
}
rain_routine_t
rain_routine_pid(struct rain_ctx *ctx)//获取id
{
	if(ctx == NULL){
		return  RAIN_INVALID_ID;
	}else{
		return rain_ctx_get_pid(ctx);
	}
}
static inline int
_is_active_id(rain_routine_t id)
{
	if(id == RAIN_INVALID_ID){
		return RAIN_ERROR;
	}
	return RAIN_OK;
}
static inline int
_send(rain_routine_t dest,struct rain_ctx_message msg)
{
	if(rain_handle_get_localid(dest) == RAIN_OK){
		return rain_handle_push_message(dest,msg);
	}else{
		//TODO RPC
		return RAIN_ERROR;
	}
}
int
rain_send(struct rain_ctx * ctx,rain_routine_t dest,
		struct rainMsg msg,int bcopy,rain_session_t * se/*in out*/)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	if(_is_active_id(dest) == RAIN_ERROR){
		return RAIN_ERROR;
	}

	void *tmp_data;
	if((bcopy == RAIN_COPY) && msg.sz){
		tmp_data = malloc(msg.sz);
		if(!tmp_data){
			return RAIN_ERROR;
		}
		memcpy(tmp_data,msg.data,msg.sz);
	}else{
		tmp_data = msg.data;
	}
	struct rain_ctx_message rmsg;
	rmsg.u_data.msg = tmp_data;
	rmsg.u_sz.sz = msg.sz;
	rmsg.type = msg.type|RAIN_MSG_REQ;
	rmsg.src = rain_ctx_get_id(ctx);
	if(se){
		*se = rain_ctx_genter_session(ctx);
		rmsg.session = *se;
	}else{
		rmsg.session = RAIN_INVALID_SESSION;
	}
	return _send(dest,rmsg);
}
int
rain_responce(struct rain_ctx *ctx,rain_routine_t dest, struct rainMsg msg,int bcopy,rain_session_t se)
{
	if(!ctx || se == RAIN_INVALID_SESSION){
		return RAIN_ERROR;
	}
	if(_is_active_id(dest) == RAIN_ERROR){
		return RAIN_ERROR;
	}
	void *tmp_data;
	if((bcopy == RAIN_COPY) && msg.sz){
		tmp_data = malloc(msg.sz);
		if(!tmp_data){
			return RAIN_ERROR;
		}
		memcpy(tmp_data,msg.data,msg.sz);
	}else{
		tmp_data = msg.data;
	}
	struct rain_ctx_message rmsg;
	rmsg.u_data.msg = tmp_data;
	rmsg.u_sz.sz = msg.sz;
	rmsg.type = msg.type|RAIN_MSG_RSP;
	rmsg.src = rain_ctx_get_id(ctx);
	rmsg.session = se;
	return _send(dest,rmsg);
}

int
rain_kill(struct rain_ctx *ctx,rain_routine_t rid,int code)
{
	if(!ctx){
		return RAIN_ERROR;
	}else{
		if(rain_ctx_get_id(ctx) == rid){
			return RAIN_ERROR;
		}
		if(rain_handle_get_localid(rid) == RAIN_OK){
			return rain_handle_kill(rid,code);
		}else{
			//TODO
			return RAIN_ERROR;
		}
	}
}
int
rain_link(struct rain_ctx *ctx,rain_routine_t rid)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	if(rain_handle_get_localid(rid) == RAIN_OK ){
		if(rain_ctx_get_id(ctx) == rid){
			return RAIN_ERROR;
		}
		struct rain_ctx *dest_ctx = rain_handle_query_ctx(rid,true);
		if(dest_ctx){
			int ret = rain_ctx_add_link(dest_ctx,rain_ctx_get_id(ctx));
			rain_ctx_unref(dest_ctx);
			return ret;
		}else{
			return RAIN_ERROR;
		}
	}else{
		//TODO
		return RAIN_ERROR;
	}
}
