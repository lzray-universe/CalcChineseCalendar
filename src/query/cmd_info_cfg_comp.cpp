namespace{

struct InfoOpt{
	std::string ephem;
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
};

struct InfoRes{
	bool exists=false;
	std::uintmax_t size=0;
	double jd_start=std::numeric_limits<double>::quiet_NaN();
	double jd_end=std::numeric_limits<double>::quiet_NaN();
	bool has_cov=false;
};

struct CfgOpt{
	std::string action;
	std::string key;
	std::string value;
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
};

struct CfgRes{
	InterCfg cfg;
	bool saved=false;
};

struct CompOpt{
	std::string shell;
};

struct CompRes{
	std::string text;
};

std::string join_sc(const std::vector<std::string>&items){
	std::string out;
	for(std::size_t i=0;i<items.size();++i){
		if(i!=0){
			out.push_back(';');
		}
		out+=items[i];
	}
	return out;
}

std::vector<std::string> split_sc(const std::string&text){
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
}

bool is_reset_cfg(const std::string&text){
	std::string v=to_low(text);
	return v=="default"||v=="auto"||v=="inherit";
}

void apply_cfg_set(InterCfg&cfg,const std::string&key,const std::string&value){
	const std::unordered_map<std::string,std::function<void()>> handlers={
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
		{"bsp_list",[&](){ cfg.bsp_list=split_sc(value); }},
		{"default_tz",[&](){
			 parse_tz(value);
			 cfg.default_tz=value;
		 }},
		{"default_lang",[&](){
			 lunar::i18n::Lang parsed=lunar::i18n::Lang::Zh;
			 if(!lunar::i18n::try_parse_lang(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid default_lang: "+value+
					 " (expected zh|zht|en|ja|ko)");
			 }
			 cfg.default_lang=lunar::i18n::lang_code(parsed);
		 }},
		{"default_lunar_day_tz",[&](){
			 if(is_reset_cfg(value)){
				 cfg.default_lunar_day_tz.clear();
				 return;
			 }
			 cfg.default_lunar_day_tz=canonical_tz_text(value);
		 }},
		{"def_fmt",[&](){
			 std::string v=to_low(value);
			 chk_fmt(v,{"txt","json","csv","jsonl","ics"},"config def_fmt");
			 cfg.def_fmt=v;
		 }},
		{"hli_trad",[&](){
			 HliProfileCode parsed=HliProfileCode::Folk;
			 if(!parse_hli_profile(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid hli_trad: "+value+
					 " (expected folk|ziping|purple|xieji)");
			 }
			 cfg.hli_trad=hli_profile_key(parsed);
		 }},
		{"hli_year_boundary",[&](){
			 if(is_reset_cfg(value)){
				 cfg.hli_year_boundary.clear();
				 return;
			 }
			 HliYearBoundary parsed=HliYearBoundary::LunarNewYear;
			 if(!parse_hli_year_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid hli_year_boundary: "+value+
					 " (expected lichun|lunar_new_year|dongzhi|default)");
			 }
			 cfg.hli_year_boundary=hli_year_boundary_key(parsed);
		 }},
		{"hli_month_boundary",[&](){
			 if(is_reset_cfg(value)){
				 cfg.hli_month_boundary.clear();
				 return;
			 }
			 HliMonthBoundary parsed=HliMonthBoundary::LunarFirstDay;
			 if(!parse_hli_month_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid hli_month_boundary: "+value+
					 " (expected solar_term|lunar_first_day|default)");
			 }
			 cfg.hli_month_boundary=hli_month_boundary_key(parsed);
		 }},
		{"hli_leap_month_mode",[&](){
			 if(is_reset_cfg(value)){
				 cfg.hli_leap_month_mode.clear();
				 return;
			 }
			 HliLeapMonthMode parsed=HliLeapMonthMode::InheritPrevious;
			 if(!parse_hli_leap_month_mode(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid hli_leap_month_mode: "+value+
					 " (expected ignore|inherit_previous|split_midway|"
					 "shift_to_next|default)");
			 }
			 cfg.hli_leap_month_mode=hli_leap_month_mode_key(parsed);
		 }},
		{"hli_day_boundary",[&](){
			 if(is_reset_cfg(value)){
				 cfg.hli_day_boundary.clear();
				 return;
			 }
			 HliDayBoundary parsed=HliDayBoundary::Hour23;
			 if(!parse_hli_day_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid hli_day_boundary: "+value+
					 " (expected hour23|hour0|default)");
			 }
			 cfg.hli_day_boundary=hli_day_boundary_key(parsed);
		 }},
		{"def_prety",[&](){ cfg.def_prety=parse_bool01(value,"def_prety"); }},
	};
	auto it=handlers.find(key);
	if(it==handlers.end()){
		throw std::invalid_argument("unknown config key: "+key);
	}
	it->second();
}

