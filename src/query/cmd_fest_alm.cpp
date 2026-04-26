namespace{

struct FestOpt{
	std::string ephem;
	int year=0;
	std::string tz;
	std::string lunar_day_tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
};

struct FestRes{
	std::vector<EventRec> rows;
};

struct AlmOpt{
	std::string ephem;
	std::string date_text;
	std::string tz;
	std::string lunar_day_tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
	double hli_lon_deg=120.0;
	HliRuleSet hli_rules;
};

struct AlmRes{
	AtData atd;
	std::vector<EventRec> evs;
	std::vector<EventRec> fests;
};

FestOpt parse_fest(const std::vector<std::string>&args){
	if(args.size()<2){
		throw std::invalid_argument("festival requires: <bsp> <year>");
	}
	InterCfg cfg=load_def();
	FestOpt opt;
	opt.ephem=args[0];
	opt.year=parse_int(args[1],"year");
	opt.tz=cfg.default_tz;
	opt.lunar_day_tz=resolve_lunar_day_tz(cfg);
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"){
		opt.format="txt";
	}
	opt.pretty=cfg.def_prety;
	lunar::ArgParser parser;
	parser.add_value("--tz",[&](const std::string&v){ opt.tz=v; })
		.add_value("--lunar-day-tz",
				   [&](const std::string&v){ opt.lunar_day_tz=v; })
		.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; });
	for(std::size_t i=2;i<args.size();++i){
		if(!parser.parse_one(args,i,"festival")){
			throw std::invalid_argument("unknown option for festival: "+args[i]);
		}
	}
	chk_fmt(opt.format,{"json","txt","csv"},"festival");
	opt.lunar_day_tz=canonical_tz_text(opt.lunar_day_tz);
	return opt;
}

FestRes run_fest(const FestOpt&opt){
	FestRes res;
	int tz_off=parse_tz(opt.tz);
	int lunar_day_tz_off=parse_tz(opt.lunar_day_tz);
	EphRead eph(opt.ephem);
	QueryCache cache(eph);
	res.rows=bld_fest(eph,opt.year,tz_off,lunar_day_tz_off,&cache);
	return res;
}

void write_fest(std::ostream&os,const FestOpt&opt,const FestRes&res){
	EphRead eph(opt.ephem);
	const FmtMap fmts={
		{"json",[&](){
			 wr_eljs(os,opt.ephem,opt.tz,opt.pretty,res.rows,"festival",eph);
		 }},
		{"csv",[&](){ wr_elcsv(os,res.rows); }},
		{"txt",[&](){ wr_eltxt(os,opt.tz,res.rows,"festival"); }},
	};
	run_fmt(fmts,opt.format,"festival");
}

AlmOpt parse_alm(const std::vector<std::string>&args){
	if(args.size()<2){
		throw std::invalid_argument("almanac requires: <bsp> <YYYY-MM-DD>");
	}
	InterCfg cfg=load_def();
	AlmOpt opt;
	opt.ephem=args[0];
	opt.date_text=args[1];
	opt.tz=cfg.default_tz;
	opt.lunar_day_tz=resolve_lunar_day_tz(cfg);
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"){
		opt.format="txt";
	}
	opt.pretty=cfg.def_prety;
	opt.hli_rules=hli_rules_from_cfg(cfg);
	lunar::ArgParser parser;
	parser.add_value("--tz",[&](const std::string&v){ opt.tz=v; })
		.add_value("--lunar-day-tz",
				   [&](const std::string&v){ opt.lunar_day_tz=v; })
		.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; })
		.add_value("--lon",[&](const std::string&v){
			opt.hli_lon_deg=parse_double(v,"--lon");
		})
		.add_value("--trad",[&](const std::string&v){
			HliProfileCode trad=parse_hli_profile_arg(v,"--trad");
			opt.hli_rules=make_hli_rule_set(trad);
		})
		.add_value("--year-boundary",[&](const std::string&v){
			set_hli_year_boundary(opt.hli_rules,v);
		})
		.add_value("--month-boundary",[&](const std::string&v){
			set_hli_month_boundary(opt.hli_rules,v);
		})
		.add_value("--leap-month-mode",[&](const std::string&v){
			set_hli_leap_month_mode(opt.hli_rules,v);
		})
		.add_value("--day-boundary",[&](const std::string&v){
			set_hli_day_boundary(opt.hli_rules,v);
		});
	for(std::size_t i=2;i<args.size();++i){
		if(!parser.parse_one(args,i,"almanac")){
			throw std::invalid_argument("unknown option for almanac: "+args[i]);
		}
	}
	chk_fmt(opt.format,{"json","txt","csv"},"almanac");
	opt.lunar_day_tz=canonical_tz_text(opt.lunar_day_tz);
	opt.hli_rules=normalize_hli_rule_set(opt.hli_rules);
	return opt;
}

