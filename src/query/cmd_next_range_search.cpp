namespace{

struct SearchQuerySpec{
	std::string kinds;
	std::string code;
};

struct EvOpt{
	std::string type;
	std::string ephem;
	std::string def_tz;
	std::string tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
	bool calc_ecl=false;
};

struct NextOpt : EvOpt{
	std::string from_time;
	int count=1;
	std::string kinds="solar_term,lunar_phase,lunar_eclipse,solar_eclipse";
};

struct RangeOpt : EvOpt{
	std::string from_time;
	std::string to_time;
	std::string kinds="solar_term,lunar_phase,lunar_eclipse,solar_eclipse";
};

struct SearchOpt : EvOpt{
	std::string query;
	std::string from_time;
	int count=1;
};

struct EvRes{
	std::vector<EventRec> rows;
};

template<typename Opt>
lunar::ArgParser& add_event_output_opts(lunar::ArgParser&parser,Opt&opt){
	return parser.add_value("--tz",[&](const std::string&v){ opt.tz=v; })
		.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; })
		.add_value("--eclipse",[&](const std::string&v){
			opt.calc_ecl=parse_bool01(v,"--eclipse");
		});
}

std::vector<EventRec> collect_next_events(EphRead&eph,double jd_utc_from,
										  int count,const EvtFilt&filter,
										  int tz_off,bool quiet,
										  const std::string&code_filter=""){
	if(count<1){
		throw std::invalid_argument("--count must be >=1");
	}
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2cst(jd_utc_from,cst_year,cst_month,cst_day);
	if(only_ecl_flt(filter)){
		return col_next_ecl(eph,jd_utc_from,cst_year,count,tz_off,
							quiet?nullptr:&std::cerr,filter,code_filter);
	}

	int span=1;
	std::vector<EventRec> picked;
	std::set<int> loaded_years;
	std::vector<EventRec> all;
	while(span<=8){
		std::set<int> years=
			add_years(loaded_years,cst_year,cst_year+span);
		merge_evs(
			all,col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr,filter));
		std::vector<EventRec> filtered=
			filt_evs(all,filter,jd_utc_from,
					 std::numeric_limits<double>::infinity(),false,true);
		if(!code_filter.empty()){
			filtered.erase(
				std::remove_if(filtered.begin(),filtered.end(),
							   [&](const EventRec&ev){
								   return ev.code!=code_filter;
							   }),
				filtered.end());
		}
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
	return picked;
}

void write_ev(std::ostream&os,const EvOpt&opt,const std::vector<EventRec>&rows){
	const int tz_off=parse_tz(opt.tz);
	EphRead eph(opt.ephem);
	const FmtMap fmts={
		{"json",[&](){
			 wr_eljs(os,opt.ephem,opt.tz,opt.pretty,rows,opt.type,eph,
					 opt.calc_ecl,tz_off);
		 }},
		{"txt",[&](){
			 wr_eltxt(os,opt.tz,rows,opt.type,&eph,opt.calc_ecl,tz_off);
		 }},
		{"csv",[&](){ wr_elcsv(os,rows,&eph,opt.calc_ecl,tz_off); }},
		{"jsonl",[&](){
			 wr_eljsl(os,opt.ephem,opt.tz,rows,opt.type,eph,opt.calc_ecl,
					  tz_off);
		 }},
		{"ics",[&](){ wr_elics(os,opt.ephem,"lunar-"+opt.type,rows); }},
	};
	run_fmt(fmts,opt.format,opt.type);
}

