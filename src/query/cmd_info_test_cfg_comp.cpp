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
				 w.key("default_lang");
				 w.value(cfg.default_lang);
				 w.key("default_lunar_day_tz");
				 w.value(resolve_lunar_day_tz(cfg));
				 w.key("def_fmt");
				 w.value(cfg.def_fmt);
				 w.key("hli_trad");
				 w.value(cfg.hli_trad);
				 w.key("hli_year_boundary");
				 w.value(cfg.hli_year_boundary);
				 w.key("hli_month_boundary");
				 w.value(cfg.hli_month_boundary);
				 w.key("hli_leap_month_mode");
				 w.value(cfg.hli_leap_month_mode);
				 w.key("hli_day_boundary");
				 w.value(cfg.hli_day_boundary);
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
				 *out.stream<<"default_lang="<<cfg.default_lang<<"\n";
				 *out.stream<<"default_lunar_day_tz="<<resolve_lunar_day_tz(cfg)
							<<"\n";
				 *out.stream<<"def_fmt="<<cfg.def_fmt<<"\n";
				 *out.stream<<"hli_trad="<<cfg.hli_trad<<"\n";
				 *out.stream<<"hli_year_boundary="<<cfg.hli_year_boundary<<"\n";
				 *out.stream<<"hli_month_boundary="<<cfg.hli_month_boundary<<"\n";
				 *out.stream<<"hli_leap_month_mode="<<cfg.hli_leap_month_mode<<"\n";
				 *out.stream<<"hli_day_boundary="<<cfg.hli_day_boundary<<"\n";
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
		auto is_reset_value=[&](const std::string&text){
			std::string v=to_low(text);
			return v=="default"||v=="auto"||v=="inherit";
		};
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
				 if(is_reset_value(value)){
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
				 if(is_reset_value(value)){
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
				 if(is_reset_value(value)){
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
				 if(is_reset_value(value)){
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
				 if(is_reset_value(value)){
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
		auto it=key_handlers.find(key);
		if(it==key_handlers.end()){
			throw std::invalid_argument("unknown config key: "+key);
		}
		it->second();
		if(!save_cfg(cfg)){
			throw std::runtime_error("failed to save config: "+CFG_FILE);
		}
		if(!quiet){
			std::cerr<<lunar::i18n::pick("已写入: ","written: ","出力先: ","저장됨: ")
					 <<CFG_FILE<<"\n";
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
				   "convert zodiac sky day monthview next range search eclipse festival "
				   "almanac info config completion\"\n"
				 <<"  if [[ ${COMP_CWORD} -eq 1 ]]; then\n"
				 <<"    COMPREPLY=( $(compgen -W \"${cmds}\" -- \"${cur}\") )\n"
				 <<"    return 0\n"
				 <<"  fi\n"
				 <<"  local opts=\"--help --time --input-tz --format --out --tz "
				   "--lunar-day-tz "
				   "--pretty --quiet --stdin --file --jobs --meta-once --from "
				   "--to --count --kinds --kind --mode --pick --lat --lon "
				   "--height --eot-lon --near --stage --sample-min --point-lat "
				   "--point-lon --point-height --point-refine --global-vis "
				   "--global --global-format --grid-lat-step --grid-lon-step "
				   "--lang --year --trad --year-boundary --month-boundary "
				   "--leap-month-mode --day-boundary\"\n"
				 <<"  COMPREPLY=( $(compgen -W \"${opts}\" -- \"${cur}\") )\n"
				 <<"}\n"
				 <<"complete -F _lunar_complete lunar\n";
		return 0;
	}
	if(shell=="fish"){
		std::cout<<"complete -c lunar -f\n"
				 <<"complete -c lunar -n '__fish_use_subcommand' -a 'months "
				   "calendar year event download at convert zodiac sky day monthview next "
				   "range search eclipse festival almanac info config "
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
			  "zodiac','sky','day','monthview','next','range','search','eclipse','festival',"
			  "'almanac','info','config','completion'\n"
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

