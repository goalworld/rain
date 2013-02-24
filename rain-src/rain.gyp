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
				"src/rain_context.c",
				"src/rain_imp.c",
				"src/rain_lifequeue.c",
				"src/rain_msgqueue.c",
				"src/rain_module.c",
				"src/rain_start.c",
				"src/rain_rpc.c",
				"src/rain_timer.c",
				"src/rain_loger.c"
			],
			"libraries":['-ldl','-lpthread',"-lm","-lwod"],
		},
	]
}
