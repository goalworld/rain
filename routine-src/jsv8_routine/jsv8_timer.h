/*
 * jsv8_timer.h
 *
 *  Created on: 2012-11-22
 *      Author: goalworld
 */

#ifndef JSV8_TIMER_H_
#define JSV8_TIMER_H_
#include <v8.h>
class jsv8_timer
{
public:
	bool Pending();
private:
	unsigned int id_;
	int ms_;
	int times_;
	v8::Persistent<v8::Function> cb_;
};

#endif /* JSV8_TIMER_H_ */
