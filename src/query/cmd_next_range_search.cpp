namespace{

struct SearchQuerySpec{
	std::string kinds;
	std::string code;
};

double current_jd_utc(){
	const std::time_t now=std::time(nullptr);
	std::tm utc_tm{};
#if defined(_WIN32)
	gmtime_s(&utc_tm,&now);
#else
	gmtime_r(&now,&utc_tm);
#endif
	return greg2jd(utc_tm.tm_year+1900,utc_tm.tm_mon+1,utc_tm.tm_mday,
				   utc_tm.tm_hour,utc_tm.tm_min,
				   static_cast<double>(utc_tm.tm_sec));
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

	int span=1;
	std::vector<EventRec> picked;
	while(span<=8){
		std::set<int> years;
		for(int y=cst_year-span;y<=cst_year+span;++y){
			years.insert(y);
		}
		std::vector<EventRec> all=
			col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr,filter.inc_ecl);
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

void write_event_output(std::ostream&os,const std::string&format,
						const std::string&ephem,const std::string&tz,bool pretty,
						const std::vector<EventRec>&events,
						const std::string&type,EphRead&eph,bool calc_eclipse,
						int tz_off){
	const FmtMap fmt_handlers={
		{"json",[&](){
			 wr_eljs(os,ephem,tz,pretty,events,type,eph,calc_eclipse,tz_off);
		 }},
		{"txt",[&](){
			 wr_eltxt(os,tz,events,type,&eph,calc_eclipse,tz_off);
		 }},
		{"csv",[&](){ wr_elcsv(os,events,&eph,calc_eclipse,tz_off); }},
		{"jsonl",[&](){
			 wr_eljsl(os,ephem,tz,events,type,eph,calc_eclipse,tz_off);
		 }},
		{"ics",[&](){ wr_elics(os,ephem,"lunar-"+type,events); }},
	};
	run_fmt(fmt_handlers,format,type);
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

}

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
	if(filter.inc_ecl){
		calc_eclipse=true;
	}
	int tz_off=parse_tz(tz);

	EphRead eph(ephem);
	std::vector<EventRec> picked=
		collect_next_events(eph,parsed.jd_utc,count,filter,tz_off,quiet);

	OutTgt out=open_out(out_path);
	write_event_output(*out.stream,format,ephem,tz,pretty,picked,"next",eph,
					   calc_eclipse,tz_off);
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

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string query=args[1];
	std::string from_time;
	int count=1;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"&&format!="ics"&&
	   format!="jsonl"){
		format="txt";
	}
	std::string tz=cfg.default_tz;
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

	chk_fmt(format,{"json","txt","csv","ics","jsonl"},"search");

	const std::string key=normalize_search_key(query);
	SearchQuerySpec spec=resolve_search_query(key);
	if(spec.kinds.empty()){
		throw std::invalid_argument("unsupported search query: "+query);
	}
	std::string kinds=spec.kinds;
	double jd_utc_from=current_jd_utc();
	if(!from_time.empty()){
		jd_utc_from=parse_iso(from_time,cfg.default_tz).jd_utc;
	}
	EvtFilt filter=parse_ef(kinds);
	if(filter.inc_ecl){
		calc_eclipse=true;
	}
	int tz_off=parse_tz(tz);
	EphRead eph(ephem);
	std::vector<EventRec> picked=collect_next_events(
		eph,jd_utc_from,count,filter,tz_off,quiet,spec.code);

	OutTgt out=open_out(out_path);
	write_event_output(*out.stream,format,ephem,tz,pretty,picked,"search",eph,
					   calc_eclipse,tz_off);
	note_out(out_path,quiet);
	return 0;
}

