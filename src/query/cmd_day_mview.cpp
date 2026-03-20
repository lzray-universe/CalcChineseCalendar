int cmd_day(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_day();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("day requires: <bsp> <YYYY-MM-DD>");
	}

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string date_text=args[1];
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"&&format!="jsonl"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	std::string at_time="12:00:00";
	bool inc_ev=true;
	bool inc_astro=false;
	std::string astro_mode_text="less";
	std::string astro_pick_csv;
	double astro_lat_deg=0.0;
	double astro_lon_deg=0.0;
	double astro_h_m=0.0;
	bool has_astro_lat=false;
	bool has_astro_lon=false;
	bool has_astro_h=false;
	double hli_lon_deg=120.0;
	HliRuleSet hli_rules=hli_rules_from_cfg(cfg);
	lunar::ArgParser parser;
	parser.add_value("--tz",[&](const std::string&value){ tz=value; });
	parser.add_value("--format",
					 [&format](const std::string&value){ format=to_low(value); });
	parser.add_value("--out",[&](const std::string&value){ out_path=value; });
	parser.add_value(
		"--pretty",[&](const std::string&value){
			pretty=parse_bool01(value,"--pretty");
		});
	parser.add_flag("--quiet",[&](){ quiet=true; });
	parser.add_value("--at",[&](const std::string&value){ at_time=value; });
	parser.add_value(
		"--events",[&](const std::string&value){
			inc_ev=parse_bool01(value,"--events");
		});
	parser.add_value(
		"--lon",[&](const std::string&value){
			hli_lon_deg=parse_double(value,"--lon");
		});
	parser.add_value("--trad",[&](const std::string&value){
		HliProfileCode trad=HliProfileCode::Folk;
		if(!parse_hli_profile(value,&trad)){
			throw std::invalid_argument(
				"invalid --trad: "+value+
				" (expected folk|ziping|purple|xieji)");
		}
		hli_rules=make_hli_rule_set(trad);
	});
	parser.add_value("--year-boundary",[&](const std::string&value){
		HliYearBoundary parsed=HliYearBoundary::LunarNewYear;
		if(!parse_hli_year_boundary(value,&parsed)){
			throw std::invalid_argument(
				"invalid --year-boundary: "+value+
				" (expected lichun|lunar_new_year|dongzhi)");
		}
		hli_rules.year_boundary=static_cast<int>(parsed);
	});
	parser.add_value("--month-boundary",[&](const std::string&value){
		HliMonthBoundary parsed=HliMonthBoundary::LunarFirstDay;
		if(!parse_hli_month_boundary(value,&parsed)){
			throw std::invalid_argument(
				"invalid --month-boundary: "+value+
				" (expected solar_term|lunar_first_day)");
		}
		hli_rules.month_boundary=static_cast<int>(parsed);
	});
	parser.add_value("--leap-month-mode",[&](const std::string&value){
		HliLeapMonthMode parsed=HliLeapMonthMode::InheritPrevious;
		if(!parse_hli_leap_month_mode(value,&parsed)){
			throw std::invalid_argument(
				"invalid --leap-month-mode: "+value+
				" (expected ignore|inherit_previous|split_midway|shift_to_next)");
		}
		hli_rules.leap_month_mode=static_cast<int>(parsed);
	});
	parser.add_value("--day-boundary",[&](const std::string&value){
		HliDayBoundary parsed=HliDayBoundary::Hour23;
		if(!parse_hli_day_boundary(value,&parsed)){
			throw std::invalid_argument(
				"invalid --day-boundary: "+value+
				" (expected hour23|hour0)");
		}
		hli_rules.day_boundary=static_cast<int>(parsed);
	});
	parser.add_value(
		"--astro",[&](const std::string&value){
			inc_astro=parse_bool01(value,"--astro");
		});
	parser.add_value("--astro-mode",
					 [&](const std::string&value){ astro_mode_text=value; });
	parser.add_value("--astro-pick",
					 [&](const std::string&value){ astro_pick_csv=value; });
	parser.add_value(
		"--astro-lat",[&](const std::string&value){
			astro_lat_deg=parse_double(value,"--astro-lat");
			has_astro_lat=true;
		});
	parser.add_value(
		"--astro-lon",[&](const std::string&value){
			astro_lon_deg=parse_double(value,"--astro-lon");
			has_astro_lon=true;
		});
	parser.add_value(
		"--astro-height",[&](const std::string&value){
			astro_h_m=parse_double(value,"--astro-height");
			has_astro_h=true;
		});

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="-h"||opt=="--help"){
			use_day();
			return 0;
		}
		if(!parser.parse_one(args,i,"day")){
			throw std::invalid_argument("unknown option for day: "+opt);
		}
	}
	if(has_astro_lat!=has_astro_lon){
		throw std::invalid_argument(
			"astro site requires both --astro-lat and --astro-lon");
	}
	if(has_astro_h&&!has_astro_lat){
		throw std::invalid_argument(
			"--astro-height requires --astro-lat and --astro-lon");
	}
	chk_fmt(format,{"json","txt","csv","jsonl"},"day");

	lunar::core::DayComputeOptions opt;
	opt.ephem=ephem;
	opt.date_text=date_text;
	opt.at_time=at_time;
	opt.tz=tz;
	opt.quiet=quiet;
	opt.include_events=inc_ev;
	opt.include_astro=inc_astro;
	opt.astro_mode_text=astro_mode_text;
	opt.astro_pick_csv=astro_pick_csv;
	opt.hli_lon_deg=hli_lon_deg;
	opt.hli_rules=normalize_hli_rule_set(hli_rules);
	opt.has_astro_site=has_astro_lat;
	if(has_astro_lat){
		opt.astro_lat_deg=astro_lat_deg;
		opt.astro_lon_deg=astro_lon_deg;
		opt.astro_height_m=has_astro_h?astro_h_m:0.0;
	}

	DayResult day=lunar::core::compute_day(opt);
	OutTgt out=open_out(out_path);
	lunar::core::format_day_output(*out.stream,day,format,pretty);
	note_out(out_path,quiet);
	return 0;
}

