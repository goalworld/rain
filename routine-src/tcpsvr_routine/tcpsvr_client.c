/*
 * tcpsvr_client.c
 *
 *  Created on: 2012-11-11
 *      Author: goalworld
 */


#include "tcpsvr.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <memory.h>
#include <netinet/in.h>

#define TCPCLI_BUF_DEFAULT_SZ 1024
static void
_doRead(struct wod_event *loop,void * nv,int mask);
static void
_doWrite(struct wod_event *loop,void * nv,int mask);
static int real_read(tcpclient_t * cli);
static int real_write(tcpclient_t * cli,void *buf,unsigned sz);

int
tcpclient_init(tcpclient_t * cli,wod_socket_t fd,int id)
{
	if(cli->binuse){
		return RAIN_ERROR;
	}
	cli->id = id;
	cli->fd = fd;
	int ret = wod_cycle_buffer_init(&cli->cycle_wrbuf,TCPCLI_BUF_DEFAULT_SZ);
	if(ret !=0){
		return RAIN_ERROR;
	}
	ret = wod_cycle_buffer_init(&cli->cycle_rebuf,TCPCLI_BUF_DEFAULT_SZ);
	if(ret !=0){
		wod_cycle_buffer_destroy(&cli->cycle_wrbuf);
		return RAIN_ERROR;
	}
	cli->sockstate = SOCK_OK;
	wod_event_io_add(cli->svr->loop,fd,WV_IO_READ,_doRead,cli);
	//wodEvIOAdd(cli->svr->loop,fd,WV_IO_WRITE,_doWrite,cli);
	cli->parser.buf = &cli->cycle_rebuf;
	cli->parser.state = PARSE_HEAD;
	cli->parser.headsz = 4;
	cli->parser.bodysz = 0;
	return RAIN_OK;
}

static inline void
_release(tcpclient_t * cli)
{
	wod_net_close(cli->fd);
	wod_cycle_buffer_destroy(&cli->cycle_wrbuf);
	wod_cycle_buffer_destroy(&cli->cycle_rebuf);
}

static void inline
_do_error(tcpclient_t * cli)
{
	wod_event_io_remove(cli->svr->loop,cli->fd,WV_IO_READ|WV_IO_WRITE);
	tcpsvr_notifyerror(cli->svr,cli);
	_release(cli);
}
static inline void
_paser2(tcpclient_t * cli)
{
	tcp_head_parser_t * parser = &cli->parser;
	struct wod_cycle_pair pair;
	wod_cycle_buffer_get_used(parser->buf,&pair);
	for(;;){
		int allbacksz  = 0;
		if(parser->state == PARSE_HEAD){
			unsigned int a;
			if( wod_cycle_pair_readsz(&pair,&a,4) != 0){
				return ;
			}else{
				parser->state = PARSE_BODY;
				parser->bodysz = htonl(a);
				allbacksz += 4;
			}
		}
		if(parser->state == PARSE_BODY){
			if(wod_cycle_pair_sz(&pair) >= parser->bodysz){
				void *buf = malloc(parser->bodysz+4);
				wod_cycle_pair_readsz(&pair,(char *)buf+4,parser->bodysz);
				allbacksz += parser->bodysz;
				wod_cycle_buffer_pop(parser->buf,allbacksz);
				tcpsvr_notifyread(cli->svr,cli,buf,parser->bodysz+4);
			}else{
				wod_cycle_buffer_pop(parser->buf,allbacksz);
				return;
			}
		}
	}
}
static void
_doRead(struct wod_event *loop,void * nv,int mask)
{
	tcpclient_t * cli = (tcpclient_t *)nv;
	int ret = real_read(cli);
	if( wod_cycle_buffer_used_size(&cli->cycle_rebuf) > 0){
		_paser2(cli);
	}
	if(ret == 0){
		wod_event_io_remove(cli->svr->loop,cli->fd,WV_IO_READ);
		if(cli->sockstate & SOCK_WRITE_CLOSED){
			tcpsvr_notifyclosecp(cli->svr,cli);
			_release(cli);
		}
		tcpsvr_notifyclose(cli->svr,cli);
	}else if(ret < 0 ){
		_do_error(cli);
	}
}

static void
_doWrite(struct wod_event *loop,void * nv,int mask)
{
	tcpclient_t * cli = (tcpclient_t *)nv;
	tcpclient_write(cli,NULL,0);
}
int
tcpclient_write(tcpclient_t * cli,void *buf,unsigned sz)
{
	int send = real_write(cli,buf,sz);
	if(send == 0){
		wod_event_io_remove(cli->svr->loop,cli->fd,WV_IO_WRITE);
		if(cli->sockstate & SOCK_WRITE_WAIT_CLOSED){
			cli->sockstate |= SOCK_WRITE_CLOSED;
			shutdown(cli->fd,SHUT_WR);
		}
		if(cli->sockstate & SOCK_READ_CLOSED){
			tcpsvr_notifyclosecp(cli->svr,cli);
			tcpclient_destroy(cli);
		}
	}else if(send < 0  ){
		_do_error(cli);
	}else{
		wod_event_io_add(cli->svr->loop,cli->fd,WV_IO_WRITE,_doWrite,cli);
	}
	return send;
}

