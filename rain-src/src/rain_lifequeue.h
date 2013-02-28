/*
 * rain_lifequeue.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_LIFEQUEUE_H_
#define RAIN_LIFEQUEUE_H_
#include "rain_type.h"
int rain_life_queue_init();
void rain_life_queue_push(rain_routine_t rid);
int rain_life_queue_pop(rain_routine_t *rid);
#endif /* RAIN_LIFEQUEUE_H_ */
