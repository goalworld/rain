/*
 * rain_timer.c
 *
 *  Created on: 2012-11-17
 *      Author: goalworld
 */
#include "rain.h"
#include "rain_ctx.h"
#include "rain_timer.h"
#include <stdlib.h>
#include "rain_mutex.h"
#include "rain_utils.h"
typedef struct rain_timer_s
{
	routine_t ctx_id;
	void* user_data;
	//rain_ctx_t * ctx;
	double timeout;
	double now;
	double lefttime;
	struct rain_timer_s * next;
}rain_timer_t;
static void
_pending_times(rain_timer_t* timer)
{
	rain_ctxmsg_t msg;
	msg.type = RAIN_MSG_TIMER;
	msg.u_data.time_data = timer->user_data;
	//msg.u_data.time_data = timer->ext_data;
	rain_ctx_handle_pushmsg(timer->ctx_id,msg);
	//rain_ctx_pushmsg(timer->ctx,msg);
}

typedef struct rain_timermgr_s
{
	rain_timer_t * head;
	rain_timer_t * runhead;
	double min;
	rain_mutex_t mtx;
}rain_timermgr_t;

static rain_timermgr_t mgr;

int
rain_timer_init()
{
	mgr.head = NULL;
	rain_mutex_init(&mgr.mtx);
	mgr.runhead = NULL;
	mgr.min = 1.0;
	return RAIN_OK;
}
static inline void
_test_swap(double newTime)
{
	if(newTime < mgr.min){
		mgr.min = newTime;
	}
}
void
rain_timer_loop()
{
	for(;;){
		rain_timer_t* pre = NULL;
		rain_timer_t* tmp = mgr.runhead;
		while(tmp){
			double now = rain_time();
			tmp->timeout -= (now-tmp->now);
			tmp->now = now;
			if(tmp->timeout <= 0.0){
				_pending_times(tmp);
				if(pre){
					pre->next = tmp->next;
				}else{
					mgr.runhead = tmp->next;
				}
				rain_timer_t *tf = tmp;
				tmp = tmp->next;
				free(tf);
				continue;
			}else{
				rain_mutex_lock(&mgr.mtx);
				_test_swap(tmp->timeout);
				rain_mutex_unlock(&mgr.mtx);
			}
			pre = tmp;
			tmp = tmp->next;
		}
		if(mgr.head){
			rain_mutex_lock(&mgr.mtx);
			if(pre){
				pre->next = mgr.head;
			}else{
				mgr.runhead = mgr.head;
			}
			mgr.head = NULL;
			rain_mutex_unlock(&mgr.mtx);
		}
		rain_sleep(mgr.min);
	}
}
int
rain_timeout(rain_ctx_t *ctx,double timeout,void *user_data)
{
	if(!ctx || timeout<=0.0 ){
		return RAIN_ERROR;
	}
	rain_timer_t * p = malloc(sizeof(rain_timer_t));
	p->ctx_id = rain_routineid(ctx);
	p->user_data = user_data;
	p->timeout = timeout;
	p->now = rain_time();
	//p->ctx = ctx;
	rain_mutex_lock(&mgr.mtx);
	p->next = mgr.head;
	mgr.head = p;
	_test_swap(timeout);
	rain_mutex_unlock(&mgr.mtx);
	return RAIN_OK;
}

