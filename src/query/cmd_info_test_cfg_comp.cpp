int cmd_info(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_info();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument("info requires: <bsp>");
	}
	std::string ephem=args[0];
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
	const OptMap handlers={
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ out_path=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ quiet=true; }},
	};
	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"info");
	}
	chk_fmt(format,{"json","txt"},"info");

	std::error_code ec;
	bool exists=std::filesystem::exists(ephem,ec);
	std::uintmax_t size=exists?std::filesystem::file_size(ephem,ec):0;

	double jd_start=std::numeric_limits<double>::quiet_NaN();
	double jd_end=std::numeric_limits<double>::quiet_NaN();
	bool has_cov=false;
	if(exists){
		try{
			has_cov=parse_spk(ephem,jd_start,jd_end);
		}catch(...){
			has_cov=false;
		}
	}

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,pretty);
			 w.obj_begin();
			 write_meta(w,ephem,"Z",{"type=info"});
			 w.key("data");
			 w.obj_begin();
			 w.key("exists");
			 w.value(exists);
			 w.key("fsize_b");
			 w.value(static_cast<int>(
				 size>static_cast<std::uintmax_t>(std::numeric_limits<int>::max())
					 ?std::numeric_limits<int>::max()
					 :size));
			 w.key("leap_last");
			 w.value("2017-01-01");
			 w.key("delt_str");
			 w.value("year<1970 or year>2026: em53; [1972,2026]: leap table; "
					 "[1970,1972): legacy");
			 if(has_cov){
				 w.key("spk_cov");
				 w.obj_begin();
				 w.key("jd_tstart");
				 w.value(jd_start);
				 w.key("jd_tdb_end");
				 w.value(jd_end);
				 w.key("u_sisoap");
				 w.value(fmt_iso(TimeScale::tdb_to_utc(jd_start),0,true));
				 w.key("u_eisoap");
				 w.value(fmt_iso(TimeScale::tdb_to_utc(jd_end),0,true));
				 w.obj_end();
			 }else{
				 w.key("spk_cov");
				 w.null_val();
			 }
			 w.key("tool_ver");
			 w.value(tool_ver());
			 w.key("build_time");
			 w.value(std::string(__DATE__)+" "+std::string(__TIME__));
			 w.obj_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=info\n";
			 os<<"ephem.path="<<ephem<<"\n";
			 os<<"ephem.exists="<<(exists?"1":"0")<<"\n";
			 os<<"ephem.fsize_b="<<size<<"\n";
			 os<<"timescale.leap_last=2017-01-01\n";
			 os<<"timescale.delt_str=year<1970 or year>2026: em53; "
				 "[1972,2026]: leap table; [1970,1972): legacy\n";
			 if(has_cov){
				 os<<"spk.jd_tstart="<<format_num(jd_start)<<"\n";
				 os<<"spk.jd_tdb_end="<<format_num(jd_end)<<"\n";
				 os<<"spk.u_sisoap="
				   <<fmt_iso(TimeScale::tdb_to_utc(jd_start),0,true)<<"\n";
				 os<<"spk.u_eisoap="
				   <<fmt_iso(TimeScale::tdb_to_utc(jd_end),0,true)<<"\n";
			 }else{
				 os<<"spk.coverage=not_avail\n";
			 }
			 os<<"tool.version="<<tool_ver()<<"\n";
			 os<<"tool.build_time="<<__DATE__<<" "<<__TIME__<<"\n";
		 }},
	};
	run_fmt(fmt_handlers,format,"info");
	note_out(out_path,quiet);
	return 0;
}

