/*
 * rain_lifequeue.c
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#include "rain_lifequeue.h"
#include "rain_mutex.h"
#include <stdlib.h>
#include <assert.h>
#include "rain_queue.h"
#ifdef PTHREAD_LOCK
#include <pthread.h>
#endif
#define VEC_SIZE  64
struct rainLifeQueue
{
	rain_queue_t r_queue;
	//
#ifdef PTHREAD_LOCK
	pthread_mutex_t mtx;
	pthread_cond_t con;
#else
	rainMutex mtx;
#endif
};
static struct rainLifeQueue * LQ = NULL;

int
rainLifeQueueInit()
{
	LQ = malloc(sizeof(struct rainLifeQueue));
#ifdef PTHREAD_LOCK
	pthread_mutex_init(&LQ->mtx,NULL);
	pthread_cond_init(&LQ->con,NULL);
#else
	rainMutexInit(&LQ->mtx);
#endif
	return rain_queue_init(&LQ->r_queue,sizeof(rainRoutine));
}
void
rainLifeQueuePush(rainRoutine rid)
{
	struct rainLifeQueue* lq = LQ;
#ifdef PTHREAD_LOCK
	pthread_mutex_lock(&lq->mtx);
#else
	rainMutextLock(&lq->mtx);
#endif
	rain_queue_push(&lq->r_queue,&rid);
#ifdef PTHREAD_LOCK
	pthread_cond_signal(&lq->con);
	pthread_mutex_unlock(&lq->mtx);
#else
	rainMutextUnLock(&lq->mtx);
#endif
}
int
rainLifeQueuePop(rainRoutine *rid)
{
	assert(rid);
	struct rainLifeQueue* lq = LQ;
#ifdef PTHREAD_LOCK
	pthread_mutex_lock(&lq->mtx);
#else
	rainMutextLock(&lq->mtx);
#endif
	if( rain_queue_pop(&lq->r_queue,rid) != 0){
#ifdef PTHREAD_LOCK
		pthread_cond_wait(&lq->con,&lq->mtx);
		pthread_mutex_unlock(&lq->mtx);
#else
		rainMutextUnLock(&lq->mtx);
#endif
		return RAIN_ERROR;
	}
#ifdef PTHREAD_LOCK
	pthread_mutex_unlock(&lq->mtx);
#else
	rain_mutex_unlock(&lq->mtx);
#endif
	return RAIN_OK;
}

