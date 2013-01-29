/*
 * rain_lifequeue.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_LIFEQUEUE_H_
#define RAIN_LIFEQUEUE_H_
#include "rain_type.h"
int rain_lifequeue_int();
void rain_lifequeue_push(routine_t rid);
int rain_lifequeue_pop(routine_t *rid);
#endif /* RAIN_LIFEQUEUE_H_ */
