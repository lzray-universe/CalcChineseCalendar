int cmd_next(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_next();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"next requires: <bsp> --from <time> --count N");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string from_time;
	int count=1;
	std::string kinds="solar_term,lunar_phase,lunar_eclipse,solar_eclipse";
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"&&format!="ics"&&
	   format!="jsonl"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	bool calc_eclipse=false;
	const OptMap handlers={
		{"--from",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 from_time=req_val(src,idx,opt);
		 }},
		{"--count",[&](const std::vector<std::string>&src,std::size_t&idx,
					   const std::string&opt){
			 count=parse_int(req_val(src,idx,opt),"--count");
			 if(count<1){
				 throw std::invalid_argument("--count must be >=1");
			 }
		 }},
		{"--kinds",[&](const std::vector<std::string>&src,std::size_t&idx,
					   const std::string&opt){ kinds=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ tz=req_val(src,idx,opt); }},
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
		{"--eclipse",[&](const std::vector<std::string>&src,std::size_t&idx,
						 const std::string&opt){
			 calc_eclipse=parse_bool01(req_val(src,idx,opt),opt);
		 }},
	};

	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"next");
	}
	if(from_time.empty()){
		throw std::invalid_argument("next requires --from <time>");
	}
	chk_fmt(format,{"json","txt","csv","ics","jsonl"},"next");

	IsoTime parsed=parse_iso(from_time,cfg.default_tz);
	EvtFilt filter=parse_ef(kinds);
	int tz_off=parse_tz(tz);

	EphRead eph(ephem);
	int cst_year=0,cst_month=0,cst_day=0;
	utc2cst(parsed.jd_utc,cst_year,cst_month,cst_day);
	int span=1;
	std::vector<EventRec> picked;
	while(span<=8){
		std::set<int> years;
		for(int y=cst_year-span;y<=cst_year+span;++y){
			years.insert(y);
		}
		std::vector<EventRec> all=
			col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
		std::vector<EventRec> filtered=
			filt_evs(all,filter,parsed.jd_utc,
					 std::numeric_limits<double>::infinity(),false,true);
		std::sort(
			filtered.begin(),filtered.end(),
			[](const EventRec&a,const EventRec&b){ return a.jd_utc<b.jd_utc; });
		if(static_cast<int>(filtered.size())>=count||span==8){
			if(static_cast<int>(filtered.size())>count){
				filtered.resize(static_cast<std::size_t>(count));
			}
			picked=std::move(filtered);
			break;
		}
		++span;
	}

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 wr_eljs(*out.stream,ephem,tz,pretty,picked,"next",eph,calc_eclipse,
					 tz_off);
		 }},
		{"txt",[&](){
			 wr_eltxt(*out.stream,tz,picked,"next",&eph,calc_eclipse,tz_off);
		 }},
		{"csv",[&](){
			 wr_elcsv(*out.stream,picked,&eph,calc_eclipse,tz_off);
		 }},
		{"jsonl",[&](){
			 wr_eljsl(*out.stream,ephem,tz,picked,"next",eph,calc_eclipse,
					  tz_off);
		 }},
		{"ics",[&](){ wr_elics(*out.stream,ephem,"lunar-next",picked); }},
	};
	run_fmt(fmt_handlers,format,"next");
	note_out(out_path,quiet);
	return 0;
}

