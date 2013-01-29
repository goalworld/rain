/*
 * rain_imp.c
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */
#include "rain.h"
#include "rain_msg.h"
#include "rain_ctx.h"
#include "rain_rpc.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
int
rain_spawn(rain_ctx_t * ctx,const char * mod,const char *args,routine_t * rid)
{
	assert(ctx);
	if(!ctx){
		return RAIN_ERROR;
	}
	rain_ctx_t * new_ctx = rain_ctx_new(rain_ctx_getid(ctx),mod,args);
	if(new_ctx != NULL){
		if(rid){
			*rid = rain_ctx_getid(new_ctx);
		}
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
routine_t
rain_routineid(rain_ctx_t *ctx)
{
	if(ctx == NULL){
		return  RAIN_INVALID_ID;
	}else{
		return rain_ctx_getid(ctx);
	}
}
routine_t
rain_parent_routineid(rain_ctx_t *ctx)//获取id
{
	if(ctx == NULL){
		return  RAIN_INVALID_ID;
	}else{
		return rain_ctx_getparentid(ctx);
	}
}
static inline int
_is_active_id(routine_t id)
{
	if(id == RAIN_INVALID_ID){
		return RAIN_ERROR;
	}
	return RAIN_OK;
}
static inline int
_send(routine_t dest,rain_ctxmsg_t msg)
{
	if(rain_ctx_handle_local(dest) == RAIN_OK){
		return rain_ctx_handle_pushmsg(dest,msg);
	}else{
		//TODO RPC
		return RAIN_ERROR;
	}
}
int
rain_send(rain_ctx_t * ctx,routine_t dest,
		rain_msg_t msg,int bcopy,session_t * se/*in out*/)
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
	rain_ctxmsg_t rmsg;
	rmsg.u_data.msg = tmp_data;
	rmsg.u_sz.sz = msg.sz;
	rmsg.type = msg.type|RAIN_MSG_REQ;
	rmsg.src = rain_ctx_getid(ctx);
	if(se){
		*se = rain_ctx_session(ctx);
		rmsg.session = *se;
	}else{
		rmsg.session = RAIN_INVALID_SESSION;
	}
	return _send(dest,rmsg);
}
int
rain_responce(rain_ctx_t *ctx,routine_t dest, rain_msg_t msg,int bcopy,session_t se)
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
	rain_ctxmsg_t rmsg;
	rmsg.u_data.msg = tmp_data;
	rmsg.u_sz.sz = msg.sz;
	rmsg.type = msg.type|RAIN_MSG_RSP;
	rmsg.src = rain_ctx_getid(ctx);
	rmsg.session = se;
	return _send(dest,rmsg);
}

int
rain_kill(rain_ctx_t *ctx,routine_t rid,int code)
{
	if(!ctx){
		return RAIN_ERROR;
	}else{
		if(rain_ctx_getid(ctx) == rid){
			return RAIN_ERROR;
		}
		if(rain_ctx_handle_local(rid) == RAIN_OK){
			return rain_ctx_handle_kill(rid,code);
		}else{
			//TODO
			return RAIN_ERROR;
		}
	}
}
int
rain_link(rain_ctx_t *ctx,routine_t rid)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	if(rain_ctx_handle_local(rid) == RAIN_OK ){
		if(rain_ctx_getid(ctx) == rid){
			return RAIN_ERROR;
		}
		rain_ctx_t *dest_ctx = rain_ctx_handle_query(rid,true);
		if(dest_ctx){
			int ret = rain_ctx_addlink(dest_ctx,rain_ctx_getid(ctx));
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
