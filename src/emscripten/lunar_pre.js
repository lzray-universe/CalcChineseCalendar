Module['preRun']=Module['preRun']||[];
Module['preRun'].push(function(){
	if(typeof ENVIRONMENT_IS_NODE==='undefined'||!ENVIRONMENT_IS_NODE){
		return;
	}
	if(typeof NODEFS==='undefined'){
		return;
	}

	var node_fs=require('node:fs');

	function ensure_dir(path){
		var info=FS.analyzePath(path);
		if(info.exists){
			return;
		}
		FS.mkdir(path);
	}

	function mount_dir(path,root){
		ensure_dir(path);
		try{
			FS.mount(NODEFS,{root:root},path);
		}catch(ex){
			var info=FS.analyzePath(path);
			if(!info.exists||!info.object||!info.object.mounted){
				throw ex;
			}
		}
	}

	function rewrite_host_path(text){
		if(typeof text!=='string'){
			return text;
		}
		if(/^[A-Za-z]:[\\/]/.test(text)){
			var drive=text.charAt(0).toUpperCase();
			var tail=text.slice(2).replace(/\\/g,'/');
			if(!tail.startsWith('/')){
				tail='/'+tail;
			}
			return '/'+drive+tail;
		}
		return text;
	}

	mount_dir('/working',process.cwd());
	FS.chdir('/working');

	for(var code=65;code<=90;++code){
		var drive=String.fromCharCode(code);
		var root=drive+':\\';
		if(!node_fs.existsSync(root)){
			continue;
		}
		mount_dir('/'+drive,root);
	}

	if(Array.isArray(arguments_)){
		for(var i=0;i<arguments_.length;++i){
			arguments_[i]=rewrite_host_path(arguments_[i]);
		}
	}
});
