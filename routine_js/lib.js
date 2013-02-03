function log(){
	var str = '';
	for(var i in arguments){
		if(typeof(arguments[i]) == 'string'){
			str+=arguments[i];
		}else if(typeof(arguments[i]) == 'object'){
			str+=JSON.stringify(arguments[i]);
		}else{
			str+=arguments[i];
		}
	}
	if(str != ''){
		print(str);
	}
}
exports.log = log;