int cmd_test(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_test();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument("selftest requires: <bsp>");
	}
	std::string ephem=args[0];
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
	const OptMap handlers={
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ out_path=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ quiet=true; }},
	};
	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"selftest");
	}
	chk_fmt(format,{"json","txt"},"selftest");

	struct Case{
		std::string id;
		bool pass=false;
		std::string message;
	};
	std::vector<Case> cases;
	bool all_pass=true;
	try{
		EphRead eph(ephem);
		QueryCache cache(eph);

		Case c1;
		c1.id="at_illum";
		try{
			AtData atd=at_ftxt(eph,"2025-06-01T00:00:00+08:00","+08:00",480,
							   "+08:00",true,false,0.0,120.0,&cache);
			c1.pass=(atd.ill_pct>=0.0&&atd.ill_pct<=100.0);
			c1.message=c1.pass?"ok":"illumination out of [0,100]";
		}catch(const std::exception&ex){
			c1.pass=false;
			c1.message=ex.what();
		}
		cases.push_back(c1);
		all_pass=all_pass&&c1.pass;

		Case c2;
		c2.id="conv_rt";
		try{
			IsoTime p=parse_iso("2026-02-18","+08:00");
			LunDate ld=res_lun(eph,p.jd_utc,&cache);
			GregDate g=res_greg(eph,ld.lunar_year,ld.lun_mno,ld.lunar_day,
								ld.is_leap,&cache);
			int gy=0,gm=0,gd=0;
			std::tie(gy,gm,gd)=parse_ymd("2026-02-18");
			int ry=0,rm=0,rd=0;
			utc2cst(g.cstday_jd,ry,rm,rd);
			c2.pass=(gy==ry&&gm==rm&&gd==rd);
			c2.message=c2.pass?"ok":"roundtrip mismatch";
		}catch(const std::exception&ex){
			c2.pass=false;
			c2.message=ex.what();
		}
		cases.push_back(c2);
		all_pass=all_pass&&c2.pass;

		Case c3;
		c3.id="y25_cnt";
		try{
			SolLunCal solver(eph);
			YearResult yr=solver.compute_year(2025,quiet?nullptr:&std::cerr);
			std::size_t solar_n=yr.sol_terms.size();
			std::size_t phase_n=yr.lun_phase.size()*4;
			c3.pass=(solar_n==24&&phase_n>=48);
			std::ostringstream msg;
			msg<<"sol_terms="<<solar_n<<", lp_events="<<phase_n;
			c3.message=msg.str();
		}catch(const std::exception&ex){
			c3.pass=false;
			c3.message=ex.what();
		}
		cases.push_back(c3);
		all_pass=all_pass&&c3.pass;
	}catch(const std::exception&ex){
		all_pass=false;
		cases.push_back(Case{"bootstrap",false,ex.what()});
	}

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,pretty);
			 w.obj_begin();
			 write_meta(w,ephem,"Z",{"type=selftest"});
			 w.key("data");
			 w.obj_begin();
			 w.key("pass");
			 w.value(all_pass);
			 w.key("cases");
			 w.arr_begin();
			 for(const auto&c : cases){
				 w.obj_begin();
				 w.key("id");
				 w.value(c.id);
				 w.key("pass");
				 w.value(c.pass);
				 w.key("message");
				 w.value(c.message);
				 w.obj_end();
			 }
			 w.arr_end();
			 w.obj_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=selftest\n";
			 os<<"result.pass="<<(all_pass?"1":"0")<<"\n";
			 os<<"id\tpass\tmessage\n";
			 for(const auto&c : cases){
				 os<<c.id<<"\t"<<(c.pass?"1":"0")<<"\t"<<c.message<<"\n";
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"selftest");
	note_out(out_path,quiet);
	return all_pass?0:1;
}

int cmd_cfg(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_cfg();
		return 0;
	}
	std::string action=to_low(args[0]);
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
	const OptMap handlers={
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ out_path=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ quiet=true; }},
	};

	auto parse_opt=[&](std::size_t start){
		for(std::size_t i=start;i<args.size();++i){
			const std::string&opt=args[i];
			apply_opt(handlers,args,i,opt,"config");
		}
	};

	InterCfg cfg=load_def();
	if(action=="show"){
		parse_opt(1);
		chk_fmt(format,{"json","txt"},"config show");
		auto join_sc=[](const std::vector<std::string>&items){
			std::string out;
			for(std::size_t i=0;i<items.size();++i){
				if(i!=0){
					out.push_back(';');
				}
				out+=items[i];
			}
			return out;
		};
		OutTgt out=open_out(out_path);
		const FmtMap fmt_handlers={
			{"json",[&](){
				 JsonWriter w(*out.stream,pretty);
				 w.obj_begin();
				 w.key("meta");
				 w.obj_begin();
				 w.key("tool");
				 w.value("lunar");
				 w.key("schema");
				 w.value("lunar.v1");
				 w.obj_end();
				 w.key("data");
				 w.obj_begin();
				 w.key("def_bsp");
				 w.value(cfg.def_bsp);
				 w.key("bsp_dir");
				 w.value(cfg.bsp_dir);
				 w.key("bsp_list");
				 w.value(join_sc(cfg.bsp_list));
				 w.key("default_tz");
				 w.value(cfg.default_tz);
				 w.key("def_fmt");
				 w.value(cfg.def_fmt);
				 w.key("def_prety");
				 w.value(cfg.def_prety);
				 w.obj_end();
				 w.obj_end();
				 *out.stream<<"\n";
			 }},
			{"txt",[&](){
				 *out.stream<<"tool=lunar format=txt type=config\n";
				 *out.stream<<"def_bsp="<<cfg.def_bsp<<"\n";
				 *out.stream<<"bsp_dir="<<cfg.bsp_dir<<"\n";
				 *out.stream<<"bsp_list="<<join_sc(cfg.bsp_list)<<"\n";
				 *out.stream<<"default_tz="<<cfg.default_tz<<"\n";
				 *out.stream<<"def_fmt="<<cfg.def_fmt<<"\n";
				 *out.stream<<"def_prety="<<(cfg.def_prety?"1":"0")<<"\n";
			 }},
		};
		run_fmt(fmt_handlers,format,"config show");
		note_out(out_path,quiet);
		return 0;
	}

	if(action=="set"){
		if(args.size()<3){
			throw std::invalid_argument("config set requires: <key> <value>");
		}
		std::string key=to_low(args[1]);
		std::string value=args[2];
		parse_opt(3);
		auto parse_sc=[](const std::string&text){
			std::vector<std::string> out;
			std::string token;
			auto flush=[&](){
				std::string t=trim(token);
				if(!t.empty()){
					out.push_back(t);
				}
				token.clear();
			};
			for(char ch : text){
				if(ch==','||ch==';'){
					flush();
				}else{
					token.push_back(ch);
				}
			}
			flush();
			return out;
		};
		const std::unordered_map<std::string,std::function<void()>> key_handlers={
			{"def_bsp",[&](){
				 cfg.def_bsp=value;
				 bool exists=false;
				 for(const auto&item : cfg.bsp_list){
					 if(item==value){
						 exists=true;
						 break;
					 }
				 }
				 if(!exists&&!value.empty()){
					 cfg.bsp_list.push_back(value);
				 }
			 }},
			{"bsp_dir",[&](){ cfg.bsp_dir=value; }},
			{"bsp_list",[&](){ cfg.bsp_list=parse_sc(value); }},
			{"default_tz",[&](){
				 parse_tz(value);
				 cfg.default_tz=value;
			 }},
			{"def_fmt",[&](){
				 std::string v=to_low(value);
				 chk_fmt(v,{"txt","json","csv","jsonl","ics"},"config def_fmt");
				 cfg.def_fmt=v;
			 }},
			{"def_prety",[&](){ cfg.def_prety=parse_bool01(value,"def_prety"); }},
		};
		auto it=key_handlers.find(key);
		if(it==key_handlers.end()){
			throw std::invalid_argument("unknown config key: "+key);
		}
		it->second();
		if(!save_cfg(cfg)){
			throw std::runtime_error("failed to save config: "+CFG_FILE);
		}
		if(!quiet){
			std::cerr<<"written: "<<CFG_FILE<<"\n";
		}
		return 0;
	}

	throw std::invalid_argument("config action must be show or set");
}

