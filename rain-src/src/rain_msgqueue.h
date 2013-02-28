/*
 * rain_msgqueue.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_MSGQUEUE_H_
#define RAIN_MSGQUEUE_H_
#include "rain.h"
#include "rain_msg.h"
struct rain_message_queue;
int rain_message_queue_init();
typedef void (*rain_message_del_cb)(struct rain_ctx_message *);
struct rain_message_queue* rain_message_queue_new();
void rain_message_queue_delete(struct rain_message_queue*,rain_message_del_cb delfn);
void rain_message_queue_push(struct rain_message_queue *,struct rain_ctx_message msg);
int  rain_message_queue_pop(struct rain_message_queue *,struct rain_ctx_message *msg);
int  rain_message_queue_size(struct rain_message_queue *);
#endif /* RAIN_MSGQUEUE_H_ */
