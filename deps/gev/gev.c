/*
 * gev.c
 *
 *  Created on: 2012-10-23
 *      Author: goalworld
 */

#include "gev_inner.h"
#include "gev.h"
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
gev_malloc_proc gevMalloc = malloc;
gev_realloc_proc gevRealloc = realloc;
gev_free_proc gevFree = free;
static int _initPollor(struct gev_pollor* pllor,int type);
static inline void _gev_sleep(int usec){
	if( usec <= 0 ){
		return ;
	}
	struct timeval tv;
	tv.tv_sec = usec/1000000;
	tv.tv_usec = usec - tv.tv_sec*1000000;
	select(0,NULL,NULL,NULL,&tv);
}
static double inline gevGetTime(){
	struct timeval time;
	gettimeofday(&time,NULL);
	return time.tv_sec + time.tv_usec * 1E-6;
}
static inline int _hashFunction(int id){
	return id%HASH_SIZE;
}
struct gev_loop * gev_loop_new(int set_size,int type){
	struct gev_loop * loop = gevMalloc(sizeof(struct gev_loop));
	int ret = _initPollor(&loop->pollor,type);
	if(ret == GEV_RERROR){
		gevFree(loop);
		return NULL;
	}
	ret = loop->pollor.Init(loop,0);
	if(ret == GEV_RERROR){
		gevFree(loop);
		return NULL;
	}
	loop->set_size = set_size;
	loop->idIndex = loop->set_size;
	loop->userdefHead = NULL;
	loop->files = gevMalloc(sizeof(struct gev_file) *set_size);
	loop->pendFds = gevMalloc(sizeof(int) *set_size);
	int i=0;
	for(i=0; i< loop->set_size; i++){
		loop->files[i].event = GEV_NONE;
	}
	memset(loop->hashMap,0,sizeof(loop->hashMap));
	loop->isQuit = 0;
	loop->used = 0;
	loop->minSec = SLEEP;
	return loop;
}
void gev_loop_delete(struct gev_loop *loop){
	loop->pollor.Destroy(loop);
	gevFree(loop);
}
static void _processFile(struct gev_loop *loop,double runSec){
	double tmpSec = loop->minSec - runSec;
	if(tmpSec < 0.0 ){
		tmpSec = 0.0;
	}
	if(loop->used){
		int ret = loop->pollor.Poll(loop,tmpSec);
		struct gev_file * fev;
		for(;ret>0;ret--){
			fev = &loop->files[loop->pendFds[ret-1]];
			if(fev->event & fev->revent & GEV_IO_READ){
				fev->readProc(fev->readArg,GEV_IO_READ);
			}
			if(fev->event & fev->revent & GEV_IO_WRITE){
				fev->writeProc(fev->writeArg,GEV_IO_WRITE);
			}
		}
	}else{
		int usec = tmpSec*1E6 - 100;
		_gev_sleep(usec);
	}
}
static void _processIdle(struct gev_loop *loop){
	struct gev_userdef*tmp=loop->userdefHead,*pre = NULL,*next;
	while(tmp){
		next = tmp->next;
		if(!tmp->dispose){
			tmp->idluceProc(tmp->userdefArg);
		}else{
			if(pre){
				pre->next = next;
			}else{
				loop->userdefHead = next;
			}
			gevFree(tmp);
		}
		pre = tmp;
		tmp = next;
	}
}
static void _processTime(struct gev_loop *loop){
	int i =0;
	loop->minSec = SLEEP;
	for(i=0;i<HASH_SIZE;i++){
		struct gev_time*tmp=loop->hashMap[i],*pre = NULL,*next;
		while(tmp){
			next = tmp->next;
			if(!tmp->dispose){
				double cutClock = gevGetTime();
				tmp->passSec += cutClock-tmp->cutClock;
				tmp->cutClock = cutClock;
				double tmpSec;
				if(tmp->passSec >= tmp->sec){
					tmp->passSec = 0.0;
					tmp->dispose = tmp->timeProc(tmp->timeArg);
					tmpSec = tmp->sec;
				}else{
					tmpSec = (tmp->sec - tmp->passSec);
				}
				if(tmpSec < loop->minSec){
					loop->minSec = tmpSec;
				}
			}
			if(tmp->dispose){
				if(pre){
					pre->next = next;
				}else{
					loop->hashMap[i] = next;
				}
				gevFree(tmp);
			}
			pre = tmp;
			tmp = next;
		}
	}
}
void gev_loop_run(struct gev_loop *loop){
	double space = 0.0;
	while(!loop->isQuit){
		double  preSec = gevGetTime();
		_processTime(loop);
		_processFile(loop,space);
		_processIdle(loop);
		space =  gevGetTime() - preSec;
	}
}
void gev_loop_stop(struct gev_loop *loop){
	loop->isQuit = 1;
}

