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
static int rain_dipatch_routine(void);
static void * worker(void *arg);
static void * timer_loop(void *arg);
static void   sig_init(void);

int
main(int argc,char *argv[])
{
	assert(argc >=3);
	rain_log_init();
	rain_ctx_init(154);
	char *dir = malloc(1024);
	getcwd(dir,1024);
	strcat(dir,"/routine/");
	//printf("dir:%s %s %s %s\n",dir,argv[0],argv[1],argv[2]);
	rain_moudle_init(dir);
	free(dir);
	rain_timer_init();
	rainRpcInit();
	rain_life_queue_init();
	rain_message_queue_init();
	sig_init();
	struct rain_ctx * ctx = rain_ctx_new(0,argv[1],argv[2]);
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
	timer_loop(NULL);
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
sig_init(void)
{
	signal(SIGPIPE,SIG_IGN);
}
static int
rain_dipatch_routine(void)
{
	rain_routine_t rid;
	int ret = rain_life_queue_pop(&rid);
	if(ret == RAIN_OK){
		struct rain_ctx * ctx = rain_handle_query_ctx(rid,false);
		if(ctx){
			ret = rain_ctx_run(ctx);
			rain_ctx_unref(ctx);
			if(ret == RAIN_OK){
				rain_life_queue_push(rid);
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
		if(RAIN_ERROR == rain_dipatch_routine()){
			wod_usleep(100000);
		}
	}
	return (void *)(0);
}
static void *
timer_loop(void *arg)
{
	for(;;){
		rain_timer_loop();
		wod_usleep(100000);
	}
	return (void *)(0);
}