std::string build_comp(const std::string&shell){
	if(shell=="bash"||shell=="zsh"){
		return "_lunar_complete() {\n"
			   "  local cur\n"
			   "  COMPREPLY=()\n"
			   "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
			   "  local cmds=\"months calendar year event download at convert "
			   "zodiac sky day monthview export next range search eclipse festival "
			   "almanac info config completion\"\n"
			   "  if [[ ${COMP_CWORD} -eq 1 ]]; then\n"
			   "    COMPREPLY=( $(compgen -W \"${cmds}\" -- \"${cur}\") )\n"
			   "    return 0\n"
			   "  fi\n"
			   "  local opts=\"--help --time --input-tz --format --out --tz "
			   "--lunar-day-tz --pretty --quiet --stdin --file --jobs "
			   "--meta-once --from --to --count --kinds --kind --mode "
			   "--pick --lat --lon --height --eot-lon --near --stage "
			   "--sample-min --point-lat --point-lon --point-height "
			   "--point-refine --global-vis --global --global-format "
			   "--grid-lat-step --grid-lon-step --lang --year --trad "
			   "--year-boundary --month-boundary --leap-month-mode "
			   "--day-boundary\"\n"
			   "  COMPREPLY=( $(compgen -W \"${opts}\" -- \"${cur}\") )\n"
			   "}\n"
			   "complete -F _lunar_complete lunar\n";
	}
	if(shell=="fish"){
		return "complete -c lunar -f\n"
			   "complete -c lunar -n '__fish_use_subcommand' -a 'months "
			   "calendar year event download at convert zodiac sky day "
			   "monthview export next range search eclipse festival almanac info "
			   "config completion'\n";
	}
	if(shell=="powershell"){
		return "Register-ArgumentCompleter -Native -CommandName lunar "
			   "-ScriptBlock {\n"
			   "  param($wordToComplete, $commandAst, $cursorPosition)\n"
			   "  $cmds = "
			   "'months','calendar','year','event','download','at','convert',"
			   "'zodiac','sky','day','monthview','export','next','range','search',"
			   "'eclipse','festival','almanac','info','config','completion'\n"
			   "  $cmds | Where-Object { $_ -like \"$wordToComplete*\" } | "
			   "ForEach-Object {\n"
			   "    [System.Management.Automation.CompletionResult]::new("
			   "$_,$_,'ParameterValue',$_)\n"
			   "  }\n"
			   "}\n";
	}
	throw std::invalid_argument(
		"completion shell must be bash|zsh|fish|powershell");
}

InfoOpt parse_info(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument("info requires: <bsp>");
	}
	InfoOpt opt;
	opt.ephem=args[0];
	lunar::ArgParser parser;
	parser.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; });
	parser.parse_all(args,1,"info");
	chk_fmt(opt.format,{"json","txt"},"info");
	return opt;
}

InfoRes run_info(const InfoOpt&opt){
	InfoRes res;
	std::error_code ec;
	res.exists=std::filesystem::exists(opt.ephem,ec);
	res.size=res.exists?std::filesystem::file_size(opt.ephem,ec):0;
	if(res.exists){
		try{
			res.has_cov=parse_spk(opt.ephem,res.jd_start,res.jd_end);
		}catch(...){
			res.has_cov=false;
		}
	}
	return res;
}

