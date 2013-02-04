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
rainSpawn(struct rainContext * ctx,const char * mod,const char *args,rainRoutine * rid)
{
	assert(ctx);
	if(!ctx){
		return RAIN_ERROR;
	}
	struct rainContext * new_ctx = rainContextNew(rainContextGetId(ctx),mod,args);
	if(new_ctx != NULL){
		if(rid){
			*rid = rainContextGetId(new_ctx);
		}
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
rainRoutine
rainRoutineId(struct rainContext *ctx)
{
	if(ctx == NULL){
		return  RAIN_INVALID_ID;
	}else{
		return rainContextGetId(ctx);
	}
}
rainRoutine
rainPRoutineId(struct rainContext *ctx)//获取id
{
	if(ctx == NULL){
		return  RAIN_INVALID_ID;
	}else{
		return rainContextGetPId(ctx);
	}
}
static inline int
_is_active_id(rainRoutine id)
{
	if(id == RAIN_INVALID_ID){
		return RAIN_ERROR;
	}
	return RAIN_OK;
}
static inline int
_send(rainRoutine dest,struct rainCtxMsg msg)
{
	if(rainHandleLocal(dest) == RAIN_OK){
		return rainHandlePushMsg(dest,msg);
	}else{
		//TODO RPC
		return RAIN_ERROR;
	}
}
int
rainSend(struct rainContext * ctx,rainRoutine dest,
		struct rainMsg msg,int bcopy,rainSession * se/*in out*/)
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
	struct rainCtxMsg rmsg;
	rmsg.u_data.msg = tmp_data;
	rmsg.u_sz.sz = msg.sz;
	rmsg.type = msg.type|RAIN_MSG_REQ;
	rmsg.src = rainContextGetId(ctx);
	if(se){
		*se = rainContextSession(ctx);
		rmsg.session = *se;
	}else{
		rmsg.session = RAIN_INVALID_SESSION;
	}
	return _send(dest,rmsg);
}
int
rainResponce(struct rainContext *ctx,rainRoutine dest, struct rainMsg msg,int bcopy,rainSession se)
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
	struct rainCtxMsg rmsg;
	rmsg.u_data.msg = tmp_data;
	rmsg.u_sz.sz = msg.sz;
	rmsg.type = msg.type|RAIN_MSG_RSP;
	rmsg.src = rainContextGetId(ctx);
	rmsg.session = se;
	return _send(dest,rmsg);
}

int
rainKill(struct rainContext *ctx,rainRoutine rid,int code)
{
	if(!ctx){
		return RAIN_ERROR;
	}else{
		if(rainContextGetId(ctx) == rid){
			return RAIN_ERROR;
		}
		if(rainHandleLocal(rid) == RAIN_OK){
			return rainHandleKill(rid,code);
		}else{
			//TODO
			return RAIN_ERROR;
		}
	}
}
int
rainLink(struct rainContext *ctx,rainRoutine rid)
{
	if(!ctx){
		return RAIN_ERROR;
	}
	if(rainHandleLocal(rid) == RAIN_OK ){
		if(rainContextGetId(ctx) == rid){
			return RAIN_ERROR;
		}
		struct rainContext *dest_ctx = rainHandleQuery(rid,true);
		if(dest_ctx){
			int ret = rainContextAddLink(dest_ctx,rainContextGetId(ctx));
			rainContextUnRef(dest_ctx);
			return ret;
		}else{
			return RAIN_ERROR;
		}
	}else{
		//TODO
		return RAIN_ERROR;
	}
}
