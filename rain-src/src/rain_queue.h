/*
 * rain_queue.h
 *
 *  Created on: 2013-1-17
 *      Author: wd
 */

#ifndef RAIN_QUEUE_H_
#define RAIN_QUEUE_H_

typedef struct rain_queue
{
	void * q_buf;
	int q_cut;
	int q_end;
	int q_len;
	int q_elemsize;
}rain_queue_t;
typedef void rain_elem_release_fn(void *elem);
int rain_queue_init(rain_queue_t *que,unsigned elemsize);
void rain_queue_destroy(rain_queue_t *que,rain_elem_release_fn fn);
void rain_queue_push(rain_queue_t * que,void *elem);
int rain_queue_pop(rain_queue_t * que,void *elem);
unsigned rain_queue_size( rain_queue_t * que );
#endif /* RAIN_QUEUE_H_ */
