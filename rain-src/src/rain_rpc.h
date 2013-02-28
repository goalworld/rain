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
int rainRpcInit();
int rainRpcSend(rain_routine_t dest, struct rain_ctx_message msg);

#endif /* RAIN_PROXY_RPC_H_ */
