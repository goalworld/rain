/*
 * jsv8_rain.cc
 *
 *  Created on: 2012-11-12
 *      Author: goalworld
 */


#include "jsv8.h"
#include "jsv8_ctx.h"
#include <stddef.h>
#include <rain.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <binders.h>
using namespace v8;
using namespace std;
template<class T> T*
get_cppobj_ptr(v8::Handle<v8::Object> obj)
{
	v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(obj->GetInternalField(0)) ;
	void* raw_obj_ptr = field->Value();
	return static_cast<T*>(raw_obj_ptr);
};
extern"C"{
void
DisplayExceptionLine (TryCatch &try_catch)
{
	// Prevent re-entry into this function.  For example, if there is
	// a throw from a program in vm.runInThisContext(code, filename, true),
	// then we want to show the original failure, not the secondary one.
	HandleScope scope;

	Handle<Message> message = try_catch.Message();


	fprintf(stderr, "\n");

	if (!message.IsEmpty()) {
		// Print (filename):(line number): (message).
		String::Utf8Value filename(message->GetScriptResourceName());
		const char* filename_string = *filename;
		int linenum = message->GetLineNumber();
		fprintf(stderr, "%s:%i\n", filename_string, linenum);
		// Print line of source code.
		String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = *sourceline;
		int start = message->GetStartColumn();
		int end = message->GetEndColumn();


		fprintf(stderr, "%s\n", sourceline_string);

		for (int i = 0; i < start; i++) {
			fputc((sourceline_string[i] == '\t') ? '\t' : ' ', stderr);
		}
		for (int i = start; i < end; i++) {
			fputc('^', stderr);
		}
		fputc('\n', stderr);
	}
}

static void
ReportException(TryCatch &try_catch, bool show_line)
{
	HandleScope scope;

	if (show_line) DisplayExceptionLine(try_catch);

	String::Utf8Value trace(try_catch.StackTrace());

	// range errors have a trace member set to undefined
	if (trace.length() > 0 && !try_catch.StackTrace()->IsUndefined()) {
		fprintf(stderr, "%s\n", *trace);
	} else {
		// this really only happens for RangeErrors, since they're the only
		// kind that won't have all this info in the trace, or when non-Error
		// objects are thrown manually.
		Local<Value> er = try_catch.Exception();
		bool isErrorObject = er->IsObject() &&
				!(er->ToObject()->Get(String::New("message"))->IsUndefined()) &&
				!(er->ToObject()->Get(String::New("name"))->IsUndefined());

		if (isErrorObject) {
			String::Utf8Value name(er->ToObject()->Get(String::New("name")));
			fprintf(stderr, "%s: ", *name);
		}

		String::Utf8Value msg(!isErrorObject ? er
				: er->ToObject()->Get(String::New("message")));
		fprintf(stderr, "%s\n", *msg);
	}

	fflush(stderr);
}
void
jsv8_t::_link(void *env,rainRoutine exit,int code)
{
	jsv8_t * js = (jsv8_t *)env;
	js->Link(exit,code);
}
void
jsv8_t::_recv(void *env,rainRoutine src,struct rainMsg msg,rainSession session)
{
	jsv8_t * js = (jsv8_t *)env;
	js->Recv(src,msg,session);
}
void
jsv8_t::_recv_rsp(void *env,rainRoutine src,struct rainMsg msg,rainSession session)
{
	jsv8_t * js = (jsv8_t *)env;
	js->RecvResponce(src,msg,session);
}



bool
ReadFile(const std::string & str,std::string &out)
{
	FILE* file = fopen(str.c_str(), "rb");
	if (file == NULL) return false;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i = 0; i < size;) {
		int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
		i += read;
	}
	fclose(file);
	out = string(chars,size);
	delete[] chars;
	return true;
}
Handle<Value>
require(const v8::Arguments & args)
{
	HandleScope scope;
	if (args.Length() != 1) {
		v8::ThrowException(v8::String::New("Bad parameters"));
	}
	v8::String::Utf8Value file(args[0]);
	if (*file == NULL) {
		v8::ThrowException(v8::String::New("Error loading file"));
	}
	std::string source;
	if (!ReadFile(*file,source)) {
		v8::ThrowException(v8::String::New("Error loading file"));
	}
	TryCatch try_catch;
	source = "(function(exports,__filename,__dirname){"+source+"})";
	v8::Handle<Script> script =
			v8::Script::Compile(String::New(source.c_str(),source.length()),v8::String::New(*file));
	if (try_catch.HasCaught())  {
		ReportException(try_catch,true);
	}
	v8::Local<Value> f_value = script->Run();
	if (try_catch.HasCaught())  {
		ReportException(try_catch,true);
	}
	Local<Function> f = Local<Function>::Cast(f_value);
	Local<Value> exports = Object::New();
	f->Call(v8::Context::GetCurrent()->Global(),1,&exports);
	if (try_catch.HasCaught())  {
		ReportException(try_catch,true);
	}
	return scope.Close(exports);
}
v8::Handle<v8::Value>
Print(const v8::Arguments& args)
{
	bool first = true;
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope;
		if (first) {
			first = false;
		} else {
			printf(" ");
		}
		v8::String::Utf8Value str(args[i]);
		std::cout << *str;
	}
	std::cout<<std::endl;
	fflush(stdout);
	return v8::Undefined();
}

