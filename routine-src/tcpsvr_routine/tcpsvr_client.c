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
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>
#include <memory.h>
#include <netinet/in.h>

#define TCPCLI_BUF_DEFAULT_SZ 1024
static void _read_cb(struct ev_loop * loop, ev_io *w, int revents);
static void _write_cb(struct ev_loop * loop, ev_io *w, int revents);
static int real_read(tcpclient_t * cli);
static int real_write(tcpclient_t * cli,void *buf,int sz);

int
tcpclient_init(tcpclient_t * cli,int fd,int id)
{
	if(cli->binuse){
		return RAIN_ERROR;
	}
	cli->id = id;
	cli->fd = fd;
	int ret = wodCycleBufInit(&cli->cycle_wrbuf,TCPCLI_BUF_DEFAULT_SZ);
	if(ret !=0){
		return RAIN_ERROR;
	}
	ret = wodCycleBufInit(&cli->cycle_rebuf,TCPCLI_BUF_DEFAULT_SZ);
	if(ret !=0){
		wodCycleBufDestroy(&cli->cycle_wrbuf);
		return RAIN_ERROR;
	}
	cli->sockstate = SOCK_OK;
	ev_io_init(&cli->ioread,_read_cb,fd,EV_READ);
	ev_io_init(&cli->iowrite,_write_cb,fd,EV_WRITE);
	cli->ioread.data = cli;
	cli->iowrite.data = cli;
	ev_io_start(tcpsvr_getloop(cli->svr),&cli->ioread);
	return RAIN_OK;
}

static inline void
_release(tcpclient_t * cli)
{
	close(cli->fd);
	wodCycleBufDestroy(&cli->cycle_wrbuf);
	wodCycleBufDestroy(&cli->cycle_rebuf);
}