int cmd_range(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_range();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"range requires: <bsp> --from <time> --to <time>");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string from_time;
	std::string to_time;
	std::string kinds="solar_term,lunar_phase,lunar_eclipse,solar_eclipse";
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"&&format!="ics"&&
	   format!="jsonl"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	bool calc_eclipse=false;
	const OptMap handlers={
		{"--from",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 from_time=req_val(src,idx,opt);
		 }},
		{"--to",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ to_time=req_val(src,idx,opt); }},
		{"--kinds",[&](const std::vector<std::string>&src,std::size_t&idx,
					   const std::string&opt){ kinds=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ tz=req_val(src,idx,opt); }},
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
		{"--eclipse",[&](const std::vector<std::string>&src,std::size_t&idx,
						 const std::string&opt){
			 calc_eclipse=parse_bool01(req_val(src,idx,opt),opt);
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ quiet=true; }},
	};

	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"range");
	}
	if(from_time.empty()||to_time.empty()){
		throw std::invalid_argument(
			"range requires --from <time> and --to <time>");
	}
	chk_fmt(format,{"json","txt","csv","ics","jsonl"},"range");
	IsoTime from_par=parse_iso(from_time,cfg.default_tz);
	IsoTime to_parsed=parse_iso(to_time,cfg.default_tz);
	if(to_parsed.jd_utc<from_par.jd_utc){
		throw std::invalid_argument("--to must be >= --from");
	}

	EvtFilt filter=parse_ef(kinds);
	if(filter.inc_ecl){
		calc_eclipse=true;
	}
	int tz_off=parse_tz(tz);
	EphRead eph(ephem);
	std::vector<EventRec> picked=load_evs(eph,from_par.jd_utc,to_parsed.jd_utc,
										  filter,tz_off,quiet,false);

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 wr_eljs(*out.stream,ephem,tz,pretty,picked,"range",eph,calc_eclipse,
					 tz_off);
		 }},
		{"txt",[&](){
			 wr_eltxt(*out.stream,tz,picked,"range",&eph,calc_eclipse,tz_off);
		 }},
		{"csv",[&](){ wr_elcsv(*out.stream,picked,&eph,calc_eclipse,tz_off); }},
		{"jsonl",[&](){
			 wr_eljsl(*out.stream,ephem,tz,picked,"range",eph,calc_eclipse,
					  tz_off);
		 }},
		{"ics",[&](){ wr_elics(*out.stream,ephem,"lunar-range",picked); }},
	};
	run_fmt(fmt_handlers,format,"range");
	note_out(out_path,quiet);
	return 0;
}

int cmd_search(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_search();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("search requires: <bsp> <query>");
	}

	std::string ephem=args[0];
	std::string query=args[1];
	std::string from_time="2025-01-01T00:00:00+08:00";
	int count=1;
	std::string format="txt";
	std::string tz="+08:00";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
	bool calc_eclipse=false;
	const OptMap handlers={
		{"--from",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 from_time=req_val(src,idx,opt);
		 }},
		{"--count",[&](const std::vector<std::string>&src,std::size_t&idx,
					   const std::string&opt){
			 count=parse_int(req_val(src,idx,opt),"--count");
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 format=to_low(req_val(src,idx,opt));
		 }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ tz=req_val(src,idx,opt); }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ out_path=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ quiet=true; }},
		{"--eclipse",[&](const std::vector<std::string>&src,std::size_t&idx,
						 const std::string&opt){
			 calc_eclipse=parse_bool01(req_val(src,idx,opt),opt);
		 }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"search");
	}

	std::istringstream iss(to_low(query));
	std::string a,b,c;
	iss>>a>>b>>c;
	if(a!="next"){
		throw std::invalid_argument(
			"search currently supports query starting with 'next ...'");
	}

	std::vector<std::string> next_args;
	next_args.push_back(ephem);
	next_args.push_back("--from");
	next_args.push_back(from_time);
	next_args.push_back("--count");
	next_args.push_back(std::to_string(count));
	next_args.push_back("--format");
	next_args.push_back(format);
	next_args.push_back("--tz");
	next_args.push_back(tz);
	if(!out_path.empty()){
		next_args.push_back("--out");
		next_args.push_back(out_path);
	}
	next_args.push_back("--pretty");
	next_args.push_back(pretty?"1":"0");
	if(quiet){
		next_args.push_back("--quiet");
	}
	if(calc_eclipse){
		next_args.push_back("--eclipse");
		next_args.push_back("1");
	}

	const std::unordered_map<std::string,std::string> kind_hints={
		{"full_moon","lunar_phase"},
		{"new_moon","lunar_phase"},
		{"fst_qtr","lunar_phase"},
		{"lst_qtr","lunar_phase"},
		{"solar_term","solar_term"},
		{"lunar_phase","lunar_phase"},
		{"lunar_eclipse","lunar_eclipse"},
		{"lunar-eclipse","lunar_eclipse"},
		{"solar_eclipse","solar_eclipse"},
		{"solar-eclipse","solar_eclipse"},
		{"eclipse","lunar_eclipse,solar_eclipse"},
		{"total_eclipse","lunar_eclipse"},
		{"partial_eclipse","lunar_eclipse"},
		{"penumbral_eclipse","lunar_eclipse"},
		{"total_solar_eclipse","solar_eclipse"},
		{"annular_eclipse","solar_eclipse"},
		{"hybrid_eclipse","solar_eclipse"},
		{"partial_solar_eclipse","solar_eclipse"},
	};
	auto it=kind_hints.find(b);
	if(it!=kind_hints.end()){
		next_args.push_back("--kinds");
		next_args.push_back(it->second);
	}
	return cmd_next(next_args);
}