jsv8_t::jsv8_t()
{
	this->ctx_ = NULL;
	this->solt_ = NULL;
	this->bStart = false;
	this->timeid_ = 0;
}
void
jsv8_t::Exit(int code)
{
	if(!this->exit_.IsEmpty()){
		Locker lock(this->solt_);
		Isolate::Scope isocpe(this->solt_);
		HandleScope socpe;
		Context::Scope c_scope(this->v8ctx_);
		const int num_arg = 1;
		Handle<Value> argc[num_arg];
		argc[0] = Integer::New(code);
		this->exit_->Call(this->v8ctx_->Global(),num_arg,argc);
	}
}
jsv8_t::~jsv8_t()
{
	if(!this->exit_.IsEmpty()){
		this->exit_.Dispose(this->solt_);
	}
	if(!this->recv_.IsEmpty()){
		this->recv_.Dispose(this->solt_);
	}
	if(!this->exit_.IsEmpty()){
		this->exit_.Dispose(this->solt_);
	}
	if(!this->v8ctx_.IsEmpty()){
		this->v8ctx_.Dispose(this->solt_);
	}
	if(this->solt_){
		this->solt_->Dispose();
	}

}

v8::Handle<v8::Object>
jsv8_t::SteupRoutine(const std::string & args)
{
	v8::HandleScope scope;
	v8::Handle<v8::ObjectTemplate> foo_templ ;
	v8::Handle<v8::Object> foo_class_obj ;
	v8::Handle<v8::External> foo_class_ptr ;

	foo_templ = v8::ObjectTemplate::New();
	foo_templ->SetInternalFieldCount(1);
	foo_class_obj = foo_templ->NewInstance();

	foo_class_ptr = v8::External::New(this);
	foo_class_obj->SetInternalField(0, foo_class_ptr) ;

	foo_class_obj->Set(v8::String::New("On"),
			v8::FunctionTemplate::New(jsv8_t::On)->GetFunction()) ;
	foo_class_obj->Set(v8::String::New("Spawn"),
			v8::FunctionTemplate::New(jsv8_t::Spawn)->GetFunction()) ;
	foo_class_obj->Set(v8::String::New("Exit"),
			v8::FunctionTemplate::New(jsv8_t::Exit)->GetFunction()) ;
	foo_class_obj->Set(v8::String::New("Timer"),
			v8::FunctionTemplate::New(jsv8_t::Timer)->GetFunction()) ;
	foo_class_obj->Set(v8::String::New("version"),
			String::New(RAIN_VERSION)) ;
	foo_class_obj->Set(String::NewSymbol("platform"),
			String::New("linux"));
	rainRoutine rid = rainRoutineId(this->ctx_);
	foo_class_obj->Set(String::NewSymbol("rid"),
			Integer::New(rid));
	foo_class_obj->Set(String::NewSymbol("parent"),
			this->parent_);
	return foo_class_obj;
}