SearchQuerySpec resolve_search_query(const std::string&token){
	static const std::unordered_map<std::string,SearchQuerySpec> kSpecs={
		{"full_moon",{"lunar_phase","full_moon"}},
		{"new_moon",{"lunar_phase","new_moon"}},
		{"fst_qtr",{"lunar_phase","fst_qtr"}},
		{"lst_qtr",{"lunar_phase","lst_qtr"}},
		{"solar_term",{"solar_term",""}},
		{"lunar_phase",{"lunar_phase",""}},
		{"lunar_eclipse",{"lunar_eclipse",""}},
		{"lunar-eclipse",{"lunar_eclipse",""}},
		{"solar_eclipse",{"solar_eclipse",""}},
		{"solar-eclipse",{"solar_eclipse",""}},
		{"eclipse",{"lunar_eclipse,solar_eclipse",""}},
		{"total_eclipse",{"lunar_eclipse","total"}},
		{"partial_eclipse",{"lunar_eclipse","partial"}},
		{"penumbral_eclipse",{"lunar_eclipse","penumbral"}},
		{"total_solar_eclipse",{"solar_eclipse","total"}},
		{"annular_eclipse",{"solar_eclipse","annular"}},
		{"hybrid_eclipse",{"solar_eclipse","hybrid"}},
		{"partial_solar_eclipse",{"solar_eclipse","partial"}},
	};
	auto it=kSpecs.find(token);
	if(it==kSpecs.end()){
		return {};
	}
	return it->second;
}

std::string normalize_search_key(const std::string&query){
	std::istringstream iss(to_low(query));
	std::string head;
	if(!(iss>>head)||head!="next"){
		throw std::invalid_argument(
			"search currently supports query starting with 'next ...'");
	}
	std::string token;
	std::string key;
	while(iss>>token){
		if(!key.empty()){
			key.push_back('_');
		}
		key+=token;
	}
	if(key.empty()){
		throw std::invalid_argument("search requires a target after 'next'");
	}
	return key;
}

EvOpt mk_ev_opt(const InterCfg&cfg,const std::string&ephem,
				const std::string&type,const std::string&format){
	EvOpt opt;
	opt.type=type;
	opt.ephem=ephem;
	opt.def_tz=cfg.default_tz;
	opt.tz=cfg.default_tz;
	opt.format=to_low(format);
	opt.pretty=cfg.def_prety;
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"&&
	   opt.format!="ics"&&opt.format!="jsonl"){
		opt.format="txt";
	}
	return opt;
}

NextOpt parse_next(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument(
			"next requires: <bsp> --from <time> --count N");
	}
	InterCfg cfg=load_def();
	NextOpt opt;
	static_cast<EvOpt&>(opt)=mk_ev_opt(cfg,args[0],"next",cfg.def_fmt);
	lunar::ArgParser parser;
	parser.add_value("--from",[&](const std::string&v){ opt.from_time=v; })
		.add_value("--count",[&](const std::string&v){
			opt.count=parse_int(v,"--count");
		})
		.add_value("--kinds",[&](const std::string&v){ opt.kinds=v; });
	add_event_output_opts(parser,opt);
	parser.parse_all(args,1,"next");
	if(opt.from_time.empty()){
		throw std::invalid_argument("next requires --from <time>");
	}
	if(opt.count<1){
		throw std::invalid_argument("--count must be >=1");
	}
	chk_fmt(opt.format,{"json","txt","csv","ics","jsonl"},"next");
	return opt;
}

RangeOpt parse_range(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument(
			"range requires: <bsp> --from <time> --to <time>");
	}
	InterCfg cfg=load_def();
	RangeOpt opt;
	static_cast<EvOpt&>(opt)=mk_ev_opt(cfg,args[0],"range",cfg.def_fmt);
	lunar::ArgParser parser;
	parser.add_value("--from",[&](const std::string&v){ opt.from_time=v; })
		.add_value("--to",[&](const std::string&v){ opt.to_time=v; })
		.add_value("--kinds",[&](const std::string&v){ opt.kinds=v; });
	add_event_output_opts(parser,opt);
	parser.parse_all(args,1,"range");
	if(opt.from_time.empty()||opt.to_time.empty()){
		throw std::invalid_argument(
			"range requires --from <time> and --to <time>");
	}
	chk_fmt(opt.format,{"json","txt","csv","ics","jsonl"},"range");
	return opt;
}

