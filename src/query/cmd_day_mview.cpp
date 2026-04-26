namespace{

struct DayCmd{
	lunar::core::DayComputeOptions run;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
};

struct MvOpt{
	std::string ephem;
	std::string ym;
	std::string tz;
	std::string lunar_day_tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
	bool inc_astro=false;
	std::string astro_mode_text="less";
	std::string astro_pick_csv;
	AstroObs astro_obs;
};

struct MvRow{
	std::string greg_date;
	std::string lun_label;
	bool is_leap=false;
	std::string lun_mlab;
	double ill_pct=0.0;
	std::string moon_xg_region;
	std::string moon_xg_star;
	double moon_xg_sep_deg=std::numeric_limits<double>::quiet_NaN();
	std::string ev_sum;
	std::string astro_ev_sum;
};

struct MvRes{
	std::vector<MvRow> rows;
	StarPick astro_pick;
};

void wr_mv_csv(CsvWriter&w,const MvRow&row){
	w.write_field("greg_date",row.greg_date);
	w.write_field("lun_label",row.lun_label);
	w.write_field("is_leap",row.is_leap);
	w.write_field("lun_m_label",row.lun_mlab);
	w.write_raw("ill_pct",format_num(row.ill_pct));
	w.write_field("moon_xg_region",row.moon_xg_region);
	w.write_field("moon_xg_star",row.moon_xg_star);
	w.write_raw("moon_xg_sep_deg",format_num(row.moon_xg_sep_deg));
	w.write_field("ev_sum",row.ev_sum);
	w.write_field("astro_ev_sum",row.astro_ev_sum);
	w.finish_row();
}

DayCmd parse_day(const std::vector<std::string>&args){
	if(args.size()<2){
		throw std::invalid_argument("day requires: <bsp> <YYYY-MM-DD>");
	}
	InterCfg cfg=load_def();
	DayCmd cmd;
	cmd.run.ephem=args[0];
	cmd.run.date_text=args[1];
	cmd.run.tz=cfg.default_tz;
	cmd.run.lunar_day_tz=resolve_lunar_day_tz(cfg);
	cmd.format=to_low(cfg.def_fmt);
	if(cmd.format!="txt"&&cmd.format!="json"&&cmd.format!="csv"&&
	   cmd.format!="jsonl"){
		cmd.format="txt";
	}
	cmd.pretty=cfg.def_prety;
	cmd.run.at_time="12:00:00";
	cmd.run.include_events=true;
	cmd.run.include_astro=false;
	cmd.run.astro_mode_text="less";
	cmd.run.hli_lon_deg=120.0;
	cmd.run.hli_rules=hli_rules_from_cfg(cfg);

	AstroSiteOpt site;
	lunar::ArgParser parser;
	parser.add_value("--tz",[&](const std::string&v){ cmd.run.tz=v; });
	parser.add_value("--lunar-day-tz",
					 [&](const std::string&v){ cmd.run.lunar_day_tz=v; });
	parser.add_value("--format",
					 [&](const std::string&v){ cmd.format=to_low(v); });
	parser.add_value("--out",[&](const std::string&v){ cmd.out_path=v; });
	parser.add_value("--pretty",[&](const std::string&v){
		cmd.pretty=parse_bool01(v,"--pretty");
	});
	parser.add_flag("--quiet",[&](){ cmd.quiet=true; cmd.run.quiet=true; });
	parser.add_value("--at",[&](const std::string&v){ cmd.run.at_time=v; });
	parser.add_value("--events",[&](const std::string&v){
		cmd.run.include_events=parse_bool01(v,"--events");
	});
	parser.add_value("--lon",[&](const std::string&v){
		cmd.run.hli_lon_deg=parse_double(v,"--lon");
	});
	parser.add_value("--trad",[&](const std::string&v){
		HliProfileCode trad=parse_hli_profile_arg(v,"--trad");
		cmd.run.hli_rules=make_hli_rule_set(trad);
	});
	parser.add_value("--year-boundary",[&](const std::string&v){
		set_hli_year_boundary(cmd.run.hli_rules,v);
	});
	parser.add_value("--month-boundary",[&](const std::string&v){
		set_hli_month_boundary(cmd.run.hli_rules,v);
	});
	parser.add_value("--leap-month-mode",[&](const std::string&v){
		set_hli_leap_month_mode(cmd.run.hli_rules,v);
	});
	parser.add_value("--day-boundary",[&](const std::string&v){
		set_hli_day_boundary(cmd.run.hli_rules,v);
	});
	parser.add_value("--astro",[&](const std::string&v){
		cmd.run.include_astro=parse_bool01(v,"--astro");
	});
	parser.add_value("--astro-mode",
					 [&](const std::string&v){ cmd.run.astro_mode_text=v; });
	parser.add_value("--astro-pick",
					 [&](const std::string&v){ cmd.run.astro_pick_csv=v; });
	add_astro_site_options(parser,site);

	for(std::size_t i=2;i<args.size();++i){
		if(args[i]=="-h"||args[i]=="--help"){
			use_day();
			cmd.format.clear();
			return cmd;
		}
		if(!parser.parse_one(args,i,"day")){
			throw std::invalid_argument("unknown option for day: "+args[i]);
		}
	}
	validate_astro_site(site);
	chk_fmt(cmd.format,{"json","txt","csv","jsonl"},"day");
	cmd.run.lunar_day_tz=canonical_tz_text(cmd.run.lunar_day_tz);
	cmd.run.hli_rules=normalize_hli_rule_set(cmd.run.hli_rules);
	cmd.run.has_astro_site=site.has_lat;
	if(site.has_lat){
		cmd.run.astro_lat_deg=site.lat;
		cmd.run.astro_lon_deg=site.lon;
		cmd.run.astro_height_m=site.has_height?site.height:0.0;
	}
	return cmd;
}

DayResult run_day(const DayCmd&cmd){
	return lunar::core::compute_day(cmd.run);
}

void write_day(std::ostream&os,const DayCmd&cmd,const DayResult&res){
	lunar::core::format_day_output(os,res,cmd.format,cmd.pretty);
}

MvOpt parse_mview(const std::vector<std::string>&args){
	if(args.size()<2){
		throw std::invalid_argument("monthview requires: <bsp> <YYYY-MM>");
	}
	InterCfg cfg=load_def();
	MvOpt opt;
	opt.ephem=args[0];
	opt.ym=args[1];
	opt.tz=cfg.default_tz;
	opt.lunar_day_tz=resolve_lunar_day_tz(cfg);
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"){
		opt.format="txt";
	}
	opt.pretty=cfg.def_prety;

	AstroSiteOpt site;
	lunar::ArgParser parser;
	parser.add_value("--tz",[&](const std::string&v){ opt.tz=v; });
	parser.add_value("--lunar-day-tz",
					 [&](const std::string&v){ opt.lunar_day_tz=v; });
	parser.add_value("--format",
					 [&](const std::string&v){ opt.format=to_low(v); });
	parser.add_value("--out",[&](const std::string&v){ opt.out_path=v; });
	parser.add_value("--pretty",[&](const std::string&v){
		opt.pretty=parse_bool01(v,"--pretty");
	});
	parser.add_flag("--quiet",[&](){ opt.quiet=true; });
	parser.add_value("--astro",[&](const std::string&v){
		opt.inc_astro=parse_bool01(v,"--astro");
	});
	parser.add_value("--astro-mode",
					 [&](const std::string&v){ opt.astro_mode_text=v; });
	parser.add_value("--astro-pick",
					 [&](const std::string&v){ opt.astro_pick_csv=v; });
	add_astro_site_options(parser,site);

	for(std::size_t i=2;i<args.size();++i){
		if(args[i]=="-h"||args[i]=="--help"){
			use_mview();
			opt.format.clear();
			return opt;
		}
		if(!parser.parse_one(args,i,"monthview")){
			throw std::invalid_argument("unknown option for monthview: "+args[i]);
		}
	}
	validate_astro_site(site);
	chk_fmt(opt.format,{"json","txt","csv"},"monthview");
	opt.lunar_day_tz=canonical_tz_text(opt.lunar_day_tz);
	if(site.has_lat){
		opt.astro_obs.has_site=true;
		opt.astro_obs.lat_deg=site.lat;
		opt.astro_obs.lon_deg=site.lon;
		opt.astro_obs.h_m=site.has_height?site.height:0.0;
	}
	return opt;
}

