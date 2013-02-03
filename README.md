rain
====

面向行为的并发模型（使用erlang的并发模型）
所有的并发进程 都是以消息进行传递。
(1)以组件的形式提供服务。
	实现是以动态库。
	提供2个外部接口。
	modname_init modname_destroy 
	(modname 为组件的名字，入libtest.so modename 为 test 接口为test_init ,test_destroy)
	以下为函数的原型。（rain-src/include/rain_type.h)
	typedef void *(*rain_init_fn)(struct rain_ctx_s*ctx,const char *args);
	typedef void(*rain_destroy_fn)(void *env,int code);
(2)服务---通过消息进行通信。
	通过rain-src/include/rain.h
	int rain_set_recvfn(rain_ctx_t *ctx,rain_recv_fn req);
	int rain_set_recvrspfn(rain_ctx_t *ctx, rain_recv_fn rsp);
	int rain_set_linkfn(rain_ctx_t *ctx,rain_link_fn linkfn);
	int rain_set_timeoutfn(rain_ctx_t *ctx,rain_timerout_fn timeoutfn);
	int rain_set_next_tickfun(rain_ctx_t *ctx,rain_timerout_fn next_tickfn);
	注册关注的事件。
（3）rain进程（服务）
	启动另一个rain进程（rain_spawn ）。
	可以关注服务退出（rain_link）。
	可以杀死某个服务（rain_kill）
	rain_send//给某个服务发消息
	rain_responce//给某个请求消息发送回应。
	rain_next_tick//rain_timeout(0)
	rain_timeout//注册超时事件。
（4）例子。
具体的例子可以看routine-src 中实现的几个服务。
js-v8服务提供脚本化的能力。JS写的脚本在routine_js中。