AlmRes run_alm(const AlmOpt&opt){
	int y=0;
	int m=0;
	int d=0;
	std::tie(y,m,d)=parse_ymd(opt.date_text);
	int tz_off=parse_tz(opt.tz);
	int lunar_day_tz_off=parse_tz(opt.lunar_day_tz);
	double smp_jdutc=civil_midjd(y,m,d,lunar_day_tz_off)+0.5;
	double day_sutc=civil_midjd(y,m,d,lunar_day_tz_off);
	double day_eutc=day_sutc+1.0;

	EphRead eph(opt.ephem);
	QueryCache cache(eph);
	AlmRes res;
	res.atd=at_fromjd(eph,smp_jdutc,tz_off,lunar_day_tz_off,opt.tz,
					  opt.date_text+"T12:00:00",opt.lunar_day_tz,false,false,
					  0.0,opt.hli_lon_deg,&opt.hli_rules,&cache);

	std::set<int> years={y-1,y,y+1};
	std::vector<EventRec> all=col_eyrs(eph,years,tz_off,opt.quiet?nullptr:&std::cerr);
	for(const auto&ev : all){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			res.evs.push_back(ev);
		}
	}

	std::vector<EventRec> f_all=
		bld_fest(eph,res.atd.lunar_date.lunar_year,tz_off,lunar_day_tz_off,&cache);
	for(const auto&ev : f_all){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			res.fests.push_back(ev);
		}
	}
	return res;
}

std::string join_ev_name(const std::vector<EventRec>&rows){
	std::string out;
	for(std::size_t i=0;i<rows.size();++i){
		if(i!=0){
			out.push_back('|');
		}
		out+=rows[i].name;
	}
	return out;
}

void write_alm(std::ostream&os,const AlmOpt&opt,const AlmRes&res){
	EphRead eph(opt.ephem);
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 write_meta(w,opt.ephem,opt.tz,
						{"type=almanac",lunar_day_rule_note(opt.lunar_day_tz)});
			 w.key("input");
			 w.obj_begin();
			 w.key("date");
			 w.value(opt.date_text);
			 w.key("lunar_day_tz");
			 w.value(opt.lunar_day_tz);
			 w.key("lon_deg");
			 w.value(opt.hli_lon_deg);
			 w.obj_end();
			 w.key("data");
			 w.obj_begin();
			 w.key("lunar_date");
			 wr_ljson(w,res.atd.lunar_date);
			 w.key("huangli");
			 wr_hli_json(w,res.atd.hli);
			 w.key("ill_pct");
			 w.value(res.atd.ill_pct);
			 w.key("phase_name");
			 w.value(res.atd.phase_name);
			 w.key("events");
			 w.arr_begin();
			 for(const auto&ev : res.evs){
				 wr_ejson(w,ev,eph);
			 }
			 w.arr_end();
			 w.key("festivals");
			 w.arr_begin();
			 for(const auto&ev : res.fests){
				 wr_ejson(w,ev,eph);
			 }
			 w.arr_end();
			 w.obj_end();
			 w.obj_end();
			 os<<"\n";
		 }},
		{"csv",[&](){
			 CsvWriter csv(os);
			 csv.write_field("date",opt.date_text);
			 csv.write_field("lun_label",res.atd.lunar_date.lun_label);
			 csv.write_raw("ill_pct",format_num(res.atd.ill_pct));
			 csv.write_field("phase_name",res.atd.phase_name);
			 csv.write_field("events",join_ev_name(res.evs));
			 csv.write_field("festivals",join_ev_name(res.fests));
			 wr_hli_csv(csv,res.atd.hli,HliCsvLayout::Almanac);
			 csv.finish_row();
		 }},
		{"txt",[&](){
			 os<<"tool=lunar format=txt type=almanac tz_display="<<opt.tz<<"\n";
			 os<<"input.date="<<opt.date_text<<"\n";
			 os<<"input.lunar_day_tz="<<opt.lunar_day_tz<<"\n";
			 os<<"input.lon_deg="<<format_num(opt.hli_lon_deg)<<"\n";
			 os<<"data.lun_label="<<res.atd.lunar_date.lun_label<<"\n";
			 wr_hli_txt(os,res.atd.hli,HliTxtLayout::Day);
			 os<<"data.ill_pct="<<format_num(res.atd.ill_pct)<<"\n";
			 os<<"data.phase_name="<<res.atd.phase_name<<"\n";
			 os<<"[events]\n";
			 for(const auto&ev : res.evs){
				 os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.loc_iso
				   <<"\n";
			 }
			 os<<"[festivals]\n";
			 for(const auto&ev : res.fests){
				 os<<ev.name<<"\t"<<ev.loc_iso<<"\n";
			 }
			 wr_hli_hour_txt(os,res.atd.hli);
		 }},
	};
	run_fmt(fmts,opt.format,"almanac");
}

}

int cmd_fest(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_fest();
		return 0;
	}
	FestOpt opt=parse_fest(args);
	FestRes res=run_fest(opt);
	OutTgt out=open_out(opt.out_path);
	write_fest(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}

int cmd_alm(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_alm();
		return 0;
	}
	AlmOpt opt=parse_alm(args);
	AlmRes res=run_alm(opt);
	OutTgt out=open_out(opt.out_path);
	write_alm(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}
