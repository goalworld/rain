var tcpsvr = require('./tcpsvr/tcp_server');
var  cliList=[];
for(var i=0;i<4;i++){
//	cliList[i]  = require("child_process").fork("./child.js",[],{ stdio: ['ipc', 'ipc', 'ipc', 'ipc', 'pipe'] });
}
var num =0;
var svr = tcpsvr.CreateTcpServer("0.0.0.0",8193,2);
svr.on("client_connect",function(id,info){

});
svr.on("client_message",function(id,buf){
	//console.log("cld",buf,num++);
	var str = buf.toString('utf8');
	num+=buf.length;
});
setInterval(function(){
	if(num>0){
		console.log(num/(1024*1024),"Mb");
		num = 0;
	}
},1000*5);
console.log(process.pid);
