/*
 * rain_start.c
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */
#define __USE_GNU

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include "rain_context.h"
#include "rain_lifequeue.h"
#include "rain_timer.h"
#include "rain_module.h"
#include "rain_rpc.h"
#include "rain_loger.h"
#include "rain_msgqueue.h"
#include <wod_time.h>
#include <wod_sys.h>
static int rainDispatchRoutine(void);
static void * worker(void *arg);
static void * evloop(void *arg);
static void   _sigInit(void);

int
main(int argc,char *argv[])
{
	assert(argc >=3);
	rainLogInit();
	rainContextInit(154);
	char *dir = malloc(1024);
	getcwd(dir,1024);
	strcat(dir,"/routine/");
	//printf("dir:%s %s %s %s\n",dir,argv[0],argv[1],argv[2]);
	rainModuleInit(dir);
	free(dir);
	rainTimerInit();
	rainRpcInit();
	rainLifeQueueInit();
	rainMsgQueueInit();
	_sigInit();
	struct rainContext * ctx = rainContextNew(0,argv[1],argv[2]);
	if(ctx == NULL){
		exit(-1);
	}
	//routine_t rid  = rain_ctx_getid(ctx);
	int len = 4;
	pthread_t threads[len];
	int i;
	for(i=0; i<len; i++){
		pthread_create(&threads[i],NULL,worker,NULL);
	}
	evloop(NULL);
	/*
	pthread_t thread_ev;
	pthread_create(&thread_ev,NULL,evloop,NULL);

	for(i=0; i<len; i++){
		pthread_join(threads[i],NULL);
	}
	pthread_join(thread_ev,NULL);*/
	exit(0);
}
static void
_sigInit(void)
{
	signal(SIGPIPE,SIG_IGN);
}
static int
rainDispatchRoutine(void)
{
	rainRoutine rid;
	int ret = rainLifeQueuePop(&rid);
	if(ret == RAIN_OK){
		struct rainContext * ctx = rainHandleQuery(rid,false);
		if(ctx){
			ret = rainContextRun(ctx);
			rainContextUnRef(ctx);
			if(ret == RAIN_OK){
				rainLifeQueuePush(rid);
			}
		}else{
			RAIN_LOG(0,"UNKNOW CTX %x\n",rid);
		}
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
static void *
worker(void *arg)
{
	pthread_detach(pthread_self());
	for(;;){
		if(RAIN_ERROR == rainDispatchRoutine()){
			wod_sys_usleep(100000);
		}
	}
	return (void *)(0);
}
static void *
evloop(void *arg)
{
	for(;;){
		rainTimerLoop();
		wod_sys_usleep(100000);
	}
	return (void *)(0);
}
