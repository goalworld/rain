/*
 * tcp_routine.c
 *
 *  Created on: 2012-11-10
 *      Author: goalworld
 */

#include <rain.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "tcpsvr.h"
#include <unistd.h>
enum{
	CMD_SEND=1,
	CMD_CLOSE=2,
};
typedef struct tcp_cmd_s{
	int id;
	int cmd;
}tcp_cmd_t;
static void _recv(void *arg,rain_routine_t src,struct rainMsg msg,rain_session_t session);
static void _recv_rps(void *arg,rain_routine_t src,struct rainMsg msg,rain_session_t session);
static void _next_tick(void *env,void *user_data);
static void _link_exit(void *env,rain_routine_t exitid,int code);
static void _svr_next_tick(tcpsvr_t * svr);
//static void _timercb(struct ev_loop * loop, ev_timer *w, int revents);
void
tcpsvr_delete(void *env,int code)
{
	if( !env ){
		return;
	}
	tcpsvr_t * svr = (tcpsvr_t *)env;
	int i=0;
	for(;i<TCPSVR_MAX_CONNECT;i++){
		if( svr->clients[i].binuse ){
			tcpclient_destroy(&svr->clients[i]);
		}
	}
	if(svr->fd >=0){
		close(svr->fd);
	}
	wod_event_destroy(svr->loop);
	free(svr);
}
void *
tcpsvr_new(struct rain_ctx*ctx,char *args)
{
	tcpsvr_t * svr = malloc(sizeof(tcpsvr_t));
	svr->fd = -1;
	svr->ctx = NULL;
	svr->args = NULL;
	int i=0;
	for(;i<TCPSVR_MAX_CONNECT;i++){
		svr->clients[i].binuse = false;
	}
	svr->ctx = ctx;
	char host[64];
	int port;
	rain_routine_t rids;
	char *tmp = strdup(args);
	char * token;
	int flag = WV_POLL_SELECT;
	char modbuf[32];
	// parser
	for(token = strsep(&tmp,"&");token!=NULL;token=strsep(&tmp,"&"))
	{
		char * parm,*parm2;
		parm = strsep(&token,"=");
		parm2 =parm+strlen(parm)+1;
		if(strcmp(parm,"ip") == 0){
			if(parm2){
				strncpy(host,parm2,64);
			}
		}else if(strcmp(parm,"port") == 0){
			port = strtol(parm2,NULL,10);
		}else if(strcmp(parm,"watchdog") == 0){
			rids = strtol(parm2,NULL,10);
		}else if(strcmp(parm,"mode") == 0){
			if(strcmp(parm2,"select") == 0){
				flag = WV_POLL_SELECT;
			}else if(strcmp(parm2,"epoll") == 0){
				flag = WV_POLL_EPOLL;
			}else if(strcmp(parm2,"poll") == 0){
				flag = WV_POLL_POLL;
			}
			strncpy(modbuf,parm2,sizeof(modbuf));
		}
	}
	free(tmp);
	svr->watchdog = rids;
	if( wod_event_create(&svr->loop,10240,flag) != 0){
		free(svr);
		return NULL;
	}
	int ret = tcpsvr_listen(svr,host,port);
	if(ret == RAIN_ERROR){
		wod_event_destroy(svr->loop);
		free(svr);
		return NULL;
	}
	RAIN_CALLBACK(ctx,_recv,_recv_rps,_link_exit,NULL,_next_tick);
	svr->pre_loop_time = wod_time_usecond();
	rain_next_tick(ctx,_svr_next_tick);
	rain_link(ctx,svr->watchdog);
	//rain_debug(svr->ctx,"<TCP-SERVER>: At(%s:%d),watcher:%d,mode:%s",host,port,rids,modbuf);
	return svr;
}
static void
_link_exit(void *env,rain_routine_t exitid,int code)
{
	tcpsvr_t * svr = (tcpsvr_t *)env;
	if(exitid == svr->watchdog){
		rain_exit(svr->ctx,0);
	}
}
static void
_next_tick(void *env,void *user_data)
{
	tcpsvr_t * svr = (tcpsvr_t *)env;
	//	printf("%p--%p \n",user_data,_svr_next_tick);
	if(user_data == _svr_next_tick){
		_svr_next_tick(svr);
	}
}
static void
_svr_next_tick(tcpsvr_t * svr)
{
	if( tcpsvr_run(svr) == RAIN_OK){
		rain_next_tick(svr->ctx,_svr_next_tick);
	}
}

static void
_recv(void *env,rain_routine_t src,struct rainMsg msg,rain_session_t session)
{
	tcpsvr_t * svr = (tcpsvr_t *)env;
	tcp_cmd_t * p = (tcp_cmd_t *)(msg.data);
	tcpclient_t *cli = tcpsvr_query(svr,p->id);
	if(cli){
		if(p->cmd == CMD_SEND){
			int ret = tcpclient_write(cli,NULL,0);
			if(ret < 0){
				printf("send:error");
			}
		}else if(p->cmd == CMD_CLOSE){
			tcpclient_destroy(cli);
		}
	}else if(session != RAIN_INVALID_SESSION){
		char buf[]="error_cmd";
		struct rainMsg tmpmsg={buf,sizeof(buf),-1};
		rain_responce(svr->ctx,src,tmpmsg,RAIN_COPY,session);
	}
	free(msg.data);
}
static void
_recv_rps(void *env,rain_routine_t src,struct rainMsg msg,rain_session_t session)
{
	free(msg.data);
}