static void inline
_do_error(tcpclient_t * cli)
{
	ev_io_stop(tcpsvr_getloop(cli->svr),&cli->iowrite);
	ev_io_stop(tcpsvr_getloop(cli->svr),&cli->ioread);
	tcpsvr_notifyerror(cli->svr,cli);
	_release(cli);
}
static inline void
_paser2(tcpclient_t * cli ,struct wodCyclePair pair)
{
	int firstsz = pair.first.sz;
	int secondsz = pair.second.sz;
	uint8_t * cutBuf = pair.first.buf;
	int bufsz = 0;// big-endian
	bool bFirst = true;
	int numRead = 0;
	while((firstsz + secondsz) > 2){
		if(bFirst){
			if(firstsz >= 2){
				bufsz = (cutBuf[0])<<8|(cutBuf[1]);
				cutBuf+=2;
				firstsz-=2;
				if((firstsz + secondsz) >=bufsz){
					numRead+=(2+bufsz);
					void *buf = malloc(bufsz+4);
					*(int *)(buf) = cli->id;
					if(firstsz >= bufsz){
						memcpy(buf+4,cutBuf,bufsz);
						cutBuf+=bufsz;
						firstsz-=bufsz;
						tcpsvr_notifyread(cli->svr,cli,buf,bufsz+4);
					}else{
						memcpy(buf+4,cutBuf,firstsz);
						cutBuf = pair.second.buf;
						firstsz = 0;
						memcpy(buf+4+firstsz,cutBuf,bufsz-firstsz);
						secondsz -= (bufsz-firstsz);
						bFirst = false;
						cutBuf+=(bufsz-firstsz);
						tcpsvr_notifyread(cli->svr,cli,buf,bufsz+4);
					}
					continue;
				}else{
					break;
				}
			}else{
				bFirst = false;
				if(firstsz == 1){
					bufsz  = (cutBuf[0])<<8|(*(uint8_t*)(pair.second.buf));
					cutBuf = (uint8_t*)(pair.second.buf)+1;
					secondsz-=1;
					firstsz = 0;
				}else{
					cutBuf = pair.second.buf;
					bufsz = (cutBuf[0])<<8|(cutBuf[1]);
					secondsz-=2;
					cutBuf+=2;
				}
				if((firstsz + secondsz) >= bufsz){
					numRead+=(2+bufsz);
					void *buf = malloc(bufsz+4);
					*(int *)(buf) = cli->id;
					memcpy(buf+4,cutBuf,bufsz);
					cutBuf+=bufsz;
					secondsz-=bufsz;
					tcpsvr_notifyread(cli->svr,cli,buf,bufsz+4);
					continue;
				}else{
					break;
				}
			}
		}else{
			bufsz = (cutBuf[0])<<8|(cutBuf[1]);
			cutBuf+=2;
			secondsz-=2;
			if((firstsz + secondsz) >= bufsz){
				numRead+=(2+bufsz);
				void *buf = malloc(bufsz+4);
				*(int *)(buf) = cli->id;
				memcpy(buf+4,cutBuf,bufsz);
				cutBuf+=bufsz;
				secondsz-=bufsz;
				tcpsvr_notifyread(cli->svr,cli,buf,bufsz+4);
				continue;
			}else{
				break;
			}
		}
	}
	wodCycleBufPop(&cli->cycle_rebuf,numRead);
}
static void
_read_cb(struct ev_loop * loop, ev_io *w, int revents)
{
	tcpclient_t * cli = (tcpclient_t *)w->data;
	int ret = real_read(cli);
	struct wodCyclePair  pair;
	if( wodCycleBufUsed(&cli->cycle_rebuf,&pair) == 0){
		if(cli->svr->headsz == 2){
			_paser2(cli,pair);
		}else{
			wodCycleBufPop(&cli->cycle_rebuf,pair.first.sz+pair.second.sz);
		}
	}
	if(ret == 0){
		ev_io_stop(tcpsvr_getloop(cli->svr),&cli->ioread);
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
_write_cb(struct ev_loop * loop, ev_io *w, int revents)
{
	tcpclient_t * cli = (tcpclient_t *)w->data;
	tcpclient_write(cli,NULL,0);
}
int
tcpclient_write(tcpclient_t * cli,void *buf,int sz)
{
	int send = real_write(cli,buf,sz);
	if(send == 0){
		ev_io_stop(tcpsvr_getloop(cli->svr),&cli->iowrite);
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
		ev_io_start(tcpsvr_getloop(cli->svr),&cli->iowrite);
	}
	return send;
}

static int
real_write(tcpclient_t * cli,void *buf,int sz)
{
	assert(sz <= 0xffff);
	struct wodCyclePair pair;
	int ret = wodCycleBufUsed(&cli->cycle_wrbuf,&pair);
	struct iovec io[4];
	int num = 0;
	int allsz = 0;
	if(ret == 0){
		io[num].iov_base = pair.first.buf;
		io[num].iov_len = pair.first.sz;
		++num;
		allsz+=pair.first.sz;
		if(pair.second.sz > 0){
			io[num].iov_base = pair.second.buf;
			io[num].iov_len = pair.second.sz;
			++num;
			allsz+=pair.second.sz;
		}
	}
	uint8_t zsBuf[2];
	if(buf){
		zsBuf[0] = sz >>8;
		zsBuf[1] = sz;
		io[num].iov_base = zsBuf;
		io[num].iov_len = 2;
		++num;
		allsz+=2;
		io[num].iov_base = buf;
		io[num].iov_len = sz;
		++num;
		allsz+=sz;
	}
	if(num > 0){
		int wsz=0;
		struct iovec *tmpio = io;
		for(;;){
			int wret = writev(cli->fd,tmpio,num);
			if(wret<0){
				if(errno == EAGAIN){
					if(buf){
						int tmp = allsz-wsz;
						uint8_t *tmpbuf = NULL;
						if(tmp > sz){
							int dif = (sz+2-tmp)>0?(sz+2-tmp):0;
							tmpbuf = zsBuf+dif;
							wodCycleBufPush(&cli->cycle_wrbuf,zsBuf,2-dif);
						}else{
							tmpbuf = buf+(sz-tmp);
							wodCycleBufPush(&cli->cycle_wrbuf,tmpbuf,tmp);
						}
					}
					wodCycleBufPop(&cli->cycle_wrbuf,wret);
					return 1;
				}else if(errno != EINTR){
					return -1;
				}
			}else{
				wsz+=wret;
				if(wret == allsz){
					wodCycleBufPop(&cli->cycle_wrbuf,wret);
					return 0;
				}
			}
			int tmpwsz = wsz;
			int i=0,tmpnum = num;
			for(i=0; i<tmpnum ;i++){
				tmpwsz-=tmpio->iov_len;
				if(tmpwsz >= 0){
					++tmpio;
					num--;
				}else{
					tmpio->iov_base = (uint8_t*)(tmpio->iov_base)+(tmpio->iov_len+tmpwsz);
					tmpio->iov_len = -tmpwsz;
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
	struct wodCyclePair pair;
	int fret = 0;

	for(;;){
		int ret = wodCycleBufGrow(&cli->cycle_rebuf,0,&pair);
		if(ret != 0){
			ret = wodCycleBufGrow(&cli->cycle_rebuf,512,&pair);
			if(ret !=0 ){
				fret = -1;
				break;
			}
		}
		int rret = 0,allsz=0,num=0;
		struct iovec io[2];
		io[num].iov_base = pair.first.buf;
		io[num].iov_len = pair.first.sz;
		++num;
		allsz +=pair.first.sz;
		if(pair.second.sz > 0 ){
			io[1].iov_base = pair.second.buf;
			io[1].iov_len = pair.second.sz;
			++num;
			allsz +=pair.second.sz;
		}
		rret = readv(cli->fd,io,num);
		if(rret < 0){
			wodCycleBufBack(&cli->cycle_rebuf,allsz);
			if(errno == EAGAIN){
				fret = 1;
				break;
			}else if(errno != EINTR){
				fret = -1;
				break;
			}
		}else if(rret == 0){
			fret = 0;
			wodCycleBufBack(&cli->cycle_rebuf,allsz);
			break;
		}else if(allsz > rret){
			fret = 1;
			wodCycleBufBack(&cli->cycle_rebuf,allsz-rret);
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
	ev_io_stop(tcpsvr_getloop(cli->svr),&cli->ioread);
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
	if(wodCycleBufEmpty(&cli->cycle_rebuf)){
		ev_io_stop(tcpsvr_getloop(cli->svr),&cli->iowrite);
		tcpsvr_notifyclosecp(cli->svr,cli);
		_release(cli);
	}else{
		cli->sockstate |= SOCK_WRITE_WAIT_CLOSED;
	}
}

