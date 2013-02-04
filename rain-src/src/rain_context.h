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
int rainContextInit(int rainid);
struct rainContext * rainContextNew(rainRoutine prid,const char * mod,const char *args);
const char * rainContextModName(struct rainContext *ctx);
rainRoutine	 rainContextGetId(struct rainContext *ctx);
rainRoutine	 rainContextGetPId(struct rainContext *ctx);
int rainContextAddLink(struct rainContext *ctx,rainRoutine rid);
rainSession rainContextSession(struct rainContext *ctx);
int rainContextRun(struct rainContext *ctx);
int rainContextPushMsg(struct rainContext *ctx,struct rainCtxMsg msg);
void rainContextRef(struct rainContext *ctx);
void rainContextUnRef(struct rainContext *ctx);

struct rainContext * rainHandleQuery(rainRoutine rid,bool blog);
int rainHandlePushMsg(rainRoutine dest, struct rainCtxMsg msg);
int rainHandleLocal(rainRoutine rid);
int rainHandleKill(rainRoutine rid,int code);

#endif /* RAIN_CTX_H_ */