bool
jsv8_t::Initialize(struct rainContext *ctx,const std::string & args)
{
	this->ctx_ = ctx;
	{
		this->solt_ = Isolate::New();
		Locker lock(this->solt_);
		Isolate::Scope icsope(this->solt_);
		HandleScope scope;

		Handle<ObjectTemplate> global = ObjectTemplate::New();
		global->Set(String::New("require"), FunctionTemplate::New(require));
		global->Set(String::New("print"), FunctionTemplate::New(Print));

		this->v8ctx_ = Context::New(NULL,global);
		Context::Scope c_scope(this->v8ctx_);
		TryCatch try_catch;
		
		std::string source;
		if(!ReadFile(args,source)){
			return false;
		}

		source = "(function(routine){"+source+"})";
		Handle<Script> script = Script::Compile(
				String::New(source.c_str(),source.length()),
				String::New(args.c_str(),args.length())
		);
		Handle<Value> f_value = script->Run();
		if (try_catch.HasCaught())  {
			ReportException(try_catch,true);
			return false;
		}
		this->start_ = Persistent<Function>::New(Handle<Function>::Cast(f_value));
	}
	RAIN_CALLBACK(ctx,jsv8_t::_recv,jsv8_t::_recv_rsp,jsv8_t::_link,jsv8_t::_time_out,jsv8_t::_next_tick);
	rainNextTick(ctx,NULL);
	return true;
}
int
jsv8_t::Run()
{
	Locker lock(this->solt_);
	Isolate::Scope isocpe(this->solt_);
	HandleScope socpe;
	Context::Scope c_scope(this->v8ctx_);
	bStart = true;
	rainRoutine parent =rainPRoutineId(this->ctx_);
	jsv8_ctx_t * p = new jsv8_ctx_t(this->ctx_,parent);
	this->relates_[parent] = p;
	this->parent_  =  Persistent<Value>::New( jsv8_ctx_t::Register(p));
	this->routine_ =  Persistent<Value>::New(this->SteupRoutine(""));

	TryCatch try_catch;
	Handle<Value> func_args[1];
	func_args[0] = this->routine_;

	this->start_->Call(this->v8ctx_->Global(),1,func_args);
	if (try_catch.HasCaught())  {
		ReportException(try_catch,true);
		return RAIN_ERROR;
	}
	if(bStart){
		return RAIN_OK;
	}
	return RAIN_ERROR;
}
void
jsv8_t::_next_tick(void *arg,void *userdata)
{
	jsv8_t * js = (jsv8_t *)arg;
	if( js->Run() == RAIN_ERROR){
		rainExit(js->ctx_,1);
	}
}
void
jsv8_t::_time_out(void *arg,void *userdata)
{
	jsv8_t * js = (jsv8_t *)arg;
	uint32_t id = (ptrdiff_t)(userdata);
	jsv8_t::TimerMapIter iter = js->timers_.find(id);
	if(iter != js->timers_.end()){
		{
			Locker lock(js->solt_);
			Isolate::Scope isocpe(js->solt_);
			HandleScope socpe;
			Context::Scope c_scope(js->v8ctx_);
			iter->second.cb_->Call(js->v8ctx_->Global(),0,NULL);
		}
		if(iter->second.repeat_ == -1){
			rainTimeout(js->ctx_,iter->second.times_,(void *)(ptrdiff_t)(id));
			return;
		}
		if(--iter->second.repeat_ >= 0){
			rainTimeout(js->ctx_,iter->second.times_,(void *)(ptrdiff_t)(id));
		}else{
			iter->second.cb_.Dispose(js->solt_);
			js->timers_.erase(iter);
		}

	}
}
void
jsv8_t::Recv(rainRoutine src,struct rainMsg msg,rainSession session)
{
	Locker lock(this->solt_);
	Isolate::Scope isocpe(this->solt_);
	HandleScope socpe;
	Context::Scope c_scope(this->v8ctx_);
	if( this->relates_.find(src) != this->relates_.end()){
		jsv8_ctx_t* jsct =  this->relates_[src];
		if(!jsct->recv_cb.IsEmpty()){
			const int num_arg = 1;
			Handle<Value> argc[num_arg];
			Handle<Object> obj = Object::New();
			obj->Set(v8::String::New("data"),String::New(static_cast<char *>(msg.data),msg.sz));
			obj->Set(v8::String::New("type"),Integer::New(msg.type));
			if(session != RAIN_INVALID_SESSION){
				obj->Set(v8::String::New("session"),Integer::New(session));
			}
			argc[0] = obj;
			jsct->recv_cb->Call(this->v8ctx_->Global(),num_arg,argc);
		}
	}else{
		if(src == rainPRoutineId(this->ctx_)){
			jsv8_ctx_t * p = new jsv8_ctx_t(this->ctx_,src);
			this->relates_[src] = p;
			const int num_arg = 2;
			Handle<Value> argc[num_arg];
			argc[0] = jsv8_ctx_t::Register(p);
			Handle<Object> obj = Object::New();
			obj->Set(v8::String::New("data"),String::New(static_cast<char *>(msg.data),msg.sz));
			obj->Set(v8::String::New("type"),Integer::New(msg.type));
			if(session != RAIN_INVALID_SESSION){
				obj->Set(v8::String::New("session"),Integer::New(session));
			}
			argc[1]=obj;
			this->recv_->Call(this->v8ctx_->Global(),num_arg,argc);
		}
	}
	free(msg.data);
}
void
jsv8_t::Link(rainRoutine exit,int code)
{
	jsv8_t::RelationMapIter iter = this->relates_.find(exit);
	if( iter  != this->relates_.end()){
		jsv8_ctx_t* jsct =  this->relates_[exit];
		Locker lock(this->solt_);
		Isolate::Scope isocpe(this->solt_);
		HandleScope socpe;
		Context::Scope c_scope(this->v8ctx_);
		if(!jsct->exit_cb.IsEmpty()){
			const int num_arg = 1;
			Handle<Value> argc[num_arg];
			argc[0] = Integer::New(code);
			jsct->exit_cb->Call(this->v8ctx_->Global(),num_arg,argc);
		}
		delete jsct;
		this->relates_.erase(iter);
	}
}
void
jsv8_t::RecvResponce(rainRoutine src,struct rainMsg msg,rainSession session)
{
	if( this->relates_.find(src) != this->relates_.end()){
		jsv8_ctx_t* jsct =  this->relates_[src];
		if(jsct->func_map_.find(session) != jsct->func_map_.end()){
			v8::Persistent<v8::Function> func = jsct->func_map_[session];
			Locker lock(this->solt_);
			Isolate::Scope isocpe(this->solt_);
			HandleScope socpe;
			Context::Scope c_scope(this->v8ctx_);

			const int num_arg = 1;
			Handle<Value> argc[num_arg];
			Handle<Object> obj = Object::New();
			obj->Set(v8::String::New("data"),String::New(static_cast<char *>(msg.data),msg.sz));
			obj->Set(v8::String::New("type"),Integer::New(msg.type));
			argc[0] = obj;
			func->Call(this->v8ctx_->Global(),num_arg,argc);
		}
	}
	free(msg.data);
}
v8::Handle<v8::Value>
jsv8_t::Spawn(const v8::Arguments& args)
{
	if(args.Length() < 2){
		v8::ThrowException(v8::String::New("arguments is too length"));
	}
	if(!args[0]->IsString()){
		v8::ThrowException(v8::String::New("arg_one need string"));
	}
	if(!args[1]->IsString()){
		v8::ThrowException(v8::String::New("arg_two need object"));
	}
	v8::HandleScope scope;
	v8::String::Utf8Value file(args[0]);
	v8::String::Utf8Value rain_arg(args[1]);
	rainRoutine rd;
	jsv8_t * js = get_cppobj_ptr<jsv8_t>(args.Holder());
	if( rainSpawn(js->ctx_,*file,*rain_arg,&rd) ==RAIN_OK){
		jsv8_ctx_t * p = new jsv8_ctx_t(js->ctx_,rd);
		js->relates_[rd] = p;
		return scope.Close(jsv8_ctx_t::Register(p));
	}
	return v8::Undefined();
}
v8::Handle<v8::Value>
jsv8_t::On(const v8::Arguments& args)
{
	HandleScope scope;
	if(args.Length()<2){
		ThrowException(String::New("argent need 2"));
		return v8::Boolean::New(false);
	}
	if(!args[0]->IsString()){
		ThrowException(String::New("argument:0 need string"));
		return scope.Close(v8::Boolean::New(false));
	}
	String::Utf8Value arg1(args[0]);
	if(!args[1]->IsFunction()){
		ThrowException(String::New("arg 1:need a function"));
		return v8::Boolean::New(false);
	}
	jsv8_t * js = get_cppobj_ptr<jsv8_t>(args.Holder());
	if(strcmp(*arg1,"message") == 0){
		js->recv_ =
				v8::Persistent<Function>::New(v8::Handle<Function>::Cast(args[1]));
		return scope.Close(v8::Boolean::New(true));
	}else if(strcmp(*arg1,"exit") == 0){
		js->exit_ =
				v8::Persistent<Function>::New(v8::Handle<Function>::Cast(args[1]));
		return scope.Close(v8::Boolean::New(true));
	}else{
		ThrowException(String::New("argument 0:need a String of message or exit"));
		return v8::Boolean::New(false);
	}
}
v8::Handle<v8::Value>
jsv8_t::Exit(const v8::Arguments& args)
{
	HandleScope scope;
	jsv8_t * js = get_cppobj_ptr<jsv8_t>(args.Holder());
	int code = 0;
	if(args.Length() >= 1){
		code = v8::Handle<Integer>::Cast(args[0])->Value();
	}
	rainExit(js->ctx_,code);
	return v8::Undefined();
}
v8::Handle<v8::Value>
jsv8_t::Timer(const v8::Arguments& args)
{
	HandleScope scope;
	jsv8_t * js = get_cppobj_ptr<jsv8_t>(args.Holder());
	jsv8_t::timer timr;
	int id = js->timeid_++;
	if(args.Length() >= 3){
		timr.repeat_ = v8::Handle<Integer>::Cast(args[2])->Value();
	}else{
		timr.repeat_ = -1;
	}
	timr.times_ = v8::Handle<Number>::Cast(args[1])->Value();
	timr.cb_ =v8::Persistent<Function>::New(v8::Handle<Function>::Cast(args[0]));
	js->timers_[id]=timr;
	rainTimeout(js->ctx_,timr.times_,(void *)(ptrdiff_t)(id));
	return scope.Close(Integer::New(id));
}
/*v8::Handle<v8::Value> jsv8_t::ClearTimeout(const v8::Arguments& args){
	HandleScope scope;
	jsv8_t * js = get_cppobj_ptr<jsv8_t>(args.Holder());
	jsv8_t::timer timr;
	timr.id = js->timeid_++;
	timr.cb_ =v8::Persistent<Function>::New(v8::Handle<Function>::Cast(args[0]));
	js->timers_[timr.id]=timr;
	rain_timeout(js->ctx_,1.0,(void *)(ptrdiff_t)(timr.id));
	return scope.Close(Integer::New(timr.id));
}*/
}
