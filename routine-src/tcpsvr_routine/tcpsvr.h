/*
 * tcpsvr_routine.h
 *
 *  Created on: 2012-11-10
 *      Author: goalworld
 */

#ifndef TCPSVR_ROUTINE_H_
#define TCPSVR_ROUTINE_H_
#include "wod_cyclebuffer.h"
#include <stdbool.h>
#include <rain.h>
#include <wod_event.h>
#include <wod_net.h>
#include <wod_time.h>
#define TCPSVR_MAX_CONNECT 4096
struct tcpsvr_s;
enum
{
	SOCK_OK=0X00,
	SOCK_WRITE_WAIT_CLOSED=0x01,
	SOCK_WRITE_CLOSED=0X2,
	SOCK_READ_CLOSED=0X4,
};
enum
{
	PARSE_HEAD,
	PARSE_BODY
};
typedef struct tcp_head_parser tcp_head_parser_t;
struct tcp_head_parser{
	struct wod_cycle_buffer * buf;
	unsigned headsz;
	unsigned state;
	unsigned bodysz;
};
typedef struct tcpclient_s
{
	struct tcpsvr_s *svr;
	wod_socket_t fd;
	int id;
	bool binuse;
	int sockstate;
	struct wod_cycle_buffer cycle_wrbuf;
	struct wod_cycle_buffer cycle_rebuf;
	tcp_head_parser_t parser;
}tcpclient_t;

typedef struct tcpsvr_s
{
	wod_socket_t fd;
	struct rain_ctx * ctx;
	char *args;
	tcpclient_t clients[TCPSVR_MAX_CONNECT];
	int num_cli;
	int cut_index;
	struct wod_event *loop;
	long long pre_loop_time;
	rain_routine_t watchdog;
	int headsz;
	long all_recv;
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

int tcpclient_init(tcpclient_t * cli,wod_socket_t fd,int id);
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
