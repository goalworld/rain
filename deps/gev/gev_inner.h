/*
 * gev_inner.h
 *
 *  Created on: 2012-10-23
 *      Author: goalworld
 */

#ifndef GODEV_INNER_H_
#define GODEV_INNER_H_
#include "gev.h"
#define HASH_SIZE 32
#define SLEEP 1E1//1ms
struct gev_loop;
struct gev_pollor{
	int (*Init)(struct gev_loop * loop,int flag);
	void (*Destroy)(struct gev_loop *loop);
	int (*Add)(struct gev_loop *loop,int fd,int mask);
	int	(*Remove)(struct gev_loop *loop , int fd,int mask);
	int (*Poll)(struct gev_loop *loop,double timeOut);
};
struct gev_time{
	int id;
	gev_time_proc timeProc;
	void * timeArg;
	double sec;
	double passSec;
	double cutClock;
	int dispose;
	int repetTims;
	struct gev_time *next;
};
struct gev_file{
	int fd;
	gev_file_proc readProc;
	void * readArg;
	gev_file_proc writeProc;
	void * writeArg;
	int event;//GODEV_NONE
	int revent;//GODEV_NONE
};
struct gev_userdef{
	int id;
	gev_userdef_proc idluceProc;
	int dispose;
	void * userdefArg;
	struct gev_userdef * next;
};
struct gev_loop{
	struct gev_pollor pollor;
	void * pollorData;
	int isQuit;
	int set_size;
	struct gev_file * files;
	int *pendFds;
	int idIndex;
	struct gev_userdef * userdefHead;
	struct gev_time* hashMap[HASH_SIZE];
	int used;
	double minSec;
};

extern gev_malloc_proc gevMalloc;
extern gev_realloc_proc gevRealloc;
extern gev_free_proc gevFree;
#endif /* GODEV_INNER_H_ */
