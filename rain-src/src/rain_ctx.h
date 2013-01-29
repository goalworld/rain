/*
 * rain_ctx.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_CTX_H_
#define RAIN_CTX_H_
#include <stdbool.h>
#include "rain_type.h"
#include "rain_msg.h"
int rain_ctx_init(int rainid);
rain_ctx_t * rain_ctx_new(routine_t prid,const char * mod,const char *args);
const char * rain_ctx_mod_name(rain_ctx_t *ctx);
routine_t	 rain_ctx_getid(rain_ctx_t *ctx);
routine_t	 rain_ctx_getparentid(rain_ctx_t *ctx);
int rain_ctx_addlink(rain_ctx_t *ctx,routine_t rid);
session_t rain_ctx_session(rain_ctx_t *ctx);
int rain_ctx_run(rain_ctx_t *ctx);
int rain_ctx_pushmsg(rain_ctx_t *ctx,rain_ctxmsg_t msg);
void rain_ctx_ref(rain_ctx_t *ctx);
void rain_ctx_unref(rain_ctx_t *ctx);

rain_ctx_t * rain_ctx_handle_query(routine_t rid,bool blog);
int rain_ctx_handle_pushmsg(routine_t dest, rain_ctxmsg_t msg);
int rain_ctx_handle_local(routine_t rid);
int rain_ctx_handle_kill(routine_t rid,int code);

#endif /* RAIN_CTX_H_ */