MvRes run_mview(const MvOpt&opt){
	int year=0;
	int month=0;
	std::tie(year,month)=parse_ym(opt.ym);
	int tz_off=parse_tz(opt.tz);
	int lunar_day_tz_off=parse_tz(opt.lunar_day_tz);
	int n_days=days_gm(year,month);

	MvRes res;
	if(opt.inc_astro){
		StarMode mode=parse_star_mode(opt.astro_mode_text);
		res.astro_pick=make_star_pick(mode,opt.astro_pick_csv);
	}

	EphRead eph(opt.ephem);
	QueryCache cache(eph);
	std::set<int> years={year-1,year,year+1};
	std::vector<EventRec> events=
		col_eyrs(eph,years,tz_off,opt.quiet?nullptr:&std::cerr);
	std::map<int,std::vector<std::string>> day2ev;
	for(const auto&ev : events){
		int ey=0,em=0,ed=0;
		utc2civil(ev.jd_utc,lunar_day_tz_off,ey,em,ed);
		if(ey==year&&em==month){
			day2ev[ed].push_back(ev.name);
		}
	}

	std::map<int,std::vector<std::string>> day2astro;
	if(opt.inc_astro){
		int ny=year;
		int nm=month+1;
		if(nm>12){
			nm=1;
			++ny;
		}
		double st=civil_midjd(year,month,1,lunar_day_tz_off);
		double ed=civil_midjd(ny,nm,1,lunar_day_tz_off);
		std::vector<AstroEvt> astro=
			calc_astro_evt(eph,st,ed,res.astro_pick,opt.astro_obs);
		for(const auto&ev : astro){
			int ey=0,em=0,ed2=0;
			utc2civil(ev.jd_utc,lunar_day_tz_off,ey,em,ed2);
			if(ey==year&&em==month){
				day2astro[ed2].push_back(ev.name);
			}
		}
	}

	res.rows.reserve(static_cast<std::size_t>(n_days));
	for(int day=1;day<=n_days;++day){
		double smp=civil_midjd(year,month,day,lunar_day_tz_off)+0.5;
		AtData atd=at_fromjd(eph,smp,tz_off,lunar_day_tz_off,opt.tz,
							 ymd_str(year,month,day),opt.lunar_day_tz,false,
							 false,0.0,120.0,nullptr,&cache);
		res.rows.push_back(MvRow{
			ymd_str(year,month,day),
			atd.lunar_date.lun_label,
			atd.lunar_date.is_leap,
			atd.lunar_date.lun_mlab,
			atd.ill_pct,
			atd.moon_xg.region,
			atd.moon_xg.star_name,
			atd.moon_xg.sep_deg,
			join_pipe(day2ev[day]),
			join_pipe(day2astro[day]),
		});
	}
	return res;
}

