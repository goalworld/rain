/*
 * rain_msgqueue.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_MSGQUEUE_H_
#define RAIN_MSGQUEUE_H_
#include "rain.h"
#include "rain_msg.h"
struct rainMsgQueue;
int rainMsgQueueInit();
typedef void (*rainMsgDelFn)(struct rainCtxMsg *);
struct rainMsgQueue* rainMsgQueueNew();
void rainMsgQueueDelete(struct rainMsgQueue*,rainMsgDelFn delfn);
void rainMsgQueuePush(struct rainMsgQueue *,struct rainCtxMsg msg);
int  rainMsgQueuePop(struct rainMsgQueue *,struct rainCtxMsg *msg);
int  rainMsgQueueSize(struct rainMsgQueue *);
#endif /* RAIN_MSGQUEUE_H_ */
