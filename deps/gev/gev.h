/*
 * gev.h
 *
 *  Created on: 2012-10-23
 *      Author: goalworld
 */

#ifndef GODEV_H_
#define GODEV_H_

#include <stddef.h>
enum gev_file_type{
	GEV_NONE=0X00,
	GEV_IO_READ=0X01,
	GEV_IO_WRITE=0X02
};
enum{
	GEV_ROK = 0,
	GEV_RERROR=-1
};
enum{
	GEV_POLL_EPOLL,
	GEV_POLL_SELECT,
	GEV_POLL_KQUEUE
};
typedef void (*gev_file_proc)(void * nv,int mask);
typedef int (*gev_time_proc)(void * nv);
typedef int (*gev_userdef_proc)(void * nv);

typedef void *(*gev_malloc_proc)(size_t sz);
typedef void (*gev_free_proc)(void *p);
typedef void *(*gev_realloc_proc)(void *p,size_t sz);

struct gev_loop;
struct gev_loop * gev_loop_new(int set_size,int type);
void gev_loop_delete(struct gev_loop *);
void gev_loop_run(struct gev_loop *);
void gev_loop_stop(struct gev_loop *);

int gev_file_add(struct gev_loop *,int fd,int event,gev_file_proc cb,void *cbArg);
void gev_file_remove(struct gev_loop *,int id,int event);
int gev_file_waite(int fd,int mask,long long timeout);
int gev_time_add(struct gev_loop *,int sec,int usec,gev_time_proc cb,void *cbArg);
void gev_time_remove(struct gev_loop *,int id);
//将会在每个循环都会调用
int gev_userdef_add(struct gev_loop *,gev_userdef_proc cb,void *cbArg);
void gev_userdef_remove(struct gev_loop *,int id);

void gev_set_malloc(gev_malloc_proc fn);
void gev_set_realloc(gev_realloc_proc fn);
void gev_set_free(gev_free_proc fn);

#endif /* GODEV_H_ */