int cmd_mview(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_mview();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("monthview requires: <bsp> <YYYY-MM>");
	}

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string ym=args[1];
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	bool inc_astro=false;
	std::string astro_mode_text="less";
	std::string astro_pick_csv;
	double astro_lat_deg=0.0;
	double astro_lon_deg=0.0;
	double astro_h_m=0.0;
	bool has_astro_lat=false;
	bool has_astro_lon=false;
	bool has_astro_h=false;
	lunar::ArgParser parser;
	parser.add_value("--tz",[&](const std::string&value){ tz=value; });
	parser.add_value("--format",
					 [&format](const std::string&value){ format=to_low(value); });
	parser.add_value("--out",[&](const std::string&value){ out_path=value; });
	parser.add_value(
		"--pretty",[&](const std::string&value){
			pretty=parse_bool01(value,"--pretty");
		});
	parser.add_flag("--quiet",[&](){ quiet=true; });
	parser.add_value(
		"--astro",[&](const std::string&value){
			inc_astro=parse_bool01(value,"--astro");
		});
	parser.add_value("--astro-mode",
					 [&](const std::string&value){ astro_mode_text=value; });
	parser.add_value("--astro-pick",
					 [&](const std::string&value){ astro_pick_csv=value; });
	parser.add_value(
		"--astro-lat",[&](const std::string&value){
			astro_lat_deg=parse_double(value,"--astro-lat");
			has_astro_lat=true;
		});
	parser.add_value(
		"--astro-lon",[&](const std::string&value){
			astro_lon_deg=parse_double(value,"--astro-lon");
			has_astro_lon=true;
		});
	parser.add_value(
		"--astro-height",[&](const std::string&value){
			astro_h_m=parse_double(value,"--astro-height");
			has_astro_h=true;
		});

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="-h"||opt=="--help"){
			use_mview();
			return 0;
		}
		if(!parser.parse_one(args,i,"monthview")){
			throw std::invalid_argument("unknown option for monthview: "+opt);
		}
	}
	if(has_astro_lat!=has_astro_lon){
		throw std::invalid_argument(
			"astro site requires both --astro-lat and --astro-lon");
	}
	if(has_astro_h&&!has_astro_lat){
		throw std::invalid_argument(
			"--astro-height requires --astro-lat and --astro-lon");
	}
	chk_fmt(format,{"json","txt","csv"},"monthview");
	int year=0;
	int month=0;
	std::tie(year,month)=parse_ym(ym);

	int tz_off=parse_tz(tz);
	int n_days=days_gm(year,month);
	StarPick astro_pick;
	if(inc_astro){
		StarMode mode=parse_star_mode(astro_mode_text);
		astro_pick=make_star_pick(mode,astro_pick_csv);
	}
	AstroObs astro_obs;
	if(has_astro_lat){
		astro_obs.has_site=true;
		astro_obs.lat_deg=astro_lat_deg;
		astro_obs.lon_deg=astro_lon_deg;
		astro_obs.h_m=has_astro_h?astro_h_m:0.0;
	}
	EphRead eph(ephem);
	QueryCache cache(eph);

	std::set<int> years={year-1,year,year+1};
	std::vector<EventRec> events=
		col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
	std::map<int,std::vector<std::string>> day2ev;
	for(const auto&ev : events){
		int ey=0,em=0,ed=0;
		utc2cst(ev.jd_utc,ey,em,ed);
		if(ey==year&&em==month){
			day2ev[ed].push_back(ev.name);
		}
	}
	std::map<int,std::vector<std::string>> day2astro;
	if(inc_astro){
		int n_year=year;
		int n_month=month+1;
		if(n_month>12){
			n_month=1;
			++n_year;
		}
		double month_sutc=cst_midjd(year,month,1);
		double month_eutc=cst_midjd(n_year,n_month,1);
		std::vector<AstroEvt> astro=
			calc_astro_evt(eph,month_sutc,month_eutc,astro_pick,astro_obs);
		for(const auto&ev : astro){
			int ey=0;
			int em=0;
			int ed=0;
			utc2cst(ev.jd_utc,ey,em,ed);
			if(ey==year&&em==month){
				day2astro[ed].push_back(ev.name);
			}
		}
	}

	struct Row{
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
	std::vector<Row> rows;
	rows.reserve(static_cast<std::size_t>(n_days));
	for(int d=1;d<=n_days;++d){
		double smp_jdutc=greg2jd(year,month,d,12,0,0.0)-UTC8DAY;
		AtData atd=at_fromjd(eph,smp_jdutc,tz_off,tz,ymd_str(year,month,d),
							 "+08:00",false,false,0.0,120.0,nullptr,&cache);
		std::vector<std::string> ev_names;
		auto it=day2ev.find(d);
		if(it!=day2ev.end()){
			ev_names=it->second;
		}
		std::vector<std::string> astro_names;
		auto ita=day2astro.find(d);
		if(ita!=day2astro.end()){
			astro_names=ita->second;
		}
		rows.push_back(Row{ymd_str(year,month,d),atd.lunar_date.lun_label,
						   atd.lunar_date.is_leap,atd.lunar_date.lun_mlab,
						   atd.ill_pct,atd.moon_xg.region,atd.moon_xg.star_name,
						   atd.moon_xg.sep_deg,join_pipe(ev_names),
						   join_pipe(astro_names)});
	}

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,pretty);
			 w.obj_begin();
			 write_meta(w,ephem,tz,{"type=monthview",lunar::i18n::day_rule_note()});
			 w.key("input");
			 w.obj_begin();
			 w.key("month");
			 w.value(ym);
			 w.key("astro");
			 w.value(inc_astro);
			 w.key("astro_mode");
			 if(inc_astro){
				 w.value(astro_mode_text);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_pick");
			 if(inc_astro&&astro_pick.mode==StarMode::Pick){
				 w.value(astro_pick_csv);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_site");
			 w.value(astro_obs.has_site);
			 w.key("astro_lat_deg");
			 if(astro_obs.has_site){
				 w.value(astro_obs.lat_deg);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_lon_deg");
			 if(astro_obs.has_site){
				 w.value(astro_obs.lon_deg);
			 }else{
				 w.null_val();
			 }
			 w.key("astro_height_m");
			 if(astro_obs.has_site){
				 w.value(astro_obs.h_m);
			 }else{
				 w.null_val();
			 }
			 w.obj_end();
			 w.key("data");
			 w.arr_begin();
			 for(const auto&row : rows){
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
			 *out.stream<<"\n";
		 }},
		{"csv",[&](){
			 *out.stream<<"greg_date,lun_label,is_leap,lun_m_"
						  "label,ill_pct,moon_xg_region,moon_xg_star,"
						  "moon_xg_sep_deg,ev_sum,astro_ev_sum\n";
			 for(const auto&row : rows){
				 *out.stream<<csv_quote(row.greg_date)<<","
						   <<csv_quote(row.lun_label)<<","
						   <<(row.is_leap?"1":"0")<<","
						   <<csv_quote(row.lun_mlab)<<","
						   <<format_num(row.ill_pct)<<","
						   <<csv_quote(row.moon_xg_region)<<","
						   <<csv_quote(row.moon_xg_star)<<","
						   <<format_num(row.moon_xg_sep_deg)<<","
						   <<csv_quote(row.ev_sum)<<","
						   <<csv_quote(row.astro_ev_sum)<<"\n";
			 }
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=monthview tz_display="<<tz<<"\n";
			 os<<"input.month="<<ym<<"\n";
			 os<<"input.astro="<<(inc_astro?"1":"0")<<"\n";
			 os<<"input.astro_mode="<<astro_mode_text<<"\n";
			 os<<"input.astro_pick="<<astro_pick_csv<<"\n";
			 os<<"input.astro_site="<<(astro_obs.has_site?"1":"0")<<"\n";
			 os<<"input.astro_lat_deg="
			   <<(astro_obs.has_site?format_num(astro_obs.lat_deg):"null")
			   <<"\n";
			 os<<"input.astro_lon_deg="
			   <<(astro_obs.has_site?format_num(astro_obs.lon_deg):"null")
			   <<"\n";
			 os<<"input.astro_height_m="
			   <<(astro_obs.has_site?format_num(astro_obs.h_m):"null")
			   <<"\n";
			 os<<"greg_date\tlunar_date_label\tis_leap\tlunar_month_"
				 "label\till_pct\tmoon_xg_region\tmoon_xg_star\tmoon_xg_sep_"
				 "deg\tevents_summary\tastro_events_summary\n";
			 for(const auto&row : rows){
				 os<<row.greg_date<<"\t"<<row.lun_label<<"\t"
				   <<(row.is_leap?"1":"0")<<"\t"<<row.lun_mlab<<"\t"
				   <<format_num(row.ill_pct)<<"\t"<<row.moon_xg_region<<"\t"
				   <<row.moon_xg_star<<"\t"<<format_num(row.moon_xg_sep_deg)
				   <<"\t"<<row.ev_sum<<"\t"<<row.astro_ev_sum<<"\n";
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"monthview");
	note_out(out_path,quiet);
	return 0;
}

