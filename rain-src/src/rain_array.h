/*
 * rain_array.h
 *
 *  Created on: 2012-11-14
 *      Author: goalworld
 */

#ifndef RAIN_ARRAY_H_
#define RAIN_ARRAY_H_

typedef struct rain_array_s rain_array_t;

struct rain_array_s
{
	void *arr_data;
	int elem_sz;
	int arr_sz;
	int cut_sz;
};

void rain_array_init(rain_array_t *arr,int elemsz);
void rain_array_destroy(rain_array_t *arr);
void rain_array_pushback(rain_array_t *arr,void *elem);
void rain_array_at(rain_array_t *arr,int index,void *elem);
void rain_array_set(rain_array_t *arr,int index,void *elem);
void rain_array_pushfront(rain_array_t *arr,void *elem);
void rain_array_erase(rain_array_t *arr,int index,int numelem,void *elem);
void rain_array_insert(rain_array_t *arr,int index,void *elem,int numelem);
int rain_array_size(rain_array_t *arr);
#endif /* RAIN_ARRAY_H_ */