void write_mview(std::ostream&os,const MvOpt&opt,const MvRes&res){
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 write_meta(w,opt.ephem,opt.tz,
						{"type=monthview",lunar_day_rule_note(opt.lunar_day_tz)});
			 w.key("input");
			 w.obj_begin();
			 w.key("month");
			 w.value(opt.ym);
			 w.key("lunar_day_tz");
			 w.value(opt.lunar_day_tz);
			 w.key("astro");
			 w.value(opt.inc_astro);
			 w.key("astro_mode");
			 if(opt.inc_astro){
				 w.value(opt.astro_mode_text);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_pick");
			 if(opt.inc_astro&&res.astro_pick.mode==StarMode::Pick){
				 w.value(opt.astro_pick_csv);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_site");
			 w.value(opt.astro_obs.has_site);
			 w.key("astro_lat_deg");
			 if(opt.astro_obs.has_site){
				 w.value(opt.astro_obs.lat_deg);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_lon_deg");
			 if(opt.astro_obs.has_site){
				 w.value(opt.astro_obs.lon_deg);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_height_m");
			 if(opt.astro_obs.has_site){
				 w.value(opt.astro_obs.h_m);
			 }else{
				 w.null_val();
			 }
			 w.obj_end();
			 w.key("data");
			 w.arr_begin();
			 for(const auto&row : res.rows){
				 w.obj_begin();
				 w.key("greg_date");
				 w.value(row.greg_date);
				 w.key("lun_label");
				 w.value(row.lun_label);
				 w.key("is_leap");
				 w.value(row.is_leap);
				 w.key("lun_mlab");
				 w.value(row.lun_mlab);
				 w.key("ill_pct");
				 w.value(row.ill_pct);
				 w.key("moon_xg_region");
				 w.value(row.moon_xg_region);
				 w.key("moon_xg_star");
				 w.value(row.moon_xg_star);
				 w.key("moon_xg_sep_deg");
				 w.value(row.moon_xg_sep_deg);
				 w.key("ev_sum");
				 w.value(row.ev_sum);
				 w.key("astro_ev_sum");
				 w.value(row.astro_ev_sum);
				 w.obj_end();
			 }
			 w.arr_end();
			 w.obj_end();
			 os<<"\n";
		 }},
		{"csv",[&](){
			 CsvWriter csv(os);
			 for(const auto&row : res.rows){
				 wr_mv_csv(csv,row);
			 }
		 }},
		{"txt",[&](){
			 os<<"tool=lunar format=txt type=monthview tz_display="<<opt.tz<<"\n";
			 os<<"input.month="<<opt.ym<<"\n";
			 os<<"input.lunar_day_tz="<<opt.lunar_day_tz<<"\n";
			 os<<"input.astro="<<(opt.inc_astro?"1":"0")<<"\n";
			 os<<"input.astro_mode="<<opt.astro_mode_text<<"\n";
			 os<<"input.astro_pick="<<opt.astro_pick_csv<<"\n";
			 os<<"input.astro_site="<<(opt.astro_obs.has_site?"1":"0")<<"\n";
			 os<<"input.astro_lat_deg="
			   <<(opt.astro_obs.has_site?format_num(opt.astro_obs.lat_deg):"null")
			   <<"\n";
			 os<<"input.astro_lon_deg="
			   <<(opt.astro_obs.has_site?format_num(opt.astro_obs.lon_deg):"null")
			   <<"\n";
			 os<<"input.astro_height_m="
			   <<(opt.astro_obs.has_site?format_num(opt.astro_obs.h_m):"null")
			   <<"\n";
			 os<<"greg_date\tlunar_date_label\tis_leap\tlunar_month_label\t"
				 "ill_pct\tmoon_xg_region\tmoon_xg_star\tmoon_xg_sep_deg\t"
				 "events_summary\tastro_events_summary\n";
			 for(const auto&row : res.rows){
				 os<<row.greg_date<<"\t"<<row.lun_label<<"\t"
				   <<(row.is_leap?"1":"0")<<"\t"<<row.lun_mlab<<"\t"
				   <<format_num(row.ill_pct)<<"\t"<<row.moon_xg_region<<"\t"
				   <<row.moon_xg_star<<"\t"<<format_num(row.moon_xg_sep_deg)
				   <<"\t"<<row.ev_sum<<"\t"<<row.astro_ev_sum<<"\n";
			 }
		 }},
	};
	run_fmt(fmts,opt.format,"monthview");
}

}

int cmd_day(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_day();
		return 0;
	}
	DayCmd cmd=parse_day(args);
	if(cmd.format.empty()){
		return 0;
	}
	DayResult res=run_day(cmd);
	OutTgt out=open_out(cmd.out_path);
	write_day(*out.stream,cmd,res);
	note_out(cmd.out_path,cmd.quiet);
	return 0;
}

int cmd_mview(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_mview();
		return 0;
	}
	MvOpt opt=parse_mview(args);
	if(opt.format.empty()){
		return 0;
	}
	MvRes res=run_mview(opt);
	OutTgt out=open_out(opt.out_path);
	write_mview(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}
