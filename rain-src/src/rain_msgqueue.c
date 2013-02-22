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
	struct wodQueue r_queue;
	rainMutex mtx;
};

int rainMsgQueueInit(){
	return RAIN_OK;
}
struct rainMsgQueue*
rainMsgQueueNew()
{
	struct rainMsgQueue * mq =  malloc(sizeof(struct rainMsgQueue));
	wodQueueInit(&mq->r_queue,sizeof(struct rainCtxMsg));
	rainMutexInit(&mq->mtx);
	return mq;
}
void
rainMsgQueueDelete(struct rainMsgQueue*mq,rainMsgDelFn delfn)
{
	assert(mq);
	rainMutexLock(&mq->mtx);
	wodQueueDestroy(&mq->r_queue,(void(*)(void *))delfn);
	rainMutexUnLock(&mq->mtx);
	free(mq);
}
void
rainMsgQueuePush(struct rainMsgQueue *mq,struct rainCtxMsg msg)
{
	assert(mq);
	rainMutexLock(&mq->mtx);
	wodQueuePush(&mq->r_queue,&msg);
	rainMutexUnLock(&mq->mtx);
}
int
rainMsgQueuePop(struct rainMsgQueue *mq,struct rainCtxMsg *msg)
{
	assert(mq);
	rainMutexLock(&mq->mtx);
	int ret = wodQueuePop(&mq->r_queue,msg);
	rainMutexUnLock(&mq->mtx);
	return ret;
}
int
rainMsgQueueSize(struct rainMsgQueue * mq)
{
	rainMutexLock(&mq->mtx);
	int sz = wodQueueSize(&mq->r_queue);
	rainMutexUnLock(&mq->mtx);
	return sz;
}
