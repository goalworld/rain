/*
 * tcpsvr_routine.h
 *
 *  Created on: 2012-11-10
 *      Author: goalworld
 */

#ifndef TCPSVR_ROUTINE_H_
#define TCPSVR_ROUTINE_H_
#include "cycle_buffer.h"
#include <stdbool.h>
#include <rain.h>
#include <ev.h>
#define TCPSVR_MAX_CONNECT 4096
struct tcpsvr_s;
enum
{
	SOCK_OK=0X00,
	SOCK_WRITE_WAIT_CLOSED=0x01,
	SOCK_WRITE_CLOSED=0X2,
	SOCK_READ_CLOSED=0X4,
};
typedef struct tcpclient_s
{
	struct tcpsvr_s *svr;
	ev_io ioread;
	ev_io iowrite;
	int fd;
	int id;
	bool binuse;
	int sockstate;
	cycle_buffer_t cycle_wrbuf;
	cycle_buffer_t cycle_rebuf;
}tcpclient_t;

typedef struct tcpsvr_s
{
	int fd;
	ev_io listen_ev;
	rain_ctx_t * ctx;
	char *args;
	tcpclient_t clients[TCPSVR_MAX_CONNECT];
	int num_cli;
	int cut_index;
	struct ev_loop * loop;
	double pre_loop_time;
	routine_t watchdog;
	int headsz;
	long all_recv;
	ev_timer timer;
	bool bInit;
}tcpsvr_t;
#define tcpsvr_getloop(svr) ((svr)->loop)
int tcpsvr_listen(tcpsvr_t* svr,const char *ip,int port);
int tcpsvr_run(tcpsvr_t* svr);
tcpclient_t * tcpsvr_query(tcpsvr_t *svr,int id);

void tcpsvr_notifyclosecp(tcpsvr_t *svr,tcpclient_t *cli);
void tcpsvr_notifyconnect(tcpsvr_t *svr,tcpclient_t * cli);
void tcpsvr_notifyread(tcpsvr_t *svr,tcpclient_t * cli ,void *buf,int sz);
void tcpsvr_notifyclose(tcpsvr_t *svr,tcpclient_t * cli);
void tcpsvr_notifyerror(tcpsvr_t *svr,tcpclient_t *cli);

int tcpclient_init(tcpclient_t * cli,int fd,int id);
/**
 * return>0:缓冲区满了
 * return=0:写到结尾。
 * return<0:出错了
 */

int tcpclient_write(tcpclient_t * cli,void *buf,int sz);
void tcpclient_close(tcpclient_t * cli);
void tcpclient_destroy(tcpclient_t * cli);
#define tcpclient_isactive( cli )( !((cli)->sockstate & SOCK_WRITE_WAIT_CLOSED) \
		||((cli)->sockstate & SOCK_WRITE_WAIT_CLOSED) )\
//void tcpsvr_send(int id,void *buf,int sz);
#endif /* TCPSVR_ROUTINE_H_ */