void write_info(std::ostream&os,const InfoOpt&opt,const InfoRes&res){
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 write_meta(w,opt.ephem,"Z",{"type=info"});
			 w.key("data");
			 w.obj_begin();
			 w.key("exists");
			 w.value(res.exists);
			 w.key("fsize_b");
			 w.value(static_cast<int>(
				 res.size>static_cast<std::uintmax_t>(std::numeric_limits<int>::max())
					 ?std::numeric_limits<int>::max()
					 :res.size));
			 w.key("leap_last");
			 w.value("2017-01-01");
			 w.key("delt_str");
			 w.value("year<1970 or year>2026: em53; [1972,2026]: leap table; "
					 "[1970,1972): legacy");
			 w.key("spk_cov");
			 if(res.has_cov){
				 w.obj_begin();
				 w.key("jd_tstart");
				 w.value(res.jd_start);
				 w.key("jd_tdb_end");
				 w.value(res.jd_end);
				 w.key("u_sisoap");
				 w.value(fmt_iso(TimeScale::tdb_to_utc(res.jd_start),0,true));
				 w.key("u_eisoap");
				 w.value(fmt_iso(TimeScale::tdb_to_utc(res.jd_end),0,true));
				 w.obj_end();
			 }else{
				 w.null_val();
			 }
			 w.key("tool_ver");
			 w.value(tool_ver());
			 w.key("build_time");
			 w.value(std::string(__DATE__)+" "+std::string(__TIME__));
			 w.obj_end();
			 w.obj_end();
			 os<<"\n";
		 }},
		{"txt",[&](){
			 os<<"tool=lunar format=txt type=info\n";
			 os<<"ephem.path="<<opt.ephem<<"\n";
			 os<<"ephem.exists="<<(res.exists?"1":"0")<<"\n";
			 os<<"ephem.fsize_b="<<res.size<<"\n";
			 os<<"timescale.leap_last=2017-01-01\n";
			 os<<"timescale.delt_str=year<1970 or year>2026: em53; "
			   "[1972,2026]: leap table; [1970,1972): legacy\n";
			 if(res.has_cov){
				 os<<"spk.jd_tstart="<<format_num(res.jd_start)<<"\n";
				 os<<"spk.jd_tdb_end="<<format_num(res.jd_end)<<"\n";
				 os<<"spk.u_sisoap="
				   <<fmt_iso(TimeScale::tdb_to_utc(res.jd_start),0,true)<<"\n";
				 os<<"spk.u_eisoap="
				   <<fmt_iso(TimeScale::tdb_to_utc(res.jd_end),0,true)<<"\n";
			 }else{
				 os<<"spk.coverage=not_avail\n";
			 }
			 os<<"tool.version="<<tool_ver()<<"\n";
			 os<<"tool.build_time="<<__DATE__<<" "<<__TIME__<<"\n";
		 }},
	};
	run_fmt(fmts,opt.format,"info");
}

CfgOpt parse_cfg(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument("config action must be show or set");
	}
	CfgOpt opt;
	opt.action=to_low(args[0]);
	lunar::ArgParser parser;
	parser.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; });
	if(opt.action=="show"){
		parser.parse_all(args,1,"config");
		chk_fmt(opt.format,{"json","txt"},"config show");
		return opt;
	}
	if(opt.action=="set"){
		if(args.size()<3){
			throw std::invalid_argument("config set requires: <key> <value>");
		}
		opt.key=to_low(args[1]);
		opt.value=args[2];
		parser.parse_all(args,3,"config");
		return opt;
	}
	throw std::invalid_argument("config action must be show or set");
}

CfgRes run_cfg(const CfgOpt&opt){
	CfgRes res;
	res.cfg=load_def();
	if(opt.action=="show"){
		return res;
	}
	apply_cfg_set(res.cfg,opt.key,opt.value);
	if(!save_cfg(res.cfg)){
		throw std::runtime_error("failed to save config: "+CFG_FILE);
	}
	res.saved=true;
	return res;
}

