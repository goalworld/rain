/*
 * tcpsvr_server.h
 *
 *  Created on: 2012-11-17
 *      Author: goalworld
 */
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "tcpsvr.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define BLOCK_MIN_TIME (5E-2)

static int
_noblock(int fd)
{
	int ret = fcntl(fd,F_SETFL,O_NONBLOCK);
	if(ret){
		printf("fcntl O_NONBLOCK: %s", strerror(errno));
		return RAIN_ERROR;
	}
	return RAIN_OK;
}
static int
_keep_live(int fd)
{
	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
		printf("setsockopt SO_KEEPALIVE: %s", strerror(errno));
		return RAIN_ERROR;
	}
	return RAIN_OK;
}
static inline int hash_func(int handle){
	return handle % TCPSVR_MAX_CONNECT;
}
static void _do_accept(struct ev_loop * loop, ev_io *w, int revents);
static tcpclient_t * _new_client(tcpsvr_t *svr,int fd);
int
tcpsvr_run(tcpsvr_t* svr)
{
	ev_run(svr->loop,EVLOOP_NONBLOCK);
	double now = ev_time();
	double dif_time = now - svr->pre_loop_time;
	if(dif_time < BLOCK_MIN_TIME){
		//printf("tcpsvr_run:%f\n",dif_time);
		ev_sleep(dif_time);
	}
	svr->pre_loop_time = now;
	return RAIN_OK;
}
int
tcpsvr_listen(tcpsvr_t* svr,const char *host,int port)
{
	svr->fd = socket(AF_INET,SOCK_STREAM,0);
	if(svr->fd < 0){
		perror("Error:socket create fail");
		return RAIN_ERROR;
	}
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host);
	int ret = bind(svr->fd,(struct sockaddr *)(&addr),sizeof(addr));
	if(ret){
		perror("Error:bind create fail");
		close(svr->fd);
		svr->fd = -1;
		return RAIN_ERROR;
	}
	ret = listen(svr->fd,64);
	if(ret){
		perror("Error:listen create fail");
		close(svr->fd);
		svr->fd = -1;
		return RAIN_ERROR;
	}
	_noblock(svr->fd);
	ev_io_init(&svr->listen_ev,_do_accept,svr->fd,EV_READ);
	svr->listen_ev.data = svr;
	ev_io_start(svr->loop,&svr->listen_ev);
	return RAIN_OK;
}
static void
_do_accept(struct ev_loop * loop, ev_io *w, int revents)
{
	tcpsvr_t *svr = (tcpsvr_t *)w->data;
	for(;;){
		struct sockaddr_in caddr;
		socklen_t len = sizeof(caddr);
		int cfd = accept(svr->fd,(struct sockaddr *)(&caddr),&len);
		if(cfd < 0){
			if(errno == EAGAIN){
				break;
			}
			ev_io_stop(svr->loop,&svr->listen_ev);
			perror("Error:socket create fail");
			return;
		}
		tcpclient_t* cli = _new_client(svr,cfd);
		if(cli){
			tcpsvr_notifyconnect(svr,cli);
		}else{
			close(cfd);
		}
	}
}
static tcpclient_t *
_new_client(tcpsvr_t *svr,int fd)
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
	_noblock(fd);
	_keep_live(fd);
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
	rain_msg_t msg;
	msg.data = &cli->id;
	msg.sz = sizeof(cli->id);
	msg.type = CONNECT;
	rain_send(svr->ctx,svr->watchdog,msg,RAIN_COPY,NULL);
}
void
tcpsvr_notifyread(tcpsvr_t * svr,tcpclient_t * cli ,void *buf,int sz)
{
	svr->all_recv += sz-4;
	rain_msg_t msg;
	msg.data = buf;
	msg.sz = sz;
	msg.type = MESSAGE;
	rain_send(svr->ctx,svr->watchdog,msg,RAIN_NOCOPY,NULL);
}
void
tcpsvr_notifyclose(tcpsvr_t * svr,tcpclient_t * cli)
{
	rain_msg_t msg;
	msg.data = &cli->id;
	msg.sz = sizeof(cli->id);
	msg.type = CLOSE;
	rain_send(svr->ctx,svr->watchdog,msg,RAIN_COPY,NULL);
	tcpclient_destroy(cli);
}
void
tcpsvr_notifyerror(tcpsvr_t *svr,tcpclient_t *cli)
{
	svr->num_cli -- ;
	rain_msg_t msg;
	msg.data = &cli->id;
	msg.sz = sizeof(cli->id);
	msg.type = ERROR;
	rain_send(svr->ctx,svr->watchdog,msg,RAIN_COPY,NULL);
	svr->num_cli -- ;
	cli->binuse = false;
}
void
tcpsvr_notifyclosecp(tcpsvr_t *svr,tcpclient_t *cli)
{
	cli->binuse = false;
	svr->num_cli -- ;
}
