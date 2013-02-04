/*
 * test_main.c
 *
 *  Created on: 2012-11-12
 *      Author: goalworld
 */

#include <rain.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct test_s{
	struct rainContext* ctx;
	long recvsize;
	int cli;
	rainRoutine tcpsvr_id,jsv8_test_id;
}test_t;
static void _recv(void *arg,rainRoutine src,struct rainMsg msg,rainSession session);
static void _recv_rsp(void *arg,rainRoutine src,struct rainMsg msg,rainSession session);
static void _time_out(void *env,void *userdata);
static void _link_exit(void *env,rainRoutine exitid,int code);

void *
testNew(struct rainContext *ctx,char *args)
{
	test_t * tt = malloc(sizeof(test_t));
	tt->ctx = ctx;
	tt->recvsize = 0;
	tt->cli = 0;
	char arg[1024];
	rainDebug(tt->ctx,"TestRunning,arguments:%s",args);
	fflush(stdout);
	int ret = 0;
	sprintf(arg,"ip=%s&port=%d&watchdog=%d&mode=%sheadsz=%d","127.0.0.1",8100,rainRoutineId(ctx),"epoll",4);
	ret = rainSpawn(ctx,"tcpsvr",arg,&(tt->tcpsvr_id));
	if(ret == RAIN_ERROR){
		free(tt);
		return NULL;
	}
	rainLink(ctx,tt->tcpsvr_id);
	ret = rainSpawn(ctx,"jsv8","test.js",&(tt->jsv8_test_id));
	if(ret == RAIN_ERROR){
		free(tt);
		return NULL;
	}
	rainLink(ctx,tt->jsv8_test_id);
	RAIN_CALLBACK(ctx,_recv,_recv_rsp,_link_exit,_time_out,NULL);
	rainTimeout(ctx,60.0,NULL);
	return tt;
}
void
testDelete(void *env,int code)
{
	if(!env){
		return;
	}
	test_t * tt = (test_t*)env;
	rainKill(tt->ctx,tt->jsv8_test_id,0);
	free(env);
}
static void
_time_out(void *env,void *userdata)
{
	test_t * tt = (test_t*)env;
	rainDebug(tt->ctx,"_time_out");
	//rain_kill(tt->ctx,tt->tcpsvr_id,0);
	rainKill(tt->ctx,tt->jsv8_test_id,0);
}
static void
_link_exit(void *env,rainRoutine exitid,int code)
{
	test_t * tt = (test_t*)env;
	rainDebug(tt->ctx,"_link_exit,jsv8_test:%d,exitid:%d",tt->jsv8_test_id,exitid);
	rainExit(tt->ctx,code);
}
static void
_recv(void *env,rainRoutine src,struct rainMsg msg,rainSession session)
{
	test_t * tt = (test_t*)env;
	tt->recvsize +=msg.sz;
	//	char buf[msg.sz+1];
	//	memcpy(buf,msg.data,msg.sz);
	//	buf[msg.sz] = 0x00;
	//	printf("WATCHDOG-RCV:,%d,session:%d\n",msg.sz,session);
	//	fflush(stdout);
	if(msg.type == 1){
		tt->cli++;
	}
	free(msg.data);
}
static void
_recv_rsp(void *arg,rainRoutine src,struct rainMsg msg,rainSession session)
{
	char buf[msg.sz+1];
	memcpy(buf,msg.data,msg.sz);
	buf[msg.sz] = 0x00;
	printf("WATCHDOG-RSP:%s,%d\n",buf,msg.sz);
	fflush(stdout);
	free(msg.data);
}

