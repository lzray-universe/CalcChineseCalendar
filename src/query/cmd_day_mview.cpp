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
	const OptMap handlers={
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
		{"--at",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ at_time=req_val(src,idx,opt); }},
		{"--events",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 inc_ev=parse_bool01(req_val(src,idx,opt),"--events");
		 }},
		{"--lon",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){
			 hli_lon_deg=parse_double(req_val(src,idx,opt),opt);
		 }},
		{"--astro",[&](const std::vector<std::string>&src,std::size_t&idx,
					   const std::string&opt){
			 inc_astro=parse_bool01(req_val(src,idx,opt),"--astro");
		 }},
		{"--astro-mode",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 astro_mode_text=req_val(src,idx,opt);
		 }},
		{"--astro-pick",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 astro_pick_csv=req_val(src,idx,opt);
		 }},
		{"--astro-lat",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 astro_lat_deg=parse_double(req_val(src,idx,opt),opt);
			 has_astro_lat=true;
		 }},
		{"--astro-lon",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 astro_lon_deg=parse_double(req_val(src,idx,opt),opt);
			 has_astro_lon=true;
		 }},
		{"--astro-height",[&](const std::vector<std::string>&src,
							  std::size_t&idx,const std::string&opt){
			 astro_h_m=parse_double(req_val(src,idx,opt),opt);
			 has_astro_h=true;
		 }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"day");
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

	int y=0,m=0,d=0;
	std::tie(y,m,d)=parse_ymd(date_text);
	int hh=12;
	int mm=0;
	double ss=0.0;
	parse_hms(at_time,hh,mm,ss);

	int tz_off=parse_tz(tz);
	double smp_jdutc=greg2jd(y,m,d,hh,mm,ss)-UTC8DAY;
	double day_sutc=cst_midjd(y,m,d);
	double day_eutc=day_sutc+1.0;
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
	AtData atd=
		at_fromjd(eph,smp_jdutc,tz_off,tz,date_text+"T"+at_time,"+08:00",false,
				  false,0.0,hli_lon_deg,&cache);

	std::vector<EventRec> day_events;
	if(inc_ev){
		std::set<int> years={y-1,y,y+1};
		std::vector<EventRec> all=
			col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
		for(const auto&ev : all){
			if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
				day_events.push_back(ev);
			}
		}
		std::sort(
			day_events.begin(),day_events.end(),
			[](const EventRec&a,const EventRec&b){ return a.jd_utc<b.jd_utc; });
	}
	std::vector<EventRec> astro_events;
	if(inc_astro){
		std::vector<AstroEvt> raw=
			calc_astro_evt(eph,day_sutc,day_eutc,astro_pick,astro_obs);
		astro_events.reserve(raw.size());
		for(const auto&ev : raw){
			astro_events.push_back(mk_astro_rec(ev,tz_off));
		}
	}

	OutTgt out=open_out(out_path);
	auto write_json=[&](bool json_pretty){
		JsonWriter w(*out.stream,json_pretty);
		w.obj_begin();
		write_meta(w,ephem,tz,{"type=day","农历判日固定UTC+8"});
		w.key("input");
		w.obj_begin();
		w.key("date");
		w.value(date_text);
		w.key("smp_time");
		w.value(at_time);
		w.key("smp_jdutc");
		w.value(smp_jdutc);
		w.key("lon_deg");
		w.value(hli_lon_deg);
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
		w.obj_begin();
		w.key("lunar_date");
		wr_ljson(w,atd.lunar_date);
		w.key("huangli");
		wr_hli_json(w,atd.hli);
		w.key("ill_pct");
		w.value(atd.ill_pct);
		w.key("phase_name");
		w.value(atd.phase_name);
		w.key("moon_xg");
		w.obj_begin();
		w.key("region");
		w.value(atd.moon_xg.region);
		w.key("star_id");
		w.value(atd.moon_xg.star_id);
		w.key("star_name");
		w.value(atd.moon_xg.star_name);
		w.key("sep_deg");
		w.value(atd.moon_xg.sep_deg);
		w.obj_end();
		w.key("smp_uiso");
		w.value(atd.utc_iso);
		w.key("smp_liso");
		w.value(atd.local_iso);
		w.key("events");
		w.arr_begin();
		for(const auto&ev : day_events){
			wr_ejson(w,ev,eph);
		}
		w.arr_end();
		w.key("astro_events");
		w.arr_begin();
		for(const auto&ev : astro_events){
			wr_ejson(w,ev,eph);
		}
		w.arr_end();
		w.obj_end();
		w.obj_end();
		*out.stream<<"\n";
	};
	const FmtMap fmt_handlers={
		{"json",[&](){ write_json(pretty); }},
		{"jsonl",[&](){ write_json(false); }},
		{"csv",[&](){
			 std::vector<std::string> ev_names;
			 ev_names.reserve(day_events.size());
			 for(const auto&ev : day_events){
				 ev_names.push_back(ev.name);
			 }
			 std::vector<std::string> astro_names;
			 astro_names.reserve(astro_events.size());
			 for(const auto&ev : astro_events){
				 astro_names.push_back(ev.name);
			 }
			 std::string summary=join_pipe(ev_names);
			 std::string astro_summary=join_pipe(astro_names);
			 *out.stream<<"date,lun_label,lun_mlab,is_leap,lunar_"
						  "day,ill_pct,phase_name,moon_xg_region,moon_xg_star,"
						  "moon_xg_sep_deg,smp_tlociso,ev_sum,astro_ev_sum,"
						  "y_lun_gz,y_lchun_gz,m_gz,d_gz,h_gz,h_true_gz,jianchu,"
						  "bazi_clock,bazi_true,duty_god,duty_tag,clash,"
						  "chong_sha,zodiac_day,six_he,"
						  "three_he,pengzu,nayin,wuxing_day,fetal_god,meridian,"
						  "lucky_dir,wealth_dir,mascot_dir,sun_noble_dir,"
						  "moon_noble_dir,xiu28,xiu_star,yi_ji_level,yi_ji_rule,"
						  "good_gods,bad_gods,yi,ji\n";
			 *out.stream<<csv_quote(date_text)<<","
						<<csv_quote(atd.lunar_date.lun_label)<<","
						<<csv_quote(atd.lunar_date.lun_mlab)<<","
						<<(atd.lunar_date.is_leap?"1":"0")<<","
						<<atd.lunar_date.lunar_day<<","<<format_num(atd.ill_pct)
						<<","<<csv_quote(atd.phase_name)<<","
						<<csv_quote(atd.moon_xg.region)<<","
						<<csv_quote(atd.moon_xg.star_name)<<","
						<<format_num(atd.moon_xg.sep_deg)<<","
						<<csv_quote(atd.local_iso)<<","<<csv_quote(summary)<<","
						<<csv_quote(astro_summary)<<","
						<<csv_quote(atd.hli.y_lun.text)<<","
						<<csv_quote(atd.hli.y_lchun.text)<<","
						<<csv_quote(atd.hli.m_gz.text)<<","
						<<csv_quote(atd.hli.d_gz.text)<<","
						<<csv_quote(atd.hli.h_gz.text)<<","
						<<csv_quote(atd.hli.h_gz_true.text)<<","
						<<csv_quote(atd.hli.jianchu)<<","
						<<csv_quote(atd.hli.bazi_clock)<<","
						<<csv_quote(atd.hli.bazi_true)<<","
						<<csv_quote(atd.hli.duty_god)<<","
						<<csv_quote(atd.hli.duty_tag)<<","
						<<csv_quote(atd.hli.clash)<<","
						<<csv_quote(atd.hli.chong_sha)<<","
						<<csv_quote(atd.hli.zodiac_day)<<","
						<<csv_quote(atd.hli.six_he)<<","
						<<csv_quote(atd.hli.three_he)<<","
						<<csv_quote(atd.hli.pengzu)<<","
						<<csv_quote(atd.hli.nayin)<<","
						<<csv_quote(atd.hli.wx_day)<<","
						<<csv_quote(atd.hli.fetal_god)<<","
						<<csv_quote(atd.hli.meridian)<<","
						<<csv_quote(atd.hli.lucky_dir)<<","
						<<csv_quote(atd.hli.wealth_dir)<<","
						<<csv_quote(atd.hli.mascot_dir)<<","
						<<csv_quote(atd.hli.sun_noble_dir)<<","
						<<csv_quote(atd.hli.moon_noble_dir)<<","
						<<csv_quote(atd.hli.xiu28)<<","
						<<csv_quote(atd.hli.xiu_id)<<","
						<<atd.hli.yi_ji_level<<","
						<<csv_quote(atd.hli.yi_ji_rule)<<","
						<<csv_quote(join_pipe(atd.hli.good_gods))<<","
						<<csv_quote(join_pipe(atd.hli.bad_gods))<<","
						<<csv_quote(join_pipe(atd.hli.yi))<<","
						<<csv_quote(join_pipe(atd.hli.ji))<<"\n";
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=day tz_display="<<tz<<"\n";
			 os<<"input.date="<<date_text<<"\n";
			 os<<"input.smp_time="<<at_time<<"\n";
			 os<<"input.lon_deg="<<format_num(hli_lon_deg)<<"\n";
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
			 os<<"data.lun_label="<<atd.lunar_date.lun_label<<"\n";
			 os<<"data.ill_pct="<<format_num(atd.ill_pct)<<"\n";
			 os<<"data.phase_name="<<atd.phase_name<<"\n";
			 os<<"data.moon_xg.region="<<atd.moon_xg.region<<"\n";
			 os<<"data.moon_xg.star_id="<<atd.moon_xg.star_id<<"\n";
			 os<<"data.moon_xg.star_name="<<atd.moon_xg.star_name<<"\n";
			 os<<"data.moon_xg.sep_deg="<<format_num(atd.moon_xg.sep_deg)<<"\n";
			 os<<"data.hli.y_lun="<<atd.hli.y_lun.text<<"\n";
			 os<<"data.hli.y_lchun="<<atd.hli.y_lchun.text<<"\n";
			 os<<"data.hli.month="<<atd.hli.m_gz.text<<"\n";
			 os<<"data.hli.day="<<atd.hli.d_gz.text<<"\n";
			 os<<"data.hli.hour="<<atd.hli.h_gz.text<<"\n";
			 os<<"data.hli.hour_true="<<atd.hli.h_gz_true.text<<"\n";
			 os<<"data.hli.bazi_clock="<<atd.hli.bazi_clock<<"\n";
			 os<<"data.hli.bazi_true="<<atd.hli.bazi_true<<"\n";
			 os<<"data.hli.jianchu="<<atd.hli.jianchu<<"\n";
			 os<<"data.hli.duty_god="<<atd.hli.duty_god<<"\n";
			 os<<"data.hli.duty_tag="<<atd.hli.duty_tag<<"\n";
			 os<<"data.hli.clash="<<atd.hli.clash<<"\n";
			 os<<"data.hli.chong_sha="<<atd.hli.chong_sha<<"\n";
			 os<<"data.hli.zodiac_day="<<atd.hli.zodiac_day<<"\n";
			 os<<"data.hli.six_he="<<atd.hli.six_he<<"\n";
			 os<<"data.hli.three_he="<<atd.hli.three_he<<"\n";
			 os<<"data.hli.pengzu="<<atd.hli.pengzu<<"\n";
			 os<<"data.hli.nayin="<<atd.hli.nayin<<"\n";
			 os<<"data.hli.wuxing_day="<<atd.hli.wx_day<<"\n";
			 os<<"data.hli.fetal_god="<<atd.hli.fetal_god<<"\n";
			 os<<"data.hli.meridian="<<atd.hli.meridian<<"\n";
			 os<<"data.hli.lucky_dir="<<atd.hli.lucky_dir<<"\n";
			 os<<"data.hli.wealth_dir="<<atd.hli.wealth_dir<<"\n";
			 os<<"data.hli.mascot_dir="<<atd.hli.mascot_dir<<"\n";
			 os<<"data.hli.sun_noble_dir="<<atd.hli.sun_noble_dir<<"\n";
			 os<<"data.hli.moon_noble_dir="<<atd.hli.moon_noble_dir<<"\n";
			 os<<"data.hli.xiu28="<<atd.hli.xiu28<<"\n";
			 os<<"data.hli.xiu_star="<<atd.hli.xiu_id<<"\n";
			 os<<"data.hli.yi_ji_level="<<atd.hli.yi_ji_level<<"\n";
			 os<<"data.hli.yi_ji_rule="<<atd.hli.yi_ji_rule<<"\n";
			 os<<"data.hli.good_gods="<<join_pipe(atd.hli.good_gods)<<"\n";
			 os<<"data.hli.bad_gods="<<join_pipe(atd.hli.bad_gods)<<"\n";
			 os<<"data.hli.yi="<<join_pipe(atd.hli.yi)<<"\n";
			 os<<"data.hli.ji="<<join_pipe(atd.hli.ji)<<"\n";
			 os<<"data.smp_liso="<<atd.local_iso<<"\n";
			 os<<"[events]\n";
			 os<<"kind\tcode\tname\tjd_utc\ttm_liso\n";
			 for(const auto&ev : day_events){
				 os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"
				   <<format_num(ev.jd_utc)<<"\t"<<ev.loc_iso<<"\n";
			 }
			 os<<"[astro_events]\n";
			 os<<"kind\tcode\tname\tjd_utc\ttm_liso\n";
			 for(const auto&ev : astro_events){
				 os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"
				   <<format_num(ev.jd_utc)<<"\t"<<ev.loc_iso<<"\n";
			 }
			 os<<"[hour_jx]\n";
			 os<<"slot\tgz\tluck\n";
			 for(const auto&x : atd.hli.hour_jx){
				 os<<x.slot<<"\t"<<x.gz<<"\t"<<x.luck<<"\n";
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"day");
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
	const OptMap handlers={
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
		{"--astro",[&](const std::vector<std::string>&src,std::size_t&idx,
					   const std::string&opt){
			 inc_astro=parse_bool01(req_val(src,idx,opt),"--astro");
		 }},
		{"--astro-mode",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 astro_mode_text=req_val(src,idx,opt);
		 }},
		{"--astro-pick",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 astro_pick_csv=req_val(src,idx,opt);
		 }},
		{"--astro-lat",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 astro_lat_deg=parse_double(req_val(src,idx,opt),opt);
			 has_astro_lat=true;
		 }},
		{"--astro-lon",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 astro_lon_deg=parse_double(req_val(src,idx,opt),opt);
			 has_astro_lon=true;
		 }},
		{"--astro-height",[&](const std::vector<std::string>&src,
							  std::size_t&idx,const std::string&opt){
			 astro_h_m=parse_double(req_val(src,idx,opt),opt);
			 has_astro_h=true;
		 }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"monthview");
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
							 "+08:00",false,false,0.0,120.0,&cache);
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
			 write_meta(w,ephem,tz,{"type=monthview","农历判日固定UTC+8"});
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

