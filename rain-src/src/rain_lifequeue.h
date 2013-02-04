/*
 * rain_lifequeue.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_LIFEQUEUE_H_
#define RAIN_LIFEQUEUE_H_
#include "rain_type.h"
int rainLifeQueueInit();
void rainLifeQueuePush(rainRoutine rid);
int rainLifeQueuePop(rainRoutine *rid);
#endif /* RAIN_LIFEQUEUE_H_ */