int gev_file_add(struct gev_loop *loop,int fd,int event,gev_file_proc cb,void *cbArg){
	if(fd > loop->set_size || !cb){
		return GEV_RERROR;
	}
	struct gev_file *pFile = &loop->files[fd];
	pFile->fd = fd;
	int ret = loop->pollor.Add(loop,fd,event);
	if(ret != GEV_ROK){
		return GEV_RERROR;
	}
	if(pFile->event == GEV_NONE){
		loop->used ++ ;
	}
	pFile->event |= event;
	if(event & GEV_IO_READ){
		pFile->readArg = cbArg;
		pFile->readProc = cb;
	}
	if(event & GEV_IO_WRITE){
		pFile->writeArg = cbArg;
		pFile->writeProc = cb;
	}

	return fd;
}
void gev_file_remove(struct gev_loop *loop,int id,int event){
	if(id > loop->set_size){
		return ;
	}
	struct gev_file *pFile = &loop->files[id];
	if(pFile->event & event){//存在该事件
		loop->pollor.Remove(loop,pFile->fd,event);
		pFile->event &= (~event);
		if(pFile->event == GEV_NONE){
			loop->used --;
		}
	}
}

int gev_time_add(struct gev_loop *loop,int sec,int usec,gev_time_proc cb,void *cbArg){
	if(!cb){
		return GEV_RERROR;
	}
	struct gev_time * pTime = gevMalloc(sizeof(struct gev_time));
	pTime->id = loop->idIndex++;
	pTime->cutClock = gevGetTime();
	pTime->sec = sec + usec*1E-6;
	pTime->passSec = 0.0;
	pTime->timeArg = cbArg;
	pTime->timeProc = cb;
	pTime->dispose = 0;
	int hash = _hashFunction(pTime->id);
	pTime->next = loop->hashMap[hash];
	loop->hashMap[hash] = pTime;
	return pTime->id;
}
void gev_time_remove(struct gev_loop * loop,int id){
	int hash = _hashFunction(id);
	struct gev_time*tmp=loop->hashMap[hash];
	while(tmp){
		if(tmp->id == id){
			tmp->dispose = 1;
		}
		tmp = tmp->next;
	}
}

int gev_userdef_add(struct gev_loop *loop,gev_userdef_proc cb,void *cbArg){
	if(!cb){
		return GEV_RERROR;
	}
	struct gev_userdef * pIdle = gevMalloc(sizeof(struct gev_userdef));
	pIdle->next = loop->userdefHead;
	loop->userdefHead  = pIdle;
	pIdle->id = loop->idIndex++;
	pIdle->userdefArg = cbArg;
	pIdle->idluceProc = cb;
	pIdle->dispose = 0;
	return pIdle->id;
}
void gev_userdef_remove(struct gev_loop *loop,int id){
	struct gev_userdef*tmp=loop->userdefHead;
	while(tmp){
		if(tmp->id == id){
			tmp->dispose = 1;
		}
		tmp = tmp->next;
	}
}

void gev_set_malloc(gev_malloc_proc fn){
	gevMalloc = fn;
}
void gev_set_realloc(gev_realloc_proc fn){
	gevRealloc = fn;
}
void gev_set_free(gev_free_proc fn){
	gevFree = fn;
}

#define HAS_EPOLL 1
#if HAS_EPOLL
#include "gev_epoll.c"
#endif
#if HAS_SELECT
#include "gev_epoll.c"
#endif

static int _initPollor(struct gev_pollor* pllor,int type){
	if(type == GEV_POLL_EPOLL){
		pllor->Add = _epoll_add;
		pllor->Destroy = _epoll_destroy;
		pllor->Init = _epoll_init;
		pllor->Poll = _epoll_poll;
		pllor->Remove = _epoll_remove;
		return GEV_ROK;
	}
	return GEV_RERROR;
}
