{
	'target_defaults':{
		"cflags":["-pg -Wall -g3 -std=c99"],
	},
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        '../rain-src/rain.gyp:*',
       	"../routine-src/routine.gyp:*"
      ],
    },
    {
      'target_name': 'clean',
      "type":"none",
    }
  ]
}
