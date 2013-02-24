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
static void _recv(void *arg,rainRoutine src,struct rainMsg msg,rainSession session);
static void _recv_rps(void *arg,rainRoutine src,struct rainMsg msg,rainSession session);
static void _next_tick(void *env,void *user_data);
static void _link_exit(void *env,rainRoutine exitid,int code);
static void _svr_next_tick(tcpsvr_t * svr);
//static void _timercb(struct ev_loop * loop, ev_timer *w, int revents);
void
tcpsvrDelete(void *env,int code)
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
	wod_event_main_delete(svr->loop);
	free(svr);
}
void *
tcpsvrNew(struct rainContext*ctx,char *args)
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
	rainRoutine rids;
	char *tmp = strdup(args);
	char * token;
	int flag = WV_POLL_SELECT;
	char modbuf[32];
	int headsz = 0;
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
		}else if(strcmp(parm,"headsz") == 0){
			headsz = strtol(parm2,NULL,10);
		}
	}
	free(tmp);
	if(headsz != 2 && headsz != 4){
		free(svr);
		return NULL;
	}
	svr->watchdog = rids;
	svr->headsz = headsz;
	svr->loop = wod_event_main_new(10240,flag);
	if(!svr->loop){
		free(svr);
		return NULL;
	}
	int ret = tcpsvr_listen(svr,host,port);
	if(ret == RAIN_ERROR){
		wod_event_main_delete(svr->loop);
		free(svr);
		return NULL;
	}
	RAIN_CALLBACK(ctx,_recv,_recv_rps,_link_exit,NULL,_next_tick);
	svr->pre_loop_time = wod_time_usecond();
	rainNextTick(ctx,_svr_next_tick);
	rainLink(ctx,svr->watchdog);
	//rain_debug(svr->ctx,"<TCP-SERVER>: At(%s:%d),watcher:%d,mode:%s",host,port,rids,modbuf);
	return svr;
}
static void
_link_exit(void *env,rainRoutine exitid,int code)
{
	tcpsvr_t * svr = (tcpsvr_t *)env;
	if(exitid == svr->watchdog){
		rainExit(svr->ctx,0);
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
		rainNextTick(svr->ctx,_svr_next_tick);
	}
}

static void
_recv(void *env,rainRoutine src,struct rainMsg msg,rainSession session)
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
		rainResponce(svr->ctx,src,tmpmsg,RAIN_COPY,session);
	}
	free(msg.data);
}
static void
_recv_rps(void *env,rainRoutine src,struct rainMsg msg,rainSession session)
{
	free(msg.data);
}

