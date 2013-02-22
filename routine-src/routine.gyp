{
	"target_defaults":{
		"ldflags":["-Wl,-E -pg "],
		"cflags":["-fPIC -Wall -g3"],
	},
  	'targets': [
  		{
			"target_name":"test",
			"type":"shared_library",
			"includes":["./test_routine/test.gypi",],
		},
		{
			"target_name":"tcpsvr",
			"type":"shared_library",
			"includes":["./tcpsvr_routine/tcpsvr.gypi",],
			"libraries":["-lwod"]
		},
		{
			"target_name":"jsv8",
			"type":"shared_library",
			"includes":["./jsv8_routine/jsv8.gypi",],
			"libraries":["-lv8"]
		},
  	]
}
