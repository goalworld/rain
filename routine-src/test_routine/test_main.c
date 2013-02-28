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
	struct rain_ctx* ctx;
	long recvsize;
	int cli;
	rain_routine_t tcpsvr_id,jsv8_test_id;
}test_t;
static void _recv(void *arg,rain_routine_t src,struct rainMsg msg,rain_session_t session);
static void _recv_rsp(void *arg,rain_routine_t src,struct rainMsg msg,rain_session_t session);
static void _time_out(void *env,void *userdata);
static void _link_exit(void *env,rain_routine_t exitid,int code);

void *
test_new(struct rain_ctx *ctx,char *args)
{
	test_t * tt = malloc(sizeof(test_t));
	tt->ctx = ctx;
	tt->recvsize = 0;
	tt->cli = 0;
	char arg[1024];
	rain_debug(tt->ctx,"TestRunning,arguments:%s",args);
	fflush(stdout);
	int ret = 0;
	sprintf(arg,"ip=%s&port=%d&watchdog=%d&mode=%s&headsz=%d","127.0.0.1",8100,rain_routine_id(ctx),"epoll",4);
	ret = rain_spawn(ctx,"tcpsvr",arg,&(tt->tcpsvr_id));
	if(ret == RAIN_ERROR){
		free(tt);
		return NULL;
	}
	rain_link(ctx,tt->tcpsvr_id);
	RAIN_CALLBACK(ctx,_recv,_recv_rsp,_link_exit,_time_out,NULL);
	rain_timeout(ctx,60*1000,NULL);
	return tt;
}
void
test_delete(void *env,int code)
{
	if(!env){
		return;
	}
	test_t * tt = (test_t*)env;
	rain_kill(tt->ctx,tt->jsv8_test_id,0);
	free(env);
}
static void
_time_out(void *env,void *userdata)
{
	test_t * tt = (test_t*)env;
	rain_debug(tt->ctx,"_time_out");
	//rain_kill(tt->ctx,tt->tcpsvr_id,0);
	rain_kill(tt->ctx,tt->jsv8_test_id,0);
}
static void
_link_exit(void *env,rain_routine_t exitid,int code)
{
	test_t * tt = (test_t*)env;
	rain_debug(tt->ctx,"_link_exit,jsv8_test:%d,exitid:%d",tt->jsv8_test_id,exitid);
	rain_exit(tt->ctx,code);
}
static void
_recv(void *env,rain_routine_t src,struct rainMsg msg,rain_session_t session)
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
_recv_rsp(void *arg,rain_routine_t src,struct rainMsg msg,rain_session_t session)
{
	char buf[msg.sz+1];
	memcpy(buf,msg.data,msg.sz);
	buf[msg.sz] = 0x00;
	printf("WATCHDOG-RSP:%s,%d\n",buf,msg.sz);
	fflush(stdout);
	free(msg.data);
}

