/*
 * rain_timer.c
 *
 *  Created on: 2012-11-17
 *      Author: goalworld
 */
#include "rain.h"
#include "rain_context.h"
#include "rain_timer.h"
#include <stdlib.h>
#include "rain_mutex.h"
#include "wod_time.h"
#define MIN_SLEEP 1000000
struct rainTimer
{
	rain_routine_t ctx_id;
	void* user_data;
	//rain_ctx_t * ctx;
	long long timeout;
	long long now;
	long long lefttime;
	struct rainTimer * next;
};
static void
_pending_times(struct rainTimer* timer)
{
	struct rain_ctx_message msg;
	msg.type = RAIN_MSG_TIMER;
	msg.u_data.time_data = timer->user_data;
	//msg.u_data.time_data = timer->ext_data;
	rain_handle_push_message(timer->ctx_id,msg);
	//rain_ctx_pushmsg(timer->ctx,msg);
}

struct rainTimerMgr
{
	struct rainTimer * head;
	struct rainTimer * runhead;
	long long min;
	rain_mutex mtx;
};

static struct  rainTimerMgr mgr;

int
rain_timer_init()
{
	mgr.head = NULL;
	rain_mutex_init(&mgr.mtx);
	mgr.runhead = NULL;
	mgr.min = MIN_SLEEP;
	return RAIN_OK;
}
static inline void
_test_swap(long long newTime)
{
	if(newTime < mgr.min){
		mgr.min = newTime;
	}
}
void
rain_timer_loop()
{
	for(;;){
		struct rainTimer* pre = NULL;
		struct rainTimer* tmp = mgr.runhead;
		while(tmp){
			long long now = wod_time_usecond();
			tmp->timeout -= (now-tmp->now);
			tmp->now = now;
			if(tmp->timeout <= 0.0){
				_pending_times(tmp);
				if(pre){
					pre->next = tmp->next;
				}else{
					mgr.runhead = tmp->next;
				}
				struct rainTimer *tf = tmp;
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
		wod_usleep(mgr.min);
	}
}
int
rain_timeout(struct rain_ctx *ctx,int timeout,void *user_data)
{
	if(!ctx || timeout<=0.0 ){
		return RAIN_ERROR;
	}
	struct rainTimer * p = malloc(sizeof(struct rainTimer));
	p->ctx_id = rain_routine_id(ctx);
	p->user_data = user_data;
	p->timeout = timeout*1000;
	p->now = wod_time_usecond();
	//p->ctx = ctx;
	rain_mutex_lock(&mgr.mtx);
	p->next = mgr.head;
	mgr.head = p;
	_test_swap(timeout);
	rain_mutex_unlock(&mgr.mtx);
	return RAIN_OK;
}

