/*
 * tcpsvr_server.h
 *
 *  Created on: 2012-11-17
 *      Author: goalworld
 */
#include <sys/socket.h>
#include <wod_net.h>
#include "tcpsvr.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define BLOCK_MIN_TIME 50000

static inline int hash_func(int handle){
	return handle % TCPSVR_MAX_CONNECT;
}
static void
_doAccept(struct wodEvLoop *loop,void * nv,int mask);
static tcpclient_t * _new_client(tcpsvr_t *svr,int fd);
int
tcpsvr_run(tcpsvr_t* svr)
{
	wodEvRun(svr->loop);
	long long now = wodEvGetTime();
	long long dif_time = now - svr->pre_loop_time;
	if(dif_time < BLOCK_MIN_TIME){
		wodEvSleep(dif_time);
	}
	svr->pre_loop_time = now;
	return RAIN_OK;
}
int
tcpsvr_listen(tcpsvr_t* svr,const char *host,int port)
{
	svr->fd = wodNetTcpListen(TCP4,host,port);
	wodNetSetNonBlock(svr->fd,1);
	wodEvIOAdd(svr->loop,svr->fd,WV_IO_READ,_doAccept,svr);
	return RAIN_OK;
}
static void
_doAccept(struct wodEvLoop *loop,void * nv,int mask)
{
	tcpsvr_t *svr = (tcpsvr_t *)nv;
	for(;;){
		wodNetFd cfd = wodNetAccept(svr->fd);
		if(cfd < 0){
			if(-cfd == EAGAIN){
				break;
			}
			wodEvIORemove(svr->loop,svr->fd,WV_IO_READ);
			perror("Error:socket create fail");
			return;
		}
		tcpclient_t* cli = _new_client(svr,cfd);
		if(cli){
			tcpsvr_notifyconnect(svr,cli);
		}else{
			wodNetClose(cfd);
		}
	}
}
static tcpclient_t *
_new_client(tcpsvr_t *svr,wodNetFd fd)
{
	if(svr->num_cli == TCPSVR_MAX_CONNECT){
		return NULL;
	}

	int hash = -1;
	int i=0;
	for(i=0;i<TCPSVR_MAX_CONNECT;i++){
		hash = hash_func(svr->cut_index++);
		if(!svr->clients[hash].binuse){
			break;
		}
	}
	assert(hash != -1);
	tcpclient_t * cli = &svr->clients[hash];
	cli->svr = svr;
	if(0 != tcpclient_init(cli,fd,hash)){
		return NULL;
	}
	cli->binuse = true;
	wodNetSetNonBlock(fd,1);
	wodNetSetKeepAlive(fd,1);
	++svr->num_cli;
	return cli;
}
tcpclient_t *
tcpsvr_query(tcpsvr_t *svr,int id)
{
	int hash = hash_func(id);
	tcpclient_t * cli = &(svr->clients[hash]);
	if(cli->binuse && tcpclient_isactive(cli)){
		return cli;
	}
	return NULL;
}
enum
{
	CONNECT=0X01,
	MESSAGE,
	CLOSE,
	ERROR,
};
void
tcpsvr_notifyconnect(tcpsvr_t * svr,tcpclient_t * cli)
{
	struct rainMsg msg;
	msg.data = &cli->id;
	msg.sz = sizeof(cli->id);
	msg.type = CONNECT;
	rainSend(svr->ctx,svr->watchdog,msg,RAIN_COPY,NULL);
}
void
tcpsvr_notifyread(tcpsvr_t * svr,tcpclient_t * cli ,void *buf,int sz)
{
	svr->all_recv += sz-4;
	struct rainMsg msg;
	msg.data = buf;
	msg.sz = sz;
	msg.type = MESSAGE;
	rainSend(svr->ctx,svr->watchdog,msg,RAIN_NOCOPY,NULL);
}
void
tcpsvr_notifyclose(tcpsvr_t * svr,tcpclient_t * cli)
{
	struct rainMsg msg;
	msg.data = &cli->id;
	msg.sz = sizeof(cli->id);
	msg.type = CLOSE;
	rainSend(svr->ctx,svr->watchdog,msg,RAIN_COPY,NULL);
	tcpclient_destroy(cli);
}
void
tcpsvr_notifyerror(tcpsvr_t *svr,tcpclient_t *cli)
{
	svr->num_cli -- ;
	struct rainMsg msg;
	msg.data = &cli->id;
	msg.sz = sizeof(cli->id);
	msg.type = ERROR;
	rainSend(svr->ctx,svr->watchdog,msg,RAIN_COPY,NULL);
	svr->num_cli -- ;
	cli->binuse = false;
}
void
tcpsvr_notifyclosecp(tcpsvr_t *svr,tcpclient_t *cli)
{
	cli->binuse = false;
	svr->num_cli -- ;
}
