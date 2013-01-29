{
	"target_defaults":{
		"ldflags":["-Wl,-E -pg "],
		"cflags":["-fPIC -Wall -g3"],
	},
  	'targets': [
  		{
			"target_name":"ev",
			"type":"static_library",
			"includes":["./libev-4.11/ev.gypi",],
		},
  	]
}