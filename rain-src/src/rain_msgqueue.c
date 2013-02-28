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
struct rain_message_queue
{
	struct wod_queue r_queue;
	rain_mutex mtx;
};

int rain_message_queue_init(){
	return RAIN_OK;
}
struct rain_message_queue*
rain_message_queue_new()
{
	struct rain_message_queue * mq =  malloc(sizeof(struct rain_message_queue));
	wod_queue_init(&mq->r_queue,sizeof(struct rain_ctx_message));
	rain_mutex_init(&mq->mtx);
	return mq;
}
void
rain_message_queue_delete(struct rain_message_queue*mq,rain_message_del_cb delfn)
{
	assert(mq);
	rain_mutex_lock(&mq->mtx);
	wod_queue_destroy(&mq->r_queue,(void(*)(void *))delfn);
	rain_mutex_unlock(&mq->mtx);
	free(mq);
}
void
rain_message_queue_push(struct rain_message_queue *mq,struct rain_ctx_message msg)
{
	assert(mq);
	rain_mutex_lock(&mq->mtx);
	wod_queue_push(&mq->r_queue,&msg);
	rain_mutex_unlock(&mq->mtx);
}
int
rain_message_queue_pop(struct rain_message_queue *mq,struct rain_ctx_message *msg)
{
	assert(mq);
	rain_mutex_lock(&mq->mtx);
	int ret = wod_queue_pop(&mq->r_queue,msg);
	rain_mutex_unlock(&mq->mtx);
	return ret;
}
int
rain_message_queue_size(struct rain_message_queue * mq)
{
	rain_mutex_lock(&mq->mtx);
	int sz = wod_queue_size(&mq->r_queue);
	rain_mutex_unlock(&mq->mtx);
	return sz;
}