void write_cfg(std::ostream&os,const CfgOpt&opt,const CfgRes&res){
	if(opt.action=="set"){
		if(!opt.quiet){
			std::cerr<<lunar::i18n::pick("已写入: ","written: ","出力先: ","저장됨: ")
					 <<CFG_FILE<<"\n";
		}
		return;
	}
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 w.key("meta");
			 w.obj_begin();
			 w.key("tool");
			 w.value("lunar");
			 w.key("version");
			 w.value(tool_ver());
			 w.key("schema");
			 w.value(tool_ver());
			 w.obj_end();
			 w.key("data");
			 w.obj_begin();
			 w.key("def_bsp");
			 w.value(res.cfg.def_bsp);
			 w.key("bsp_dir");
			 w.value(res.cfg.bsp_dir);
			 w.key("bsp_list");
			 w.value(join_sc(res.cfg.bsp_list));
			 w.key("default_tz");
			 w.value(res.cfg.default_tz);
			 w.key("default_lang");
			 w.value(res.cfg.default_lang);
			 w.key("default_lunar_day_tz");
			 w.value(resolve_lunar_day_tz(res.cfg));
			 w.key("def_fmt");
			 w.value(res.cfg.def_fmt);
			 w.key("hli_trad");
			 w.value(res.cfg.hli_trad);
			 w.key("hli_year_boundary");
			 w.value(res.cfg.hli_year_boundary);
			 w.key("hli_month_boundary");
			 w.value(res.cfg.hli_month_boundary);
			 w.key("hli_leap_month_mode");
			 w.value(res.cfg.hli_leap_month_mode);
			 w.key("hli_day_boundary");
			 w.value(res.cfg.hli_day_boundary);
			 w.key("def_prety");
			 w.value(res.cfg.def_prety);
			 w.obj_end();
			 w.obj_end();
			 os<<"\n";
		 }},
		{"txt",[&](){
			 os<<"tool=lunar format=txt type=config\n";
			 os<<"def_bsp="<<res.cfg.def_bsp<<"\n";
			 os<<"bsp_dir="<<res.cfg.bsp_dir<<"\n";
			 os<<"bsp_list="<<join_sc(res.cfg.bsp_list)<<"\n";
			 os<<"default_tz="<<res.cfg.default_tz<<"\n";
			 os<<"default_lang="<<res.cfg.default_lang<<"\n";
			 os<<"default_lunar_day_tz="<<resolve_lunar_day_tz(res.cfg)<<"\n";
			 os<<"def_fmt="<<res.cfg.def_fmt<<"\n";
			 os<<"hli_trad="<<res.cfg.hli_trad<<"\n";
			 os<<"hli_year_boundary="<<res.cfg.hli_year_boundary<<"\n";
			 os<<"hli_month_boundary="<<res.cfg.hli_month_boundary<<"\n";
			 os<<"hli_leap_month_mode="<<res.cfg.hli_leap_month_mode<<"\n";
			 os<<"hli_day_boundary="<<res.cfg.hli_day_boundary<<"\n";
			 os<<"def_prety="<<(res.cfg.def_prety?"1":"0")<<"\n";
		 }},
	};
	run_fmt(fmts,opt.format,"config show");
}

CompOpt parse_comp(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument(
			"completion shell must be bash|zsh|fish|powershell");
	}
	CompOpt opt;
	opt.shell=to_low(args[0]);
	return opt;
}

CompRes run_comp(const CompOpt&opt){
	CompRes res;
	res.text=build_comp(opt.shell);
	return res;
}

void write_comp(std::ostream&os,const CompRes&res){ os<<res.text; }

}

int cmd_info(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_info();
		return 0;
	}
	InfoOpt opt=parse_info(args);
	InfoRes res=run_info(opt);
	OutTgt out=open_out(opt.out_path);
	write_info(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}

int cmd_cfg(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_cfg();
		return 0;
	}
	CfgOpt opt=parse_cfg(args);
	CfgRes res=run_cfg(opt);
	if(opt.action=="show"){
		OutTgt out=open_out(opt.out_path);
		write_cfg(*out.stream,opt,res);
		note_out(opt.out_path,opt.quiet);
	}else{
		write_cfg(std::cout,opt,res);
	}
	return 0;
}

int cmd_comp(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_comp();
		return 0;
	}
	CompOpt opt=parse_comp(args);
	CompRes res=run_comp(opt);
	write_comp(std::cout,res);
	return 0;
}
