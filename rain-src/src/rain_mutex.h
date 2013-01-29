/*
 * rain_mutex.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_MUTEX_H_
#define RAIN_MUTEX_H_
#ifdef __GNUC__

typedef int rain_mutex_t;

#define rain_mutex_init( mtx )	*(mtx) = 0;
#define rain_mutex_lock( mtx )	while(__sync_lock_test_and_set((mtx),1)){;}
#define rain_mutex_unlock( mtx )__sync_lock_release ((mtx),0);


#endif
#endif /* RAIN_MUTEX_H_ */
