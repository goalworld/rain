/*
 * cycle_buffer.h
 *
 *  Created on: 2012-11-11
 *      Author: goalworld
 */

#ifndef CYCLE_BUFFER_H_
#define CYCLE_BUFFER_H_
#include <stdbool.h>
typedef struct cycle_buffer_s
{
	char *buf;
	int cap;
	int head;
	int tail;
}cycle_buffer_t;
typedef struct cycle_pair_s
{
	struct {
		void *buf;
		int sz;
	}first,second;
}cycle_pair_t;
int cycle_buffer_init( cycle_buffer_t* cycle,int defsize);
//growsz =0 剩下的空间（除了tail标记的）都将被设置为使用
int cycle_buffer_grow(cycle_buffer_t* cycle,int growsz,cycle_pair_t *pair);
void cycle_buffer_back(cycle_buffer_t* cycle,int backsz);
int cycle_buffer_push(cycle_buffer_t* cycle,void *buf,int sz);
void cycle_buffer_pop(cycle_buffer_t* cycle,int sz);
int cycle_buffer_getused(cycle_buffer_t* cycle,cycle_pair_t * pair);
void cycle_buffer_destroy( cycle_buffer_t* cycle );

bool cycle_buffer_empty( cycle_buffer_t* cycle );
#endif /* CYCLE_BUFFER_H_ */
