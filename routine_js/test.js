routine.On("message",function(src,msg){
    	print("<TEST.JS-On.message>:"+JSON.stringify(src));
});
routine.On("exit",function(){
    	print("<TEST.JS-On.exit>:","exit");
});
routine.parent.On('exit',function(code){
	routine.Exit(code);
});
print(123);
function newAdd(){
	var addRt = routine.Spawn("jsv8","./routine_js/add.js");
	if(addRt){
		addRt.On("message",function(msg){
			print(JSON.stringify(msg));
		});
		var data={
			a:3,
			b:2
		}
		routine.Timer(function(){
			data.a++;
			data.b++;
			addRt.Send({data:JSON.stringify(data),type:1},function(msg){
				print("Add:Responce",JSON.stringify(msg));
			});
		},100);
		addRt.On('exit',function (code){
			if(code == 0){
				print("ADD exit normal");
			}else{
				print("ADD exit error",code);
			}
			routine.Exit();
		});
	}else{
		print("add.js-launch failed");
	}
}
for(var i=0; i<1; i++){
	newAdd();
}
