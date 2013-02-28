/*
 * jsv8_rain_ctx.h
 *
 *  Created on: 2012-11-13
 *      Author: goalworld
 */

#ifndef JSV8_RAIN_CTX_H_
#define JSV8_RAIN_CTX_H_
#include <v8.h>
#include <rain.h>
#include <map>

struct jsv8_ctx_t
{
public:
	static v8::Handle<v8::Value> Register(jsv8_ctx_t * jsv8);
	static v8::Handle<v8::Value> On(const v8::Arguments &arg);
	static v8::Handle<v8::Value> Send(const v8::Arguments &arg);
	static v8::Handle<v8::Value> Responce(const v8::Arguments &arg);
	static v8::Handle<v8::Value> Kill(const v8::Arguments &arg);
public:
	jsv8_ctx_t(struct rain_ctx* ctx,rain_routine_t destid):ctx_(ctx),destid_(destid){};
	~jsv8_ctx_t();
	typedef std::map< rain_session_t,v8::Persistent<v8::Function> > FunctionMap;
	typedef FunctionMap::iterator FunctionMapItr;

	v8::Persistent<v8::Function> recv_cb;
	v8::Persistent<v8::Function> exit_cb;
	FunctionMap func_map_;
	struct rain_ctx* ctx_;
	rain_routine_t destid_;
};

#endif /* JSV8_RAIN_CTX_H_ */
