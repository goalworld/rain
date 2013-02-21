/*
 * rain_msgqueue.c
 *
 *  Created on: 2012-11-10
 *      Author: goalworld
 */
#include "rain_msgqueue.h"
#include "wod_queue.h"
#include "rain_mutex.h"
#include <stdlib.h>
#include <assert.h>
#define VEC_SIZE  64
struct rainMsgQueue
{
	struct wod_queue r_queue;
	rainMutex mtx;
};

int rainMsgQueueInit(){
	return RAIN_OK;
}
struct rainMsgQueue*
rainMsgQueueNew()
{
	struct rainMsgQueue * mq =  malloc(sizeof(struct rainMsgQueue));
	wod_queue_init(&mq->r_queue,sizeof(struct rainCtxMsg));
	rainMutexInit(&mq->mtx);
	return mq;
}
void
rainMsgQueueDelete(struct rainMsgQueue*mq,rainMsgDelFn delfn)
{
	assert(mq);
	rainMutexLock(&mq->mtx);
	wod_queue_destroy(&mq->r_queue,(void(*)(void *))delfn);
	rainMutexUnLock(&mq->mtx);
	free(mq);
}
void
rainMsgQueuePush(struct rainMsgQueue *mq,struct rainCtxMsg msg)
{
	assert(mq);
	rainMutexLock(&mq->mtx);
	wod_queue_push(&mq->r_queue,&msg);
	rainMutexUnLock(&mq->mtx);
}
int
rainMsgQueuePop(struct rainMsgQueue *mq,struct rainCtxMsg *msg)
{
	assert(mq);
	rainMutexLock(&mq->mtx);
	int ret = wod_queue_pop(&mq->r_queue,msg);
	rainMutexUnLock(&mq->mtx);
	return ret;
}
int
rainMsgQueueSize(struct rainMsgQueue * mq)
{
	rainMutexLock(&mq->mtx);
	int sz = wod_queue_size(&mq->r_queue);
	rainMutexUnLock(&mq->mtx);
	return sz;
}
