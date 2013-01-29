
const ParseState = {
		SIZE:0,
		SIZEING:1,
		READING:2,
};
exports.TcpReader = TcpReader;
function TcpReader(socket,headsz,recvCb){
	if(!recvCb){
		throw new Error("需要一个回调");
	}else if(headsz !=2 && headsz !=4){
		throw new Error("HEADSZ NEED 2OR4"+headsz);
	}
	var headSz_ = Number(headsz);
	var cacheBuf_ = null ;
	var parseState_ = ParseState.SIZE;
	var leftSz_ = 0;
	var readSz_ = 0;
	var headReadSz_ = 0;
	var szBuf_ = new Buffer(headSz_);
	var cb_ = function (data,sz,bcopy){
		try{
			if(bcopy){
				var tmp = new Buffer(sz);
				data.copy(tmp,0,0,sz);
				data = tmp;
			}
			process.nextTick(function (){
				recvCb(data,sz);
			});
		}catch(err){
			console.log(err,err.stack);
		};
	}
	socket.on("data",function(recvBuf){
		var buf = recvBuf;
		while(buf.length > 0){
			if( parseState_ == ParseState.SIZE ){//read head
				if(buf.length < headSz_){//cache headsz
					parseState_ = ParseState.SIZEING;
					buf.copy(szBuf_,0,0,buf.length);
					headReadSz_ = buf.length;
					break;
				}else{
					if(headSz_ == 4){
						leftSz_ = buf.readUInt32BE(0,false);
					}else if(headSz_ == 2){
						leftSz_ = buf.readUInt16BE(0,false);
					}
					if(leftSz_ >= 0xffffff){
						console.log("parse error leftSz_ is too big",leftSz_,buf);
						try{
							socket.end();
							socket.destroy();
							socket.emit('close');
						}catch(err){
							console.log(err,err.stack,"tcp_reader");
						}
						return;
					}
					buf = buf.slice(headSz_ , buf.length);
					parseState_ = ParseState.READING;
					readSz_ = 0;
				};
			}else if( parseState_ == ParseState.SIZEING){
				if(buf.length >=headSz_-headReadSz_){//merge headsz
					var tmpSz = headSz_-headReadSz_;
					buf.copy(szBuf_,headReadSz_,0,tmpSz);
					if(headSz_ == 4){
						leftSz_ = szBuf_.readUInt32BE(0,false);
					}else if(headSz_ == 2){
						leftSz_ = szBuf_.readUInt16BE(0,false);
					}
					buf = buf.slice(tmpSz, buf.length);
					parseState_ = ParseState.READING;
					readSz_ = 0;
				}else{//cache headsz
					buf.copy(szBuf_,headReadSz_,0,buf.length);
					headReadSz_+=buf.length;
					break;
				};
			}else if( parseState_ == ParseState.READING ){
				if(leftSz_ <= buf.length){//merge read buffer
					if(readSz_ > 0){
						buf.copy(cacheBuf_,readSz_,0,leftSz_,false);
						cb_(cacheBuf_,readSz_+leftSz_);
						cacheBuf_ = null;
					}else{
						cb_(buf.slice(0,leftSz_),leftSz_,true);
					}
					buf = buf.slice(leftSz_,buf.length);
					parseState_ = ParseState.SIZE;
				}else{//cache read buffer
					if( !cacheBuf_){
						cacheBuf_ = new Buffer(readSz_ + leftSz_);
					}
					buf.copy(cacheBuf_,readSz_,0,buf.length);
					leftSz_ -= buf.length;
					readSz_ += buf.length;
					break;
				};
			};
		};
	});
};