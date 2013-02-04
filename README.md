rain
====

面向行为的并发模型（使用erlang的并发模型）
所有的并发进程 都是以消息进行传递。
(1)以组件的形式提供服务。
	实现是以动态库。
	提供2个外部接口。
	modnameNew modnameDelete 
	(modname 为组件的名字，入libtest.so modename 为 test 接口为testNew ,testDelete)
	以下为函数的原型。（rain-src/include/rain_type.h)
	typedef void *(*rainNewFn)(struct rainContext*ctx,const char *args);
	typedef void(*rainDeleteFn)(void *env,int code);
(2)服务---通过消息进行通信。
	通过rain-src/include/rain.h
	int rainSetRecvfn(rainContext *ctx,rainRecvFn req);
	int rainSetRspFn(rainContext *ctx, rainRecvFn rsp);
	int rainSetLinkFn(rainContext *ctx,rainLinkFn linkfn);
	int rainSetTimeoutFn(rainContext *ctx,rainTiemoutFn timeoutfn);
	int rainSetNextTickFn(rainContext *ctx,rainTiemoutFn next_tickfn);
	注册关注的事件。
（3）rain进程（服务）
	启动另一个rain进程（rainSpawn ）。
	可以关注服务退出（rainLink）。
	可以杀死某个服务（rainkill）
	rainSend//给某个服务发消息
	rainResponce//给某个请求消息发送回应。
	rainNextTick//rainTimeout(0)
	rainTimeout//注册超时事件。
（4）例子。
具体的例子可以看routine-src 中实现的几个服务。
js-v8服务提供脚本化的能力。JS写的脚本在routine_js中。