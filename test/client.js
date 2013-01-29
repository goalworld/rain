var net = require('net');
var buf = require("buffer");
var tcpClient = require("./tcpsvr/tcp_client");
console.log(process.argv);
var port = 6192;
if(process.argv[3])
 port = Number(process.argv[3]);
var avg = Number(process.argv[4]);
if(!avg){
	avg = 1;
}
var arr=[];
for(var i=0;i<1000/avg;i++){
	arr[i] = 1;//(Math.random()*100);
}
var data={
	arr:arr,
	cmd:"CMD_HELLO",
}
//var data = "hello";
for(var i=0;i<Number(process.argv[2])*avg;i++){
	(function(){
		 var tcpcli = tcpClient.CreateTcpClient("127.0.0.1",process.argv[3],2);
		tcpcli.on("open",function(){
			console.log("open");
			//tcpcli.Send(data);
			setInterval(function(){
				tcpcli.Send(data);
			},100);
		});
		tcpcli.on("connect_fail",function(err){
			console.log(err);
		});
		tcpcli.on("close",function(){
			console.log("close");
		});

	})();
}
id = setInterval(function(){
			console.log(num*str.length);
			process.exit(0);
		},100000);
process.on('uncaughtException', function (err) {
  console.log('Caught exception: ' ,err,err.stack);
});
process.on('exit',function(){
console.log(num);
});
