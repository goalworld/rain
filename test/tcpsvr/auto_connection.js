/**@IMonitoring
 * IMonitoring.prototype.UpdatePing
 * IMonitoring.prototype.AddUpwardFlow
 * IMonitoring.prototype.AddDwonwardFlow
 * IMonitoring.prototype.AddConnectId
 * IMonitoring.prototype.RemoveConnectId
 * IMonitoring.prototype.UpdateIpNum
 * IMonitoring.prototype.UpdatePing
 * IMonitoring.prototype.GetExtMsg
 */
var tcpClient = require ( "./tcp_client.js");
var util = require("util");
var buf = require("buffer");
var events = require("events");
exports.OTO = 1;
exports.TIME =2;
exports.AutoConnection = AutoConnection;
/**
 * ID=connect id;
 * option={
 * 	retry_times:
 * 	ping_type:
 * 	id:
 * }
 * IMonitor = $@IMonitoring
 */
function AutoConnection(option,IMonitor)
{
	if(!option.time_out){
		option.time_out = 5000;
	}
	if(option.ping_type == 1){
		this.m_pingInst = new NAMESPACE_PING.OTOPing(this, this, option.time_out);
	}else if(option.ping_type == 2){
		this.m_pingInst = new NAMESPACE_PING.TimerPing(this, this, option.time_out);
	}
	if(option.id){
		this.m_id = option.id;
	}else{
		this.m_id = new Date().getTime();
	}
	if(option.headsz){
		this.m_headsz = option.headsz;
	}
	this.m_monitor = IMonitor;//监控类
	if(this.m_monitor){
		this.m_monitor.AddConnectId(this.m_id);
	}
	if(option.retry_times){
		this.m_retryTimes = option.retry_times;//重连次数
	}else{
		this.m_retryTimes = -1;
	}
	this.m_tmpRty = this.m_retryTimes;
	//connect
	this.m_conn = null;
	this.m_connState = 0;//0:未连接或者关闭了，1：正在连接，2：已经连接
	//timer  
	//重连timer;
	this.m_reconTimer = null;
	// ping keep client
	//connecting
	this.m_brelogin = false;
	if(option.ip && option.port){
		this.m_ip = option.ip;
		this.m_port = option.port;
		this._Start();
	}

	this.m_retryIndex = 0;

	this.m_cacheQueue = [];

}
util.inherits(AutoConnection, events.EventEmitter);
//监听 ping 丢失
AutoConnection.prototype.Lose = function(num) {
	if(num == 5){
		this._DoClose(false);
	}
};
//监听 ping 成功
AutoConnection.prototype.Sucess = function(ping,avgping)
{
	if(this.m_monitor)
		this.m_monitor.UpdatePing(this.m_id,ping);
};
AutoConnection.prototype.__defineGetter__("Ping",function(){
	return this.m_pingInst.GetPing();
});
AutoConnection.prototype.__defineGetter__("AvgPing",function(){
	return this.m_pingInst.GetAvgPing();
});
AutoConnection.prototype.__defineGetter__("State",function(){
	return this.m_connState;
});
//获取连接ID
AutoConnection.prototype.GetID = function ()
{
	return this.m_id;
};
//获取扩展的信息。
AutoConnection.prototype.GetExtMsg = function ()
{
	if(this.m_monitor && this.m_monitor.GetExtMsg){
		return this.m_monitor.GetExtMsg();
	}
};
AutoConnection.prototype.Open = function (ip,port,brelogin){
	if(this.m_connState != 0){
		var arr = ["connectind","connected"];
		throw new Error("Open_faile connection is"+arr[this.m_connState-1]);
		return ;
	}
	console.log(ip,port,brelogin);
	if(ip){
		this.m_ip = ip;
	}
	if(port){
		this.m_port = port;
	}
	if(!this.m_ip || !this.m_port){
		return ;
	}
	this.m_brelogin = brelogin;
	this._Start(brelogin);
};
AutoConnection.prototype.StartPing = function (precb){
	if(this.m_pingInst){
		this.m_pingInst.BeginPing(precb);
	}
};
//发送报文
AutoConnection.prototype.Send = function (data)
{
	try{
		if( this.m_connState == 2 ){
			if(this.m_monitor){
				var pktStr  = JSON.stringify(data);
				this.m_monitor.AddUpwardFlow(this.m_id,Buffer.byteLength(pktStr));
			}
			this.m_conn.Send(data);
		}else{
			this.m_cacheQueue.push(data);
		}

	}catch(err){
		console.log(err,err.stack);
	}
};
//主动关闭连接
AutoConnection.prototype.Close = function (){
	this.m_tmpRty = this.m_retryTimes;
	if(this.m_reconTimer)
		clearTimeout(this.m_reconTimer);
	this.m_reconTimer = null;
	if(this.m_conn){
		this.m_conn.removeAllListeners('connect_fail');
		this.m_conn.removeAllListeners('open');
		this.m_conn.removeAllListeners('close');
		this.m_conn.removeAllListeners('message');
		this.m_conn.Close();
		this.m_conn = null;
	}
	this.m_connState = 0;
	if(this.m_pingInst){
		this.m_pingInst.ReSet();
	}
	this.m_retryIndex = 0;
	this.m_cacheQueue = [];
};
//开始连接 ， 如果超出重练次数 就关闭；
AutoConnection.prototype._Start = function (  )
{
	if(this.m_retryTimes != -1){
		this.m_tmpRty--;
		if(this.m_tmpRty <0){
			this.Close();
			return ;
		}
	}
	if(this.m_connState == 0){
		var index = this.m_retryIndex++;
		this.m_connState = 1;
		console.log("[CONNECT_SVR]",this.m_id,(!!this.m_brelogin),"[BEGIN]",index,this.m_ip,this.m_port);
		var self = this;
		this.m_conn = tcpClient.CreateTcpClient(this.m_ip,this.m_port,this.m_headsz);
		this.m_conn.on("connect_fail",function (err){
			console.log("[CONNECT_SVR]",self.m_id,"[FAILED]",index,err);
			self.emit("open_fail",self.m_brelogin,err);
			self.m_connState = 0;
			self._DoClose(true);
		});
		this.m_conn.on("open",function(){
			console.log("[CONNECT_SVR]",self.m_id,"[SUCCESS]",index);
			self.m_connState = 2;
			self.emit("open",self.m_brelogin);
			self.m_brelogin = true;
			self._DoOpen();
		});
		this.m_conn.on("close",function(){
			console.log("[CONNECT_SVR]",self.m_id,"[CLOSED]",index);
			self.m_connState = 0;
			self.emit("close");
			self._DoClose(false);
		});
		this.m_conn.on("message",function(recvBuf,sz){
			if(self.m_monitor)
				self.m_monitor.AddDownwardFlow(self.m_id,sz);
			if(sz != recvBuf.length){
				console.log("unknow sz",sz,recvBuf.length);
				process.exit(412);
			}
			var str = recvBuf.toString('utf8', 0, sz);
			try{
				var data = JSON.parse(str);
				if(data.cmd == "CMD_ECHO"){
					self.m_pingInst.RecvEcho(data);
				}else {
					self.emit("message",data);
				}
			}catch(err){
				console.log(err,err.stack,str,recvBuf.length,recvBuf,buf.Buffer.byteLength(str));
			}
		});
	};
};
//收到 服务连接成功
AutoConnection.prototype._DoOpen = function (){
	for(var i=0; i<this.m_cacheQueue.length;i++){
		this.Send(this.m_cacheQueue[i]);
	}
	this.m_cacheQueue = [];
};
//收到 与服务器断开
AutoConnection.prototype._DoClose = function (bConnectFail){	
	if(this.m_conn){
		this.m_conn.removeAllListeners('connect_fail');
		this.m_conn.removeAllListeners('open');
		this.m_conn.removeAllListeners('close');
		this.m_conn.removeAllListeners('message');
		this.m_conn.Close();
		this.m_conn = null;
	}
	this.m_connState = 0;
	if(this.m_pingInst){
		this.m_pingInst.EndPing();
	}
	if(this.m_reconTimer)
		clearTimeout( this.m_reconTimer );
	var self = this;
	if(bConnectFail){
		this.m_reconTimer = setTimeout( function(){
			self.m_reconTimer = null;
			self._Start();
		},3000);
	}else{
		process.nextTick(function (){
			self._Start();
		});
	}
};

