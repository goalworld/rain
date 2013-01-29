/*
 * gev_epoll.c
 *
 *  Created on: 2012-10-23
 *      Author: goalworld
 */

#include "gev_inner.h"
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
struct _epollInfo{
	int epFd;
};

static int _epoll_init(struct gev_loop * loop,int flag){

	struct _epollInfo * pInfo = gevMalloc(sizeof(struct _epollInfo));
	if((pInfo->epFd = epoll_create1(EPOLL_CLOEXEC)) < 0){
		gevFree(pInfo);
		return GEV_RERROR;
	}
	loop->pollorData = pInfo;
	return GEV_ROK;
}
static void _epoll_destroy(struct gev_loop *loop){
	struct _epollInfo * pInfo = (struct _epollInfo *)(loop->pollorData);
	close( pInfo->epFd );
	gevFree(loop->pollorData);
}
static int _epoll_add(struct gev_loop *loop, int fd, int mask){
	struct _epollInfo * pInfo = (struct _epollInfo *)(loop->pollorData);
	struct epoll_event epEv;
	epEv.data.fd = fd;
	mask |=loop->files[fd].event;
	epEv.events = (mask & GEV_IO_READ) ? EPOLLIN:0 |(mask & GEV_IO_WRITE) ? EPOLLOUT:0 ;
	if(epEv.events != 0){
		//epEv.events |= EPOLLET
		int ret;
		if(loop->files[fd].event == GEV_NONE){
			ret = epoll_ctl(pInfo->epFd,EPOLL_CTL_ADD,fd,&epEv);
			if(errno == EEXIST){
				ret = epoll_ctl(pInfo->epFd,EPOLL_CTL_MOD,fd,&epEv);
			}
		}else{
			ret = epoll_ctl(pInfo->epFd,EPOLL_CTL_MOD,fd,&epEv);
		}
		if(ret == 0){
			return  GEV_ROK;
		}
	}
	return GEV_RERROR;
}

static int _epoll_remove(struct gev_loop *loop ,int fd, int mask){
	struct _epollInfo * pInfo = (struct _epollInfo *)(loop->pollorData);
	struct epoll_event epEv;
	epEv.data.fd = fd;
	mask =(loop->files[fd].event & (~mask));
	epEv.events = (mask & GEV_IO_READ) ? EPOLLIN:0 |(mask & GEV_IO_WRITE) ? EPOLLOUT:0 ;
	int ret;
	if(mask == GEV_NONE){
		for(;;){
			ret = epoll_ctl(pInfo->epFd,EPOLL_CTL_DEL,fd,&epEv);
			if(ret < 0){
				if(errno == EINTR){
					continue;
				}else{
					return GEV_RERROR;
				}
			}else{
				return GEV_ROK;
			}
		}
	}else{
		for(;;){
			ret = epoll_ctl(pInfo->epFd,EPOLL_CTL_MOD,fd,&epEv);
			if(ret < 0){
				if(errno == EINTR){
					continue;
				}else{
					return GEV_RERROR;
				}
			}else{
				return GEV_ROK;
			}
		}
	}
}

static int _epoll_poll(struct gev_loop *loop,double timeOut){
	struct _epollInfo * pInfo = (struct _epollInfo *)(loop->pollorData);
	int ret;
	struct epoll_event epEvs[loop->set_size];
	for(;;){
		ret = epoll_wait(pInfo->epFd,epEvs,loop->set_size,timeOut*1E3);
		if(ret < 0){
			if(errno == EINTR){
				continue;
			}else{
				return GEV_RERROR;
			}
		}else{
			break;
		}
	}
	int tmp = ret;
	struct epoll_event * epEv = epEvs ;
	int * pPendFd = loop->pendFds;
	for(;tmp>0;tmp--){
		loop->files[epEv->data.fd].revent = epEv->events & ( EPOLLIN | EPOLLHUP | EPOLLERR ) ? GEV_IO_READ:0
				|( epEv->events & ( EPOLLIN | EPOLLHUP | EPOLLERR ) ) ? GEV_IO_WRITE:0;
		epEv++;
		*(pPendFd++) = epEv->data.fd;
	}
	return ret;
}
