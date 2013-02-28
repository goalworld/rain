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
#include "wod_queue.h"
#ifdef PTHREAD_LOCK
#include <pthread.h>
#endif
#define VEC_SIZE  64
struct rainLifeQueue
{
	struct wod_queue r_queue;
	//
#ifdef PTHREAD_LOCK
	pthread_mutex_t mtx;
	pthread_cond_t con;
#else
	rain_mutex mtx;
#endif
};
static struct rainLifeQueue * LQ = NULL;

int
rain_life_queue_init()
{
	LQ = malloc(sizeof(struct rainLifeQueue));
#ifdef PTHREAD_LOCK
	pthread_mutex_init(&LQ->mtx,NULL);
	pthread_cond_init(&LQ->con,NULL);
#else
	rain_mutex_init(&LQ->mtx);
#endif
	return wod_queue_init(&LQ->r_queue,sizeof(rain_routine_t));
}
void
rain_life_queue_push(rain_routine_t rid)
{
	struct rainLifeQueue* lq = LQ;
#ifdef PTHREAD_LOCK
	pthread_mutex_lock(&lq->mtx);
#else
	rain_mutex_lock(&lq->mtx);
#endif
	wod_queue_push(&lq->r_queue,&rid);
#ifdef PTHREAD_LOCK
	pthread_cond_signal(&lq->con);
	pthread_mutex_unlock(&lq->mtx);
#else
	rain_mutex_unlock(&lq->mtx);
#endif
}
int
rain_life_queue_pop(rain_routine_t *rid)
{
	assert(rid);
	struct rainLifeQueue* lq = LQ;
#ifdef PTHREAD_LOCK
	pthread_mutex_lock(&lq->mtx);
#else
	rain_mutex_lock(&lq->mtx);
#endif
	if( wod_queue_pop(&lq->r_queue,rid) != 0){
#ifdef PTHREAD_LOCK
		pthread_cond_wait(&lq->con,&lq->mtx);
		pthread_mutex_unlock(&lq->mtx);
#else
		rain_mutex_unlock(&lq->mtx);
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