int cmd_comp(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_comp();
		return 0;
	}
	std::string shell=to_low(args[0]);
	if(shell=="bash"||shell=="zsh"){
		std::cout<<"_lunar_complete() {\n"
				 <<"  local cur\n"
				 <<"  COMPREPLY=()\n"
				 <<"  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
				 <<"  local cmds=\"months calendar year event download at "
				   "convert day monthview next range search eclipse festival "
				   "almanac info selftest config completion\"\n"
				 <<"  if [[ ${COMP_CWORD} -eq 1 ]]; then\n"
				 <<"    COMPREPLY=( $(compgen -W \"${cmds}\" -- \"${cur}\") )\n"
				 <<"    return 0\n"
				 <<"  fi\n"
				 <<"  local opts=\"--help --format --out --tz --pretty --quiet "
				   "--stdin --file --jobs --meta-once --from --to --count "
				   "--kinds --kind --eot-lon --near --stage --sample-min --point-lat "
				   "--point-lon --point-height --point-refine --global-vis "
				   "--global --global-format --grid-lat-step --grid-lon-step\"\n"
				 <<"  COMPREPLY=( $(compgen -W \"${opts}\" -- \"${cur}\") )\n"
				 <<"}\n"
				 <<"complete -F _lunar_complete lunar\n";
		return 0;
	}
	if(shell=="fish"){
		std::cout<<"complete -c lunar -f\n"
				 <<"complete -c lunar -n '__fish_use_subcommand' -a 'months "
				   "calendar year event download at convert day monthview next "
				   "range search eclipse festival almanac info selftest config "
				   "completion'\n";
		return 0;
	}
	if(shell=="powershell"){
		std::cout
			<<"Register-ArgumentCompleter -Native -CommandName lunar "
			  "-ScriptBlock {\n"
			<<"  param($wordToComplete, $commandAst, $cursorPosition)\n"
			<<"  $cmds = "
			  "'months','calendar','year','event','download','at','convert','"
			  "day','monthview','next','range','search','eclipse','festival',"
			  "'almanac','info','selftest','config','completion'\n"
			<<"  $cmds | Where-Object { $_ -like \"$wordToComplete*\" } | "
			  "ForEach-Object {\n"
			<<"    "
			  "[System.Management.Automation.CompletionResult]::new($_,$_,'"
			  "ParameterValue',$_)\n"
			<<"  }\n"
			<<"}\n";
		return 0;
	}
	throw std::invalid_argument(
		"completion shell must be bash|zsh|fish|powershell");
}

