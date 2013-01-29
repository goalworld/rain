{
	'target_defaults':{
		'define':['DEBUG'],
		"ldflags":['-Wl,-E -pg ',],
		"cflags":[' -Wall -g3 ',"-pg"]
	},
	'targets':[
		{
			"target_name":"rain",
			"type":"executable",
			'include_dirs': [
        		'src/',
        		'include/'
     		 ],
			"sources":[
				"src/rain_ctx.c",
				"src/rain_imp.c",
				"src/rain_lifequeue.c",
				"src/rain_module.c",
				"src/rain_queue.c",
				"src/rain_start.c",
				"src/rain_rpc.c",
				"src/rain_array.c",
				"src/rain_timer.c",
				"src/rain_loger.c",
				"src/rain_utils.c"
			],
			"libraries":['-ldl','-lpthread',"-lm"],
		},
	]
}
