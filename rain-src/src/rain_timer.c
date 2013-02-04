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
#include "rain_utils.h"
struct rainTimer
{
	rainRoutine ctx_id;
	void* user_data;
	//rain_ctx_t * ctx;
	double timeout;
	double now;
	double lefttime;
	struct rain_timer_s * next;
};
static void
_pending_times(struct rainTimer* timer)
{
	struct rainCtxMsg msg;
	msg.type = RAIN_MSG_TIMER;
	msg.u_data.time_data = timer->user_data;
	//msg.u_data.time_data = timer->ext_data;
	rainHandlePushMsg(timer->ctx_id,msg);
	//rain_ctx_pushmsg(timer->ctx,msg);
}

typedef struct rain_timermgr_s
{
	struct rainTimer * head;
	struct rainTimer * runhead;
	double min;
	rainMutex mtx;
}rain_timermgr_t;

static rain_timermgr_t mgr;

int
rainTimerInit()
{
	mgr.head = NULL;
	rainMutexInit(&mgr.mtx);
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
rainTimerLoop()
{
	for(;;){
		struct rainTimer* pre = NULL;
		struct rainTimer* tmp = mgr.runhead;
		while(tmp){
			double now = rainGetTime();
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
				rainMutexLock(&mgr.mtx);
				_test_swap(tmp->timeout);
				rainMutexUnLock(&mgr.mtx);
			}
			pre = tmp;
			tmp = tmp->next;
		}
		if(mgr.head){
			rainMutexLock(&mgr.mtx);
			if(pre){
				pre->next = mgr.head;
			}else{
				mgr.runhead = mgr.head;
			}
			mgr.head = NULL;
			rainMutexUnLock(&mgr.mtx);
		}
		rainSleep(mgr.min);
	}
}
int
rainTimeout(struct rainContext *ctx,double timeout,void *user_data)
{
	if(!ctx || timeout<=0.0 ){
		return RAIN_ERROR;
	}
	struct rainTimer * p = malloc(sizeof(struct rainTimer));
	p->ctx_id = rainRoutineId(ctx);
	p->user_data = user_data;
	p->timeout = timeout;
	p->now = rainGetTime();
	//p->ctx = ctx;
	rainMutexLock(&mgr.mtx);
	p->next = mgr.head;
	mgr.head = p;
	_test_swap(timeout);
	rainMutexUnLock(&mgr.mtx);
	return RAIN_OK;
}