static int
real_write(tcpclient_t * cli,void *buf,unsigned sz)
{
	assert(sz <= 0xffff);
	struct wod_cycle_pair pair;
	int ret = wod_cycle_buffer_get_used(&cli->cycle_wrbuf,&pair);
	struct wod_socket_buf io[4];
	int num = 0;
	int allsz = 0;
	if(ret == 0){
		io[num].b_body = pair.first.buf;
		io[num].b_sz = pair.first.sz;
		++num;
		allsz+=pair.first.sz;
		if(pair.second.sz > 0){
			io[num].b_body = pair.second.buf;
			io[num].b_sz = pair.second.sz;
			++num;
			allsz+=pair.second.sz;
		}
	}
	uint32_t zsBuf;
	if(buf){
		zsBuf = htonl(sz);
		io[num].b_body = &zsBuf;
		io[num].b_sz = 2;
		++num;
		allsz+=2;
		io[num].b_body = buf;
		io[num].b_sz = sz;
		++num;
		allsz+=sz;
	}
	if(num > 0){
		int wsz=0;
		struct wod_socket_buf *tmpio = io;
		for(;;){
			int wret = wod_net_writev(cli->fd,tmpio,num);
			if(wret<0){
				if(errno == EAGAIN){
					if(buf){
						int leftsz = allsz-wsz;
						uint8_t *tmpbuf = NULL;
						if(leftsz > sz){
							int dif = (sz+4-leftsz);
							tmpbuf = (uint8_t *)&zsBuf+dif;
							wod_cycle_buffer_push(&cli->cycle_wrbuf,&zsBuf,4-dif);
						}else{
							tmpbuf = buf+(sz-leftsz);
							wod_cycle_buffer_push(&cli->cycle_wrbuf,tmpbuf,leftsz);
						}
					}
					wod_cycle_buffer_pop(&cli->cycle_wrbuf,wret);
					return 1;
				}else if(errno != EINTR){
					return -1;
				}
			}else{
				wsz+=wret;
				if(wret == allsz){
					wod_cycle_buffer_pop(&cli->cycle_wrbuf,wret);
					return 0;
				}
			}
			int tmpwsz = wsz;
			int i=0,tmpnum = num;
			for(i=0; i<tmpnum ;i++){
				tmpwsz-=tmpio->b_sz;
				if(tmpwsz >= 0){
					++tmpio;
					num--;
				}else{
					tmpio->b_body = (uint8_t*)(tmpio->b_body)+(tmpio->b_sz+tmpwsz);
					tmpio->b_sz = -tmpwsz;
					break;
				}
			}
		}
	}else{
		return 0;
	}
	assert(0);
	return 0;
}
/**
 * return>0:缓冲区空了
 * return=0:读到结尾。
 * return<0:出错了
 */
static int
real_read(tcpclient_t * cli)
{
	struct wod_cycle_pair pair;
	int fret = 0;

	for(;;){
		int ret = wod_cycle_buffer_grow(&cli->cycle_rebuf,0,&pair);
		if(ret != 0){
			ret = wod_cycle_buffer_grow(&cli->cycle_rebuf,512,&pair);
			if(ret !=0 ){
				fret = -1;
				break;
			}
		}
		int rret = 0,allsz=0,num=0;
		struct wod_socket_buf io[2];
		io[num].b_body = pair.first.buf;
		io[num].b_sz = pair.first.sz;
		++num;
		allsz +=pair.first.sz;
		if(pair.second.sz > 0 ){
			io[1].b_body = pair.second.buf;
			io[1].b_sz = pair.second.sz;
			++num;
			allsz +=pair.second.sz;
		}
		rret = wod_net_readv(cli->fd,io,num);
		if(rret < 0){
			wod_cycle_buffer_back(&cli->cycle_rebuf,allsz);
			if(errno == EAGAIN){
				fret = 1;
				break;
			}else if(errno != EINTR){
				fret = -1;
				break;
			}
		}else if(rret == 0){
			fret = 0;
			wod_cycle_buffer_back(&cli->cycle_rebuf,allsz);
			break;
		}else if(allsz > rret){
			fret = 1;
			wod_cycle_buffer_back(&cli->cycle_rebuf,allsz-rret);
			break;
		}
	}
	return fret;
}
void
tcpclient_close(tcpclient_t * cli)
{
	if(cli->sockstate & SOCK_READ_CLOSED){
		return ;
	}
	wod_event_io_remove(cli->svr->loop,cli->fd,WV_IO_READ);
	cli->sockstate |= SOCK_READ_CLOSED;
	shutdown(cli->fd,SHUT_RD);
	if(cli->sockstate & SOCK_WRITE_CLOSED){
		tcpsvr_notifyclosecp(cli->svr,cli);
		_release(cli);
	}
}
void
tcpclient_destroy(tcpclient_t * cli)
{
	tcpclient_close(cli);
	if(!tcpclient_isactive(cli)){
		return ;
	}
	if(wod_cycle_buffer_empty(&cli->cycle_rebuf)){
		wod_event_io_remove(cli->svr->loop,cli->fd,WV_IO_WRITE);
		tcpsvr_notifyclosecp(cli->svr,cli);
		_release(cli);
	}else{
		cli->sockstate |= SOCK_WRITE_WAIT_CLOSED;
	}
}

