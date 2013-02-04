/*
 * jsv8_rain_ctx.cc
 *
 *  Created on: 2012-11-13
 *      Author: goalworld
 */

#include "jsv8_ctx.h"
#include <string.h>
#include <algorithm>
using namespace v8;
template<class T>
T* get_cppobj_ptr(v8::Handle<v8::Object> obj)
{
	v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(obj->GetInternalField(0)) ;
	void* raw_obj_ptr = field->Value();
	return static_cast<T*>(raw_obj_ptr);
};
static void
_delete(jsv8_ctx_t::FunctionMap::value_type fn)
{
	if(!(fn.second).IsEmpty()){
		(fn.second).Dispose();
	}
}
jsv8_ctx_t::~jsv8_ctx_t()
{
	if(!this->exit_cb.IsEmpty()){
		this->exit_cb.Dispose();
	}
	if(!this->recv_cb.IsEmpty()){
		this->recv_cb.Dispose();
	}
	std::for_each(this->func_map_.begin(),this->func_map_.end(),_delete);
}
v8::Handle<v8::Value>
jsv8_ctx_t::Register(jsv8_ctx_t * g_foo_ptr)
{
	v8::HandleScope scope;
	v8::Handle<v8::ObjectTemplate> foo_templ ;
	v8::Handle<v8::Object> foo_class_obj ;
	v8::Handle<v8::External> foo_class_ptr ;


	foo_templ = v8::ObjectTemplate::New();
	foo_templ->SetInternalFieldCount(1);
	foo_class_obj = foo_templ->NewInstance();

	foo_class_ptr = v8::External::New(static_cast<jsv8_ctx_t *>(g_foo_ptr)) ;
	foo_class_obj->SetInternalField(0, foo_class_ptr) ;

	foo_class_obj->Set(v8::String::New("Send"),
			v8::FunctionTemplate::New(jsv8_ctx_t::Send)->GetFunction()) ;
	foo_class_obj->Set(v8::String::New("Responce"),
			v8::FunctionTemplate::New(jsv8_ctx_t::Responce)->GetFunction()) ;
	foo_class_obj->Set(v8::String::New("On"),
			v8::FunctionTemplate::New(jsv8_ctx_t::On)->GetFunction());
	foo_class_obj->Set(v8::String::New("Kill"),
			v8::FunctionTemplate::New(jsv8_ctx_t::Kill)->GetFunction());
	return scope.Close(foo_class_obj);
}
v8::Handle<v8::Value>
jsv8_ctx_t::Send(const v8::Arguments &args)
{
	if(args.Length() == 0 ){
		v8::ThrowException(v8::String::New("Send-Need-at least one argument"));
	}
	if(!args[0]->IsObject()){
		v8::ThrowException(v8::String::New("argument one Need OBject"));
	}
	v8::HandleScope scope;
	v8::Handle<v8::Object> v8_msg = v8::Handle<v8::Object>::Cast(args[0]);
	if(v8_msg->Get(v8::String::New("data")).IsEmpty()){
		v8::ThrowException(v8::String::New("argument one Need OBject With key:data"));
	}
	jsv8_ctx_t * ptr =  get_cppobj_ptr<jsv8_ctx_t>(args.Holder());
	rain_msg_t msg;
	v8::String::Utf8Value file(v8_msg->Get(v8::String::New("data")));
	msg.data = *file;
	msg.sz = file.length();
	if(v8_msg->Get(v8::String::New("type")).IsEmpty()){
		msg.type = 0;
	}else{
		msg.type = v8_msg->Get(v8::String::New("type"))->ToInt32()->Value();
	}
	int ret;
	if(args.Length() > 1 && args[1]->IsFunction()){
		rainSession se;
		ret = rainSend(ptr->ctx_,ptr->destid_,msg,RAIN_COPY,&se);
		if(ret == RAIN_OK){
			ptr->func_map_[se] =
					v8::Persistent<v8::Function>::New(v8::Handle<v8::Function>::Cast(args[1]));
		}
	}else{
		ret =rainSend(ptr->ctx_,ptr->destid_,msg,RAIN_COPY,NULL);
	}
	if(ret == RAIN_OK){
		return scope.Close(v8::Boolean::New(true));
	}
	return scope.Close(v8::Boolean::New(false));
}
v8::Handle<v8::Value>
jsv8_ctx_t::Responce(const v8::Arguments &args)
{
	if(args.Length() < 1 ){
		v8::ThrowException(v8::String::New("Send-Need-at least one argument"));
	}
	if(!args[0]->IsObject()){
		v8::ThrowException(v8::String::New("argument one Need OBject"));
	}
	v8::HandleScope scope;
	v8::Handle<v8::Object> v8_msg = v8::Handle<v8::Object>::Cast(args[0]);
	if(v8_msg->Get(v8::String::New("data")).IsEmpty()){
		v8::ThrowException(v8::String::New("argument one Need OBject With key:data"));
	}
	if( args[1]->ToInt32()->IsUndefined() || args[1]->ToInt32()->IsNull() ){
		v8::ThrowException(v8::String::New("argument two Need Number"));
	}
	jsv8_ctx_t * ptr =  get_cppobj_ptr<jsv8_ctx_t>(args.Holder());
	rain_msg_t msg;
	v8::String::Utf8Value file(v8_msg->Get(v8::String::New("data")));
	msg.data = *file;
	msg.sz = file.length();
	if(v8_msg->Get(v8::String::New("type")).IsEmpty()){
		msg.type = 0;
	}else{
		msg.type = v8_msg->Get(v8::String::New("type"))->ToInt32()->Value();
	}
	rainSession session = args[1]->ToInt32()->Value();
	int ret =rainResponce(ptr->ctx_,ptr->destid_,msg,RAIN_COPY,session);

	if(ret == RAIN_OK){
		return scope.Close(v8::Boolean::New(true));
	}
	return scope.Close(v8::Boolean::New(false));
}
v8::Handle<v8::Value>
jsv8_ctx_t::On(const v8::Arguments &args)
{
	HandleScope scope;
	if(args.Length()<2){
		ThrowException(String::New("argument need 2"));
		return scope.Close(v8::Boolean::New(false));
	}
	if(!args[0]->IsString()){
		ThrowException(String::New("argument:0 need string"));
		return scope.Close(v8::Boolean::New(false));
	}
	String::Utf8Value arg1(args[0]);
	if(!args[1]->IsFunction()){
		ThrowException(String::New("argument:1 need a Function"));
		return scope.Close(v8::Boolean::New(false));
	}
	if(strcmp(*arg1,"message") == 0){
		jsv8_ctx_t * ptr =  get_cppobj_ptr<jsv8_ctx_t>(args.Holder());
		ptr->recv_cb =
				v8::Persistent<Function>::New(v8::Handle<Function>::Cast(args[1]));
		return scope.Close(v8::Boolean::New(true));
	}else if(strcmp(*arg1,"exit") == 0){
		jsv8_ctx_t * ptr =  get_cppobj_ptr<jsv8_ctx_t>(args.Holder());
		ptr->exit_cb =
				v8::Persistent<Function>::New(v8::Handle<Function>::Cast(args[1]));
		rainLink(ptr->ctx_,ptr->destid_);
		return scope.Close(v8::Boolean::New(true));
	}else{
		ThrowException(String::New("argument 0:need a String of message or exit"));
		return v8::Boolean::New(false);
	}
}
v8::Handle<v8::Value>
jsv8_ctx_t::Kill(const v8::Arguments &args)
{
	HandleScope scope;
	jsv8_ctx_t * ptr =  get_cppobj_ptr<jsv8_ctx_t>(args.Holder());
	rainKill(ptr->ctx_,ptr->destid_,0);
	return v8::Undefined();
}
