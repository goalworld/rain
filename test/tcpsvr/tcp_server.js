
var net = require("net");
var util = require("util");
var tcpWriter = require("./tcp_writer");
var tcpReader = require("./tcp_reader");
var events = require("events");
exports.CreateTcpServer = function (host,port,headsz){
	if(!host || !port){
		throw new Error("CreateTcpServer->arguments error");
	}
	if(!headsz){
		headsz = 4;
	}
	if(headsz != 4 && headsz!= 2){
		throw new Error("CreateTcpServer->arguments error"+headsz);
	}
	return new TcpServer(host,port,Number(headsz));
};
function TcpServer(host,port,headsz){
	this.m_con_map = {};
	this.m_id = 1;
	this.m_headsz = headsz;
	var self = this;
	this.m_numConnection = 0;
	this.m_server = net.createServer( function (socket) {
		self.m_numConnection++;
		var id = self.m_id++;
		new tcpReader.TcpReader(socket,headsz,function (buf,sz){
			try{
				if(self.m_con_map[id])
					self.emit("client_message",id,buf,sz);
			}catch(err){
				console.log(err,err.stack,buf,sz);
			}
		});
		self.m_con_map[id] = {bclose:false,bwriting:false,socket:socket};
		self.m_con_map[id].writer = new tcpWriter.TcpWriter(socket,headsz,function (ok){
			try{
				if(self.m_con_map[id]){
					if(self.m_con_map[id].close){
						socket.end();
						socket.destroy();
					}else{
						if(self.m_con_map[id].bwriting){
							self.m_con_map[id].bwriting = false;
						}
					}	
				}else{
					console.log("connect is close");
				}

			}catch(err){
				console.log(err);
			}

		});
		socket.on("error",function(){
			console.log("tcp - error");
		});
		socket.on("end",function(){
			if(self.m_con_map[id] && !self.m_con_map[id].bclose){
				self.m_con_map[id].bclose = true;
				self.emit("client_close",id);
				self._ClearConnect(id);
			}
		});
		socket.on("close",function(){
			try{
				if(self.m_con_map[id] && !self.m_con_map[id].bclose){
					self.m_con_map[id].bclose = true;
					self.emit("client_close",id);
					self._ClearConnect(id);
				}
			}catch(err){
				console.log(err,err.stack);
			}
			try{
				socket.destroy();
			}catch(err){
				console.log(err,err.stack);
			}
		});
		self.emit("client_connect",id,{ip:socket.remoteAddress,port:socket.remotePort});
	});
	this.m_server.listen(port,host);
	console.log("[TCP-SERVER AT:",host,":",port,"]");
};

util.inherits(TcpServer, events.EventEmitter);
TcpServer.prototype.__defineGetter__('HeadSize', function() {
	return this.m_headsz;
});
TcpServer.prototype.__defineGetter__('NumConnection', function() {
	return this.m_numConnection;
});

TcpServer.prototype.Broadcast = function (data){
	for(var id in this.m_con_map){
		this.Send(id,data);
	}
};
TcpServer.prototype.Send = function(id,data){
	if(!this.m_con_map[id]){
		return false;
	}
	try{
		this.m_con_map[id].bwriting = true;
		this.m_con_map[id].writer.Write(data);
	}catch(err){
		console.log(err,err.stack);
	};
};
TcpServer.prototype.DirectSend = function (id,data,cb){
	if(!this.m_con_map[id]){
		if(cb)cb(new Error("connect is not exist"));
		return;
	}
	this.m_con_map[id].bwriting = true;
	this.m_con_map[id].writer.DirectWrite(data,cb);

}
TcpServer.prototype.CloseConnect = function(id){
	if(!this.m_con_map[id]){
		return false;
	}else{
		if(this.m_con_map[id].bclose){
			return false;
		}
		this.m_con_map[id].bclose = true;
		if(!this.m_con_map[id].bwriting){
			try{
				this.m_con_map[id].socket.end();
				this.m_con_map[id].socket.destroy();

			}catch(err){
				console.log(err);
			}
		}
		this._ClearConnect(id);
	}
};
TcpServer.prototype._ClearConnect = function (id)
{
	if(this.m_con_map[id]){
		delete this.m_con_map[id];
		this.m_numConnection--;
	}
};
