/*
 * rain_proxy_rpc.h
 *
 *  Created on: 2012-11-10
 *      Author: goalworld
 */

#ifndef RAIN_RPC_H_
#define RAIN_RPC_H_
#include "rain_type.h"
#include "rain_msg.h"
int rain_rpc_init();
int rain_rpc_send(rainRoutine dest, struct rainCtxMsg msg);

#endif /* RAIN_PROXY_RPC_H_ */
