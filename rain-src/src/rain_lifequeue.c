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
typedef struct rain_lq_s
{
	rain_queue_t r_queue;
	//
#ifdef PTHREAD_LOCK
	pthread_mutex_t mtx;
	pthread_cond_t con;
#else
	int mtx;
#endif
}rain_lq_t;
static rain_lq_t * LQ = NULL;

int
rain_lifequeue_int()
{
	LQ = malloc(sizeof(rain_lq_t));
#ifdef PTHREAD_LOCK
	pthread_mutex_init(&LQ->mtx,NULL);
	pthread_cond_init(&LQ->con,NULL);
#else
	rain_mutex_init(&LQ->mtx);
#endif
	return rain_queue_init(&LQ->r_queue,sizeof(routine_t));
}
void
rain_lifequeue_push(routine_t rid)
{
	rain_lq_t* lq = LQ;
#ifdef PTHREAD_LOCK
	pthread_mutex_lock(&lq->mtx);
#else
	rain_mutex_lock(&lq->mtx);
#endif
	rain_queue_push(&lq->r_queue,&rid);
#ifdef PTHREAD_LOCK
	pthread_cond_signal(&lq->con);
	pthread_mutex_unlock(&lq->mtx);
#else
	rain_mutex_unlock(&lq->mtx);
#endif
}
int
rain_lifequeue_pop(routine_t *rid)
{
	assert(rid);
	rain_lq_t* lq = LQ;
#ifdef PTHREAD_LOCK
	pthread_mutex_lock(&lq->mtx);
#else
	rain_mutex_lock(&lq->mtx);
#endif
	if( rain_queue_pop(&lq->r_queue,rid) != 0){
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

