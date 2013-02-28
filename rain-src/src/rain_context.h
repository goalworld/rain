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
struct rain_ctx * rain_ctx_new(rain_routine_t prid,const char * mod,const char *args);
const char * rain_ctx_mod_name(struct rain_ctx *ctx);
rain_routine_t	 rain_ctx_get_id(struct rain_ctx *ctx);
rain_routine_t	 rain_ctx_get_pid(struct rain_ctx *ctx);
int rain_ctx_add_link(struct rain_ctx *ctx,rain_routine_t rid);
rain_session_t rain_ctx_genter_session(struct rain_ctx *ctx);
int rain_ctx_run(struct rain_ctx *ctx);
int rain_ctx_push_message(struct rain_ctx *ctx,struct rain_ctx_message msg);
void rain_ctx_ref(struct rain_ctx *ctx);
void rain_ctx_unref(struct rain_ctx *ctx);

struct rain_ctx * rain_handle_query_ctx(rain_routine_t rid,bool blog);
int rain_handle_push_message(rain_routine_t dest, struct rain_ctx_message msg);
int rain_handle_get_localid(rain_routine_t rid);
int rain_handle_kill(rain_routine_t rid,int code);

#endif /* RAIN_CTX_H_ */