SearchOpt parse_search(const std::vector<std::string>&args){
	if(args.size()<2){
		throw std::invalid_argument("search requires: <bsp> <query>");
	}
	InterCfg cfg=load_def();
	SearchOpt opt;
	static_cast<EvOpt&>(opt)=mk_ev_opt(cfg,args[0],"search",cfg.def_fmt);
	opt.query=args[1];
	lunar::ArgParser parser;
	parser.add_value("--from",[&](const std::string&v){ opt.from_time=v; })
		.add_value("--count",[&](const std::string&v){
			opt.count=parse_int(v,"--count");
		});
	add_event_output_opts(parser,opt);
	parser.parse_all(args,2,"search");
	if(opt.count<1){
		throw std::invalid_argument("--count must be >=1");
	}
	chk_fmt(opt.format,{"json","txt","csv","ics","jsonl"},"search");
	return opt;
}

EvRes run_next(NextOpt&opt){
	EvRes res;
	IsoTime parsed=parse_iso(opt.from_time,opt.def_tz);
	EvtFilt filter=parse_ef(opt.kinds);
	if(filter.inc_ecl){
		opt.calc_ecl=true;
	}
	int tz_off=parse_tz(opt.tz);
	EphRead eph(opt.ephem);
	res.rows=collect_next_events(eph,parsed.jd_utc,opt.count,filter,tz_off,
								 opt.quiet);
	return res;
}

EvRes run_range(RangeOpt&opt){
	EvRes res;
	IsoTime from_par=parse_iso(opt.from_time,opt.def_tz);
	IsoTime to_par=parse_iso(opt.to_time,opt.def_tz);
	if(to_par.jd_utc<from_par.jd_utc){
		throw std::invalid_argument("--to must be >= --from");
	}
	EvtFilt filter=parse_ef(opt.kinds);
	if(filter.inc_ecl){
		opt.calc_ecl=true;
	}
	int tz_off=parse_tz(opt.tz);
	EphRead eph(opt.ephem);
	res.rows=load_evs(eph,from_par.jd_utc,to_par.jd_utc,filter,tz_off,
					  opt.quiet,false);
	return res;
}

EvRes run_search(SearchOpt&opt){
	EvRes res;
	const std::string key=normalize_search_key(opt.query);
	SearchQuerySpec spec=resolve_search_query(key);
	if(spec.kinds.empty()){
		throw std::invalid_argument("unsupported search query: "+opt.query);
	}
	double jd_utc_from=current_jd_utc();
	if(!opt.from_time.empty()){
		jd_utc_from=parse_iso(opt.from_time,opt.def_tz).jd_utc;
	}
	EvtFilt filter=parse_ef(spec.kinds);
	if(filter.inc_ecl){
		opt.calc_ecl=true;
	}
	int tz_off=parse_tz(opt.tz);
	EphRead eph(opt.ephem);
	res.rows=collect_next_events(eph,jd_utc_from,opt.count,filter,tz_off,
								 opt.quiet,spec.code);
	return res;
}

}

int cmd_next(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_next();
		return 0;
	}
	NextOpt opt=parse_next(args);
	EvRes res=run_next(opt);
	OutTgt out=open_out(opt.out_path);
	write_ev(*out.stream,opt,res.rows);
	note_out(opt.out_path,opt.quiet);
	return 0;
}

int cmd_range(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_range();
		return 0;
	}
	RangeOpt opt=parse_range(args);
	EvRes res=run_range(opt);
	OutTgt out=open_out(opt.out_path);
	write_ev(*out.stream,opt,res.rows);
	note_out(opt.out_path,opt.quiet);
	return 0;
}

int cmd_search(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_search();
		return 0;
	}
	SearchOpt opt=parse_search(args);
	EvRes res=run_search(opt);
	OutTgt out=open_out(opt.out_path);
	write_ev(*out.stream,opt,res.rows);
	note_out(opt.out_path,opt.quiet);
	return 0;
}
