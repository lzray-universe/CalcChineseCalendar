var lunar_pending_request=null;
var lunar_ready=false;
var lunar_stdout=[];
var lunar_stderr=[];

function lunar_push_line(dst,text){
	if(typeof text==='undefined'){
		return;
	}
	dst.push(String(text));
}

function lunar_error_text(ex){
	if(ex&&typeof ex.message==='string'&&ex.message.length){
		return ex.message;
	}
	return String(ex);
}

function lunar_join_path(base,name){
	if(base==='/'||!base.length){
		return '/'+name;
	}
	if(base.charAt(base.length-1)==='/'){
		return base+name;
	}
	return base+'/'+name;
}

function lunar_resolve_path(path,cwd){
	if(typeof path!=='string'||!path.length){
		throw new Error('path is required');
	}
	var text=path.replace(/\\/g,'/');
	if(text.charAt(0)==='/'){
		return text;
	}
	return lunar_join_path(cwd,text);
}

function lunar_dirname(path){
	var pos=path.lastIndexOf('/');
	if(pos<=0){
		return '/';
	}
	return path.slice(0,pos);
}

function lunar_ensure_dir(path){
	if(path==='/'||!path.length){
		return;
	}
	var parts=path.split('/');
	var cur='';
	for(var i=0;i<parts.length;++i){
		var part=parts[i];
		if(!part||part==='.'||part==='..'){
			continue;
		}
		cur=lunar_join_path(cur||'/',part);
		if(FS.analyzePath(cur).exists){
			continue;
		}
		FS.mkdir(cur);
	}
}

function lunar_mount_dir(path,type,opts){
	lunar_ensure_dir(path);
	try{
		FS.mount(type,opts,path);
	}catch(ex){
		var info=FS.analyzePath(path);
		if(!info.exists||!info.object||!info.object.mounted){
			throw ex;
		}
	}
}

function lunar_to_u8(data){
	if(data instanceof Uint8Array){
		return data;
	}
	if(data instanceof ArrayBuffer){
		return new Uint8Array(data);
	}
	if(ArrayBuffer.isView(data)){
		return new Uint8Array(data.buffer,data.byteOffset,data.byteLength);
	}
	throw new Error('file data must be ArrayBuffer or Uint8Array');
}

function lunar_write_files(files,cwd){
	for(var i=0;i<files.length;++i){
		var file=files[i];
		if(!file||typeof file.path!=='string'||!file.path.length){
			throw new Error('files[].path is required');
		}
		var path=lunar_resolve_path(file.path,cwd);
		lunar_ensure_dir(lunar_dirname(path));
		FS.writeFile(path,lunar_to_u8(file.data));
	}
}

function lunar_mounts(mounts,cwd){
	for(var i=0;i<mounts.length;++i){
		var mount=mounts[i];
		if(!mount){
			throw new Error('mounts[] item is required');
		}
		var path=lunar_resolve_path(mount&&mount.path?mount.path:'/input',cwd);
		if(typeof WORKERFS==='undefined'){
			throw new Error('WORKERFS is unavailable');
		}
		if(Array.isArray(mount.files)&&mount.files.length){
			lunar_mount_dir(path,WORKERFS,{files:mount.files});
			continue;
		}
		if(Array.isArray(mount.blobs)&&mount.blobs.length){
			lunar_mount_dir(path,WORKERFS,{blobs:mount.blobs});
			continue;
		}
		throw new Error('mounts[] requires files or blobs');
	}
}

function lunar_apply_fs(request){
	var cwd='/working';
	if(typeof request.cwd==='string'&&request.cwd.length){
		cwd=lunar_resolve_path(request.cwd,'/');
	}
	lunar_ensure_dir(cwd);
	FS.chdir(cwd);
	if(Array.isArray(request.mounts)&&request.mounts.length){
		lunar_mounts(request.mounts,cwd);
	}
	if(Array.isArray(request.files)&&request.files.length){
		lunar_write_files(request.files,cwd);
	}
}

function lunar_result_payload(exit_code,error_text){
	var payload={
		type:'result',
		exit_code:exit_code,
		stdout:lunar_stdout.join('\n'),
		stderr:lunar_stderr.join('\n')
	};
	if(typeof error_text==='string'&&error_text.length){
		payload.error=error_text;
	}
	return payload;
}

function lunar_run_request(request){
	lunar_stdout=[];
	lunar_stderr=[];
	try{
		lunar_apply_fs(request||{});
		var argv=[];
		if(Array.isArray(request.argv)){
			argv=request.argv.slice();
		}
		postMessage(lunar_result_payload(Module.callMain(argv),''));
	}catch(ex){
		var text=lunar_error_text(ex);
		lunar_push_line(lunar_stderr,text);
		postMessage(lunar_result_payload(1,text));
	}
	close();
}

var Module={
	noInitialRun:true,
	print:function(text){
		lunar_push_line(lunar_stdout,text);
	},
	printErr:function(text){
		lunar_push_line(lunar_stderr,text);
	},
	onAbort:function(text){
		lunar_push_line(lunar_stderr,text);
	},
	onRuntimeInitialized:function(){
		lunar_ready=true;
		postMessage({type:'ready'});
		if(lunar_pending_request!==null){
			var request=lunar_pending_request;
			lunar_pending_request=null;
			lunar_run_request(request);
		}
	}
};

self.onmessage=function(ev){
	var request=ev.data||{};
	if(lunar_ready){
		lunar_run_request(request);
		return;
	}
	lunar_pending_request=request;
};

importScripts('lunar.js');
