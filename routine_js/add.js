
function main()
{
	routine.On("message",function (src,msg){
		print("sdf");
	});
	routine.On("exit",function (code){
		print("[ADD]-SELF-EXIT",code);
	});
	routine.parent.On("message",function (msg){
		print('[ADD]-PARENT-MESSAGE',msg);
		var data = JSON.parse(msg.data);
		var ret = data.a+data.b;
		if(msg.session){
			routine.parent.Responce({data:ret,type:1},msg.session);
		}
	});
	routine.parent.On("exit",function(code){
		print("[ADD]-PARENT-EXIT",code);
	});
	//startTcpSvr("127.0.0.1",8194,'epoll',routine.rid,2);
}
main();


function startTcpSvr(ip,port,mode,watchdog,headsz){
	const TCP_MSG_TYPE={
		CONNECT:0X01,
		MESSAGE:0X02,
		CLOSE:0X03,
	};
	//mode = 'select' 'epoll';
	var size = 0;
	var num_pkt = 0;
	var tcpSvr = routine.Spawn("tcpsvr","mode="+mode+"&ip="+ip+"&port="+port+"&watchdog="+watchdog);
	if(tcpSvr){
		var num=0;
		tcpSvr.On("message",function(msg){
			if(msg.type == TCP_MSG_TYPE.CONNECT){
				print("tcpSvr-Msg",port,"new_connect",++num);
			}else if(msg.type == TCP_MSG_TYPE.MESSAGE){
				print("tcpSvr-Msg",msg,msg.data.length-4,num_pkt++);
				var str = msg.data;
				size+=msg.data.length-4;
				num_pkt++
			}else if(msg.type == TCP_MSG_TYPE.CLOSE){
				print("tcpSvr-Msg",port,"connect_close",--num);	
				print("tcpSvr-ALL-INFO",num_pkt,size);
			}else{
				print("unknow info");
			}
		});
		tcpSvr.On("exit",function(code){
			print("tcpSvr,exit",code);
		 });
		var i = 0;
	}else{
		print("tcp-svr-launch failed");
		throw new Error("tcp-svr-lanuch-failed");
	}
}
