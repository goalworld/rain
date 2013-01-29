
exports.TcpWriter = TcpWriter;
function TcpWriter(socket,headsz,cb){
	if(headsz !=2 && headsz !=4){
		throw new Error("HEADSZ NEED 2OR4");
	}
	this.m_headsz = headsz;
	this.m_socket = socket;
	this.m_sendBuf = new Buffer(2048);
	this.m_sendsz = 0;
	this.m_sendqueue = new Array();
	this.m_sendtime = null;
	this.m_cb = cb;
}
TcpWriter.prototype.Write = function (data){
	if(!data){
		return ;
	}
	var tmpData = data;
	var len = 0;
	
	if(!Buffer.isBuffer(data) && typeof(data) != "string" ){
		tmpData = JSON.stringify(data);
	}
	if(Buffer.isBuffer(tmpData)){
		len = tmpData.length;
	}else{
		len = Buffer.byteLength(tmpData);
	}
	if(this.m_headsz == 2 &&  len> 0xffff ){
		throw new Error("write data is too big");
		return ;
	}
	if( len > 0xffffff ){
		throw new Error("write data is too big");
		return ;
	}
	if(len<=0){
		return ;
	}
	this.m_sendqueue.push(tmpData);
	this._RealWrite();
};
TcpWriter.prototype.DirectWrite = function(data,cb)
{
	var len = 0;
	if(!Buffer.isBuffer(data) && typeof(data) != "string" ){
		data = JSON.stringify(data);
	}
	var len = Buffer.isBuffer(data)?data.length:Buffer.byteLength(data) + this.m_headsz;
	var buf = new  Buffer(len);
	if(this.m_headsz == 4){
		buf.writeUInt32BE(0, this.m_sendsz, true);
	}else if(this.m_headsz == 2){
		buf.writeUInt16BE(0, this.m_sendsz, true);
	}else{
		throw Error('tcp_write.js   unknow headsz at Send()'+this.m_headsz);
	}
	if(Buffer.isBuffer(buf)){
		data.copy(buf,this.m_headsz,0,data.length);
	}else{
		buf.utf8Write(data,this.m_sendsz);
	}
	try{
		this.m_socket.write(buf,function(){if(cb)cb();});
	}catch(err){
		if(cb)cb(err);
	}
}
TcpWriter.prototype._RealWrite = function (){
	for(;;){
		if(this.m_sendsz < 1024 && this.m_sendqueue.length>0){
			var buf = this.m_sendqueue.shift();
			if( buf == null){
				continue;
			}
			var len = 0;
			if(Buffer.isBuffer(buf)){
				len = buf.length;
			}else{
				len = Buffer.byteLength(buf);
			}
			var need = this.m_sendsz + len + this.m_headsz;
			if(this.m_sendBuf.length < need){
				var tmpBuf = new  Buffer(need);
				if(this.m_sendsz>0){
					this.m_sendBuf.copy(tmpBuf,0,0,this.m_sendsz);
				}
				this.m_sendBuf = tmpBuf;
			}
			if(this.m_headsz == 4){
				this.m_sendBuf.writeUInt32BE(len, this.m_sendsz, true);
			}else if(this.m_headsz == 2){
				this.m_sendBuf.writeUInt16BE(len, this.m_sendsz, true);
			}else{
				throw Error('tcp_write.js   unknow headsz at Send()'+this.m_headsz);
			}
			this.m_sendsz+=this.m_headsz;
			if(Buffer.isBuffer(buf)){
				buf.copy(this.m_sendBuf,this.m_sendsz,0,len);
			}else{
				this.m_sendBuf.utf8Write(buf,this.m_sendsz);
			}
			this.m_sendsz+=len;
		}
		if(this.m_sendsz >= 1024){
			this._Flush();
			break;
		}else if(this.m_sendqueue.length==0){
			break;
		};
	};
	if(this.m_sendsz > 0 && !this.m_sendtime){
		var self = this;
		this.m_sendtime = setTimeout(function(){
			self.m_sendtime = null;
			self._Flush();
		},10);
	};

	if(this.m_sendsz <=0 && this.m_sendqueue.length <=0){
		if(this.m_cb){
			this.m_cb();
		}
	}
};
TcpWriter.prototype._Flush = function (){
	try{
		if(this.m_sendTime){
			clearTimeout(this.m_sendTime);
			this.m_sendTime = null;
		};
		var tmpBuf = new Buffer(this.m_sendsz);
		this.m_sendBuf.copy(tmpBuf,0,0,this.m_sendsz);
		this.m_sendsz = 0;
		var self = this;
		this.m_socket.write(tmpBuf,function(){
			self._RealWrite();
		});
	}catch(err){
		console.log("WRITER-ERROR",err,err.stack);
	};
};
