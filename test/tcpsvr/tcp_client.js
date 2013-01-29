
var net = require("net");
var util = require("util");
var events = require("events");

var tcpWriter = require("./tcp_writer");
var tcpReader = require("./tcp_reader");

exports.CreateTcpClient = function (host,port,headsz){
	if(!host || !port){
		throw new Error("CreateTcpServer->arguments error");
	}
	if(!headsz){
		headsz = 4;
	}
	return new TcpClient(host,port,headsz);
};
const state={
		connecting:0,
		open:1,
		close:2,
};
function TcpClient(host,port,headsz){

	this.m_headsz = headsz;
	this.m_state = state.connecting;
	this.m_socket = new net.Socket();
	this.m_writer = undefined;
	this.m_bWrite = false;
	var self = this;
	var timer = setTimeout(function(){
		self.m_socket.emit('error',new Error("timeout"));
		self.m_socket.destroy();
	},10000);
	this.m_socket.on("error",function(err){
		clearTimeout(timer);
		if(self.m_state == state.connecting){
			self.m_state = state.close;
			try{
				self.emit("connect_fail",err);
			}catch(err){
				console.log(err,err.stack);
			}
			try{
				self.m_socket.destroy();
			}catch(err){
				console.log(err,err.stack);
			};
		};
	});
	this.m_socket.connect(port,host,function (){
		clearTimeout(timer);
		new tcpReader.TcpReader(self.m_socket,headsz,function (buf,sz){
			try{
				self.emit("message",buf,sz);
			}catch(err){
				console.log(err,err.stack);
			};
		});
		self.m_writer = new tcpWriter.TcpWriter(self.m_socket,headsz,function (ok){
			try{
				if(self.m_state == state.close){
					self.m_socket.end();
					self.m_socket.destroy();
				}
				self.m_bWrite = false;
			}catch(err){
				console.log(err);
			};
		});
		self.m_state = state.open;
		self.emit("open");

	});
	this.m_socket.on("end",function(){
		if(self.m_state == state.open){
			self.m_state = state.close;
			self.emit("close");
		}
	});
	this.m_socket.on("close",function(){
		if(self.m_state == state.open){
			self.m_state = state.close;
			self.emit("close");
		}
	});
	
}
util.inherits(TcpClient, events.EventEmitter);
TcpClient.prototype.__defineGetter__('HeadSize', function() {
	return this.m_headsz;
});
TcpClient.prototype.Send = function (data){
	if(this.m_state == state.open){
		this.m_writer.Write(data);
	}else{
		console.log("连接未开启");
	}
};
TcpClient.prototype.DirectSend = function (data,cb){
	if(this.m_state == state.open){
		this.m_writer.DirectWrite(data,cb);
	}else{
		if(cb){
			cb(new Error("unOpen"));
		}
	}
};
TcpClient.prototype.Close = function (data){
	try{

		if(this.m_state == state.open){
			if(!this.m_bWrite){
				this.m_socket.destroy();
			}
			this.m_state = state.close;
		}
	}catch(err){
		console.log(err,err.stack);
	};
	this.m_state = state.close;
};
