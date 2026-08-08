namespace{

struct EclOpt{
	std::string ephem;
	std::string kind="lunar";
	bool kind_set=false;
	std::string near_date;
	std::string visible_near;
	int visible_years=20;
	std::string stage="any";
	std::string tz;
	std::string format;
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
	double sample_min=2.0;
	double point_lat=0.0;
	double point_lon=0.0;
	double point_height=0.0;
	bool has_point_lat=false;
	bool has_point_lon=false;
	bool has_point_height=false;
	bool point_refine=true;
	bool point_refine_set=false;
	bool global_vis=false;
	std::string global_fmt="json";
	double grid_lat=10.0;
	double grid_lon=10.0;
	bool global_fmt_set=false;
	bool grid_lat_set=false;
	bool grid_lon_set=false;
};

struct EclRes{
	int tz_off=0;
	EventRec ev;
	bool lunar=false;
	LunarEclipse le;
	SolarEclipse se;
	bool has_lpt=false;
	LunarEclipsePointVis lpt;
	bool has_lglb=false;
	LunarEclipseGlobalVis lglb;
	bool has_spt=false;
	SolarEclipsePointVis spt;
	bool has_sglb=false;
	SolarEclipseGlobalVis sglb;
	bool visible_search=false;
	double visible_target_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double visible_delta_days=std::numeric_limits<double>::quiet_NaN();
};

void out_num(std::ostream&os,double v){
	if(std::isfinite(v)){
		os<<format_num(v);
	}else{
		os<<"null";
	}
}

void out_loc(std::ostream&os,const std::string&key,double jd,int tz_off){
	os<<key<<"=";
	if(std::isfinite(jd)){
		os<<fmt_iso(jd,tz_off,true)<<"\n";
	}else{
		os<<"null\n";
	}
}

void out_coeff(std::ostream&os,const std::string&key,
			   const std::array<double,4>&coeff){
	os<<key<<"=";
	for(std::size_t i=0;i<coeff.size();++i){
		if(i>0){
			os<<",";
		}
		out_num(os,coeff[i]);
	}
	os<<"\n";
}

void write_sol_besselian_txt(std::ostream&os,const SolarBesselianElements&b,
							 int tz_off){
	os<<"[solar_besselian]\n";
	if(!b.has){
		os<<"null\n";
		return;
	}
	os<<"epoch_jd_tdb="; out_num(os,b.jd_tdb_epoch); os<<"\n";
	os<<"epoch_jd_utc="; out_num(os,TimeScale::tdb_to_utc(b.jd_tdb_epoch)); os<<"\n";
	os<<"epoch_utc="<<fmt_iso(TimeScale::tdb_to_utc(b.jd_tdb_epoch),0,true)<<"\n";
	os<<"epoch_td="<<fmt_iso(TimeScale::tdb_to_tt(b.jd_tdb_epoch),0,true)<<"\n";
	os<<"epoch_loc_iso="<<node_liso(b.jd_tdb_epoch,tz_off)<<"\n";
	os<<"time_argument=hours_from_epoch_tdb\n";
	os<<"polynomial_order=3\n";
	os<<"x="; out_num(os,b.x); os<<"\n";
	os<<"y="; out_num(os,b.y); os<<"\n";
	os<<"d_deg="; out_num(os,b.d_deg); os<<"\n";
	os<<"mu_deg="; out_num(os,b.mu_deg); os<<"\n";
	os<<"l1="; out_num(os,b.l1); os<<"\n";
	os<<"l2="; out_num(os,b.l2); os<<"\n";
	os<<"tan_f1="; out_num(os,b.tan_f1); os<<"\n";
	os<<"tan_f2="; out_num(os,b.tan_f2); os<<"\n";
	os<<"x_dot="; out_num(os,b.x_dot); os<<"\n";
	os<<"y_dot="; out_num(os,b.y_dot); os<<"\n";
	os<<"d_dot_deg="; out_num(os,b.d_dot_deg); os<<"\n";
	os<<"mu_dot_deg="; out_num(os,b.mu_dot_deg); os<<"\n";
	os<<"l1_dot="; out_num(os,b.l1_dot); os<<"\n";
	os<<"l2_dot="; out_num(os,b.l2_dot); os<<"\n";
	out_coeff(os,"x_coeff",b.x_coeff);
	out_coeff(os,"y_coeff",b.y_coeff);
	out_coeff(os,"d_coeff_deg",b.d_coeff_deg);
	out_coeff(os,"mu_coeff_deg",b.mu_coeff_deg);
	out_coeff(os,"l1_coeff",b.l1_coeff);
	out_coeff(os,"l2_coeff",b.l2_coeff);
}

void write_in(JsonWriter&w,const EclOpt&opt){
	w.key("input");
	w.obj_begin();
	w.key("kind");
	w.value(opt.kind);
	w.key("near");
	if(opt.near_date.empty()){
		w.null_val();
	}else{
		w.value(opt.near_date);
	}
	w.key("visible_near");
	if(opt.visible_near.empty()){
		w.null_val();
	}else{
		w.value(opt.visible_near);
	}
	w.key("visible_years");
	w.value(opt.visible_years);
	w.key("stage_window");
	w.value(opt.stage);
	w.key("sample_minutes");
	w.value(opt.sample_min);
	w.key("global_vis");
	w.value(opt.global_vis);
	w.key("global_format");
	w.value(opt.global_fmt);
	w.key("point");
	if(opt.has_point_lat){
		w.obj_begin();
		w.key("lat_deg");
		w.value(opt.point_lat);
		w.key("lon_deg");
		w.value(opt.point_lon);
		w.key("height_m");
		w.value(opt.point_height);
		w.key("refine");
		w.value(opt.point_refine);
		w.obj_end();
	}else{
		w.null_val();
	}
	w.obj_end();
}

EclOpt parse_ecl(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument("eclipse requires: <bsp> --near <YYYY-MM-DD>");
	}
	InterCfg cfg=load_def();
	EclOpt opt;
	opt.ephem=args[0];
	opt.tz=cfg.default_tz;
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="json"&&opt.format!="txt"&&opt.format!="geojson"){
		opt.format="json";
	}
	opt.pretty=cfg.def_prety;
	lunar::ArgParser parser;
	parser.add_value("--kind",[&](const std::string&v){
			opt.kind=to_low(v);
			opt.kind_set=true;
		})
		.add_value("--near",[&](const std::string&v){ opt.near_date=v; })
		.add_value("--visible-near",
				   [&](const std::string&v){ opt.visible_near=v; })
		.add_value("--visible-years",[&](const std::string&v){
			opt.visible_years=parse_int(v,"--visible-years");
		})
		.add_value("--stage",[&](const std::string&v){ opt.stage=to_low(v); })
		.add_value("--sample-min",[&](const std::string&v){
			opt.sample_min=parse_double(v,"--sample-min");
		})
		.add_value("--point-refine",[&](const std::string&v){
			opt.point_refine=parse_bool01(v,"--point-refine");
			opt.point_refine_set=true;
		})
		.add_value("--point-lat",[&](const std::string&v){
			opt.point_lat=parse_double(v,"--point-lat");
			opt.has_point_lat=true;
		})
		.add_value("--point-lon",[&](const std::string&v){
			opt.point_lon=parse_double(v,"--point-lon");
			opt.has_point_lon=true;
		})
		.add_value("--point-height",[&](const std::string&v){
			opt.point_height=parse_double(v,"--point-height");
			opt.has_point_height=true;
		})
		.add_value("--global-vis",[&](const std::string&v){
			opt.global_vis=parse_bool01(v,"--global-vis");
		})
		.add_value("--global",[&](const std::string&v){
			opt.global_vis=parse_bool01(v,"--global");
		})
		.add_value("--global-format",[&](const std::string&v){
			opt.global_fmt=to_low(v);
			opt.global_fmt_set=true;
		})
		.add_value("--grid-lat-step",[&](const std::string&v){
			opt.grid_lat=parse_double(v,"--grid-lat-step");
			opt.grid_lat_set=true;
		})
		.add_value("--grid-lon-step",[&](const std::string&v){
			opt.grid_lon=parse_double(v,"--grid-lon-step");
			opt.grid_lon_set=true;
		})
		.add_value("--tz",[&](const std::string&v){ opt.tz=v; })
		.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; });
	parser.parse_all(args,1,"eclipse");
	const bool visible_mode=!opt.visible_near.empty();
	if(visible_mode&&!opt.kind_set){
		opt.kind="both";
	}
	if(!visible_mode&&opt.near_date.empty()){
		throw std::invalid_argument("eclipse requires --near <YYYY-MM-DD>");
	}
	if(visible_mode&&!opt.near_date.empty()){
		throw std::invalid_argument("--near and --visible-near are mutually exclusive");
	}
	if(opt.kind!="lunar"&&opt.kind!="solar"&&
	   !(visible_mode&&opt.kind=="both")){
		throw std::invalid_argument(
			visible_mode?"--kind must be lunar, solar, or both"
						:"--kind must be lunar or solar");
	}
	if(!(opt.sample_min>0.0)){
		throw std::invalid_argument("--sample-min must be > 0");
	}
	if(visible_mode&&opt.visible_years<1){
		throw std::invalid_argument("--visible-years must be >=1");
	}
	if(opt.has_point_lat!=opt.has_point_lon){
		throw std::invalid_argument(
			"point visibility requires both --point-lat and --point-lon");
	}
	if(visible_mode&&!opt.has_point_lat){
		throw std::invalid_argument(
			"--visible-near requires --point-lat and --point-lon");
	}
	if((opt.has_point_height||opt.point_refine_set)&&!opt.has_point_lat){
		throw std::invalid_argument(
			"--point-height/--point-refine require --point-lat and --point-lon");
	}
	if(opt.global_fmt!="json"&&opt.global_fmt!="geojson"){
		throw std::invalid_argument("--global-format must be json or geojson");
	}
	chk_fmt(opt.format,{"json","txt","geojson"},"eclipse");
	if(visible_mode&&opt.format=="geojson"){
		throw std::invalid_argument("--visible-near does not support --format geojson");
	}
	if(opt.format=="geojson"){
		opt.global_vis=true;
	}
	if(visible_mode&&opt.global_vis){
		throw std::invalid_argument("--global-vis is not used with --visible-near");
	}
	if((opt.global_fmt_set||opt.grid_lat_set||opt.grid_lon_set)&&!opt.global_vis){
		throw std::invalid_argument(
			"--global-format/--grid-lat-step/--grid-lon-step require --global-vis 1 or --format geojson");
	}
	if(opt.grid_lat_set&&!(opt.grid_lat>0.0)){
		throw std::invalid_argument("--grid-lat-step must be > 0");
	}
	if(opt.grid_lon_set&&!(opt.grid_lon>0.0)){
		throw std::invalid_argument("--grid-lon-step must be > 0");
	}
	return opt;
}

bool try_visible_lunar(EphRead&eph,const EclOpt&opt,const EventRec&ev,
					   EclRes&out){
	double jd_tdb=std::isfinite(ev.jd_tdb)?ev.jd_tdb
										  :TimeScale::utc_to_tdb(ev.jd_utc);
	EclRes cand;
	cand.tz_off=parse_tz(opt.tz);
	cand.ev=ev;
	cand.lunar=true;
	if(!calc_lunar_eclipse(eph,jd_tdb,&cand.le)||!cand.le.has){
		return false;
	}
	if(!lunar_eclipse_point_visibility(eph,cand.le,opt.stage,opt.point_lat,
									   opt.point_lon,opt.point_height,
									   opt.sample_min,opt.point_refine,
									   &cand.lpt)){
		return false;
	}
	cand.has_lpt=true;
	if(!cand.lpt.visible){
		return false;
	}
	out=std::move(cand);
	return true;
}

bool try_visible_solar(EphRead&eph,const EclOpt&opt,const EventRec&ev,
					   EclRes&out){
	double jd_tdb=std::isfinite(ev.jd_tdb)?ev.jd_tdb
										  :TimeScale::utc_to_tdb(ev.jd_utc);
	EclRes cand;
	cand.tz_off=parse_tz(opt.tz);
	cand.ev=ev;
	cand.lunar=false;
	if(!calc_solar_eclipse_from_max(eph,jd_tdb,&cand.se)||!cand.se.has){
		return false;
	}
	if(!solar_eclipse_point_visibility(eph,cand.se,opt.stage,opt.point_lat,
									   opt.point_lon,opt.point_height,
									   opt.sample_min,opt.point_refine,
									   &cand.spt)){
		return false;
	}
	cand.has_spt=true;
	if(!cand.spt.visible){
		return false;
	}
	out=std::move(cand);
	return true;
}

EclRes run_visible_ecl(const EclOpt&opt){
	EclRes res;
	res.tz_off=parse_tz(opt.tz);
	IsoTime parsed=parse_iso(opt.visible_near,opt.tz);
	res.visible_search=true;
	res.visible_target_jd_utc=parsed.jd_utc;

	int y=0;
	int m=0;
	int d=0;
	utc2civil(parsed.jd_utc,res.tz_off,y,m,d);
	std::set<int> years;
	for(int year=y-opt.visible_years;year<=y+opt.visible_years;++year){
		years.insert(year);
	}

	EvtFilt filter;
	filter.inc_st=false;
	filter.inc_lph=false;
	filter.inc_lecl=(opt.kind=="lunar"||opt.kind=="both");
	filter.inc_secl=(opt.kind=="solar"||opt.kind=="both");
	filter.inc_ecl=filter.inc_lecl||filter.inc_secl;

	EphRead eph(opt.ephem);
	std::vector<EventRec> events=
		col_eyrs(eph,years,res.tz_off,opt.quiet?nullptr:&std::cerr,filter);
	std::sort(events.begin(),events.end(),
			  [&](const EventRec&a,const EventRec&b){
				  return std::fabs(a.jd_utc-parsed.jd_utc)<
						 std::fabs(b.jd_utc-parsed.jd_utc);
			  });

	for(const auto&ev : events){
		EclRes cand;
		bool visible=false;
		if(ev.kind=="lunar_eclipse"){
			visible=try_visible_lunar(eph,opt,ev,cand);
		}else if(ev.kind=="solar_eclipse"){
			visible=try_visible_solar(eph,opt,ev,cand);
		}
		if(!visible){
			continue;
		}
		cand.visible_search=true;
		cand.visible_target_jd_utc=parsed.jd_utc;
		cand.visible_delta_days=std::fabs(ev.jd_utc-parsed.jd_utc);
		return cand;
	}

	throw std::runtime_error(
		"no visible eclipse found within +/- "+std::to_string(opt.visible_years)+
		" years");
}

EclRes run_ecl(const EclOpt&opt){
	if(!opt.visible_near.empty()){
		return run_visible_ecl(opt);
	}
	int y=0,m=0,d=0;
	std::tie(y,m,d)=parse_ymd(opt.near_date);
	EclRes res;
	res.tz_off=parse_tz(opt.tz);
	double near_jd_utc=greg2jd(y,m,d,0,0,0.0)-UTC8DAY;
	EphRead eph(opt.ephem);
	std::string kind=(opt.kind=="solar")?"solar_eclipse":"lunar_eclipse";
	res.ev=nearest_ecl(eph,near_jd_utc,res.tz_off,opt.quiet,kind);
	double jd_tdb=std::isfinite(res.ev.jd_tdb)?res.ev.jd_tdb
											  :TimeScale::utc_to_tdb(res.ev.jd_utc);
	res.lunar=(opt.kind=="lunar");
	if(res.lunar){
		if(!calc_lunar_eclipse(eph,jd_tdb,&res.le)||!res.le.has){
			throw std::runtime_error("failed to compute lunar eclipse details");
		}
		if(opt.has_point_lat){
			if(!lunar_eclipse_point_visibility(eph,res.le,opt.stage,opt.point_lat,
											   opt.point_lon,opt.point_height,
											   opt.sample_min,opt.point_refine,&res.lpt)){
				throw std::invalid_argument(
					"requested stage window is unavailable for this eclipse");
			}
			res.has_lpt=true;
		}
		if(opt.global_vis){
			if(!lunar_eclipse_global_visibility(eph,res.le,opt.stage,opt.grid_lat,
												opt.grid_lon,opt.sample_min,&res.lglb)){
				throw std::invalid_argument(
					"requested stage window is unavailable for this eclipse");
			}
			res.has_lglb=true;
		}
		return res;
	}
	if(!calc_solar_eclipse_from_max(eph,jd_tdb,&res.se)||!res.se.has){
		throw std::runtime_error("failed to compute solar eclipse details");
	}
	if(opt.has_point_lat){
		if(!solar_eclipse_point_visibility(eph,res.se,opt.stage,opt.point_lat,
										   opt.point_lon,opt.point_height,opt.sample_min,
										   opt.point_refine,&res.spt)){
			throw std::invalid_argument(
				"requested stage window is unavailable for this eclipse");
		}
		res.has_spt=true;
	}
	if(opt.global_vis){
		if(!solar_eclipse_global_visibility(eph,res.se,opt.stage,opt.grid_lat,
											opt.grid_lon,opt.sample_min,&res.sglb)){
			throw std::invalid_argument(
				"requested stage window is unavailable for this eclipse");
		}
		res.has_sglb=true;
	}
	return res;
}

void write_lun_txt(std::ostream&os,const EclOpt&opt,const EclRes&res){
	const LunarEclipse&e=res.le;
	os<<"tool=lunar format=txt type=eclipse tz_display="<<opt.tz<<"\n";
	os<<"input.kind="<<opt.kind<<"\n";
	os<<"input.near="<<opt.near_date<<"\n";
	if(res.visible_search){
		os<<"input.visible_near="<<opt.visible_near<<"\n";
		os<<"input.visible_years="<<opt.visible_years<<"\n";
		os<<"data.visible_target_loc_iso="
		  <<fmt_iso(res.visible_target_jd_utc,res.tz_off,true)<<"\n";
		os<<"data.visible_delta_days="<<format_num(res.visible_delta_days)<<"\n";
	}
	os<<"input.stage_window="<<opt.stage<<"\n";
	os<<"input.sample_minutes="<<format_num(opt.sample_min)<<"\n";
	os<<"data.event.kind="<<res.ev.kind<<"\n";
	os<<"data.event.code="<<res.ev.code<<"\n";
	os<<"data.event.name="<<res.ev.name<<"\n";
	os<<"data.event.loc_iso="<<res.ev.loc_iso<<"\n";
	os<<"[lunar_eclipse]\n";
	os<<"type="<<e.type<<"\n";
	os<<"pen_mag="; out_num(os,e.pen_mag); os<<"\n";
	os<<"umb_mag="; out_num(os,e.umb_mag); os<<"\n";
	os<<"rp_re="; out_num(os,e.rp_re); os<<"\n";
	os<<"ru_re="; out_num(os,e.ru_re); os<<"\n";
	os<<"opp_rp_re="; out_num(os,e.opp_rp_re); os<<"\n";
	os<<"opp_ru_re="; out_num(os,e.opp_ru_re); os<<"\n";
	os<<"dur_pen_sec="; out_num(os,e.dur_pen_sec); os<<"\n";
	os<<"dur_umb_sec="; out_num(os,e.dur_umb_sec); os<<"\n";
	os<<"dur_tot_sec="; out_num(os,e.dur_tot_sec); os<<"\n";
	os<<"dt_max_sec="; out_num(os,e.dt_max_sec); os<<"\n";
	os<<"moon_dist_km="; out_num(os,e.moon_dist_km); os<<"\n";
	os<<"gamma="; out_num(os,e.gamma); os<<"\n";
	os<<"eps_deg="; out_num(os,e.eps_deg); os<<"\n";
	os<<"sun_ra_deg="; out_num(os,e.sun_geo.ra_deg); os<<"\n";
	os<<"sun_dec_deg="; out_num(os,e.sun_geo.dec_deg); os<<"\n";
	os<<"sun_sd_deg="; out_num(os,e.sun_geo.sd_deg); os<<"\n";
	os<<"sun_ehp_deg="; out_num(os,e.sun_geo.ehp_deg); os<<"\n";
	os<<"moon_ra_deg="; out_num(os,e.moon_geo.ra_deg); os<<"\n";
	os<<"moon_dec_deg="; out_num(os,e.moon_geo.dec_deg); os<<"\n";
	os<<"moon_sd_deg="; out_num(os,e.moon_geo.sd_deg); os<<"\n";
	os<<"moon_ehp_deg="; out_num(os,e.moon_geo.ehp_deg); os<<"\n";
	os<<"lib_l_deg="; out_num(os,e.lib.l_deg); os<<"\n";
	os<<"lib_b_deg="; out_num(os,e.lib.b_deg); os<<"\n";
	os<<"lib_c_deg="; out_num(os,e.lib.c_deg); os<<"\n";
	os<<"p1_loc_iso="<<node_liso(e.jd_tdb_p1,res.tz_off)<<"\n";
	os<<"u1_loc_iso="<<node_liso(e.jd_tdb_u1,res.tz_off)<<"\n";
	os<<"opp_loc_iso="<<node_liso(e.jd_tdb_opp,res.tz_off)<<"\n";
	os<<"max_loc_iso="<<node_liso(e.jd_tdb_max,res.tz_off)<<"\n";
	os<<"u4_loc_iso="<<node_liso(e.jd_tdb_u4,res.tz_off)<<"\n";
	os<<"p4_loc_iso="<<node_liso(e.jd_tdb_p4,res.tz_off)<<"\n";
	os<<"u2_loc_iso="<<node_liso(e.jd_tdb_u2,res.tz_off)<<"\n";
	os<<"u3_loc_iso="<<node_liso(e.jd_tdb_u3,res.tz_off)<<"\n";
	wr_node_kv(os,"p1",e.jd_tdb_p1,e.p1_meta); wr_node_kv(os,"u1",e.jd_tdb_u1,e.u1_meta);
	wr_node_kv(os,"u2",e.jd_tdb_u2,e.u2_meta); wr_node_kv(os,"max",e.jd_tdb_max,e.max_meta);
	wr_node_kv(os,"u3",e.jd_tdb_u3,e.u3_meta); wr_node_kv(os,"u4",e.jd_tdb_u4,e.u4_meta);
	wr_node_kv(os,"p4",e.jd_tdb_p4,e.p4_meta);
	if(res.has_lpt){
		os<<"[point_visibility]\n";
		os<<"visible="<<(res.lpt.visible?"1":"0")<<"\n";
		os<<"max_alt_deg="<<format_num(res.lpt.max_alt_deg)<<"\n";
		out_loc(os,"first_visible",res.lpt.first_jd_utc,res.tz_off);
		out_loc(os,"last_visible",res.lpt.last_jd_utc,res.tz_off);
		os<<"sample_count="<<res.lpt.sample_count<<"\n";
	}
	if(res.has_lglb){
		os<<"[global_visibility]\n";
		os<<"points="<<res.lglb.points.size()<<"\n";
		os<<"lat\tlon\tmax_alt_deg\tfirst_visible\tlast_visible\n";
		for(const auto&pt : res.lglb.points){
			os<<format_num(pt.lat_deg)<<"\t"<<format_num(pt.lon_deg)<<"\t"
			  <<format_num(pt.max_alt_deg)<<"\t"<<fmt_iso(pt.first_jd_utc,res.tz_off,true)
			  <<"\t"<<fmt_iso(pt.last_jd_utc,res.tz_off,true)<<"\n";
		}
	}
}

void write_sol_txt(std::ostream&os,const EclOpt&opt,const EclRes&res){
	const SolarEclipse&e=res.se;
	os<<"tool=lunar format=txt type=eclipse tz_display="<<opt.tz<<"\n";
	os<<"input.kind="<<opt.kind<<"\n";
	os<<"input.near="<<opt.near_date<<"\n";
	if(res.visible_search){
		os<<"input.visible_near="<<opt.visible_near<<"\n";
		os<<"input.visible_years="<<opt.visible_years<<"\n";
		os<<"data.visible_target_loc_iso="
		  <<fmt_iso(res.visible_target_jd_utc,res.tz_off,true)<<"\n";
		os<<"data.visible_delta_days="<<format_num(res.visible_delta_days)<<"\n";
	}
	os<<"input.stage_window="<<opt.stage<<"\n";
	os<<"input.sample_minutes="<<format_num(opt.sample_min)<<"\n";
	os<<"data.event.kind="<<res.ev.kind<<"\n";
	os<<"data.event.code="<<res.ev.code<<"\n";
	os<<"data.event.name="<<res.ev.name<<"\n";
	os<<"data.event.loc_iso="<<res.ev.loc_iso<<"\n";
	os<<"[solar_eclipse]\n";
	os<<"type="<<e.type<<"\n";
	os<<"mag="; out_num(os,e.mag); os<<"\n";
	os<<"obscuration="; out_num(os,e.obscuration); os<<"\n";
	os<<"gamma="; out_num(os,e.gamma); os<<"\n";
	os<<"sep_max_deg="; out_num(os,e.sep_max_deg); os<<"\n";
	os<<"sun_sd_max_deg="; out_num(os,e.sun_sd_max_deg); os<<"\n";
	os<<"moon_sd_max_deg="; out_num(os,e.moon_sd_max_deg); os<<"\n";
	os<<"sun_dist_km="; out_num(os,e.sun_dist_km); os<<"\n";
	os<<"moon_dist_km="; out_num(os,e.moon_dist_km); os<<"\n";
	os<<"dt_max_sec="; out_num(os,e.dt_max_sec); os<<"\n";
	os<<"rp_re="; out_num(os,e.rp_re); os<<"\n";
	os<<"ru_re="; out_num(os,e.ru_re); os<<"\n";
	write_sol_besselian_txt(os,e.besselian,res.tz_off);
	os<<"c1_loc_iso="<<node_liso(e.jd_tdb_c1,res.tz_off)<<"\n";
	os<<"c2_loc_iso="<<node_liso(e.jd_tdb_c2,res.tz_off)<<"\n";
	os<<"max_loc_iso="<<node_liso(e.jd_tdb_max,res.tz_off)<<"\n";
	os<<"c3_loc_iso="<<node_liso(e.jd_tdb_c3,res.tz_off)<<"\n";
	os<<"c4_loc_iso="<<node_liso(e.jd_tdb_c4,res.tz_off)<<"\n";
	if(res.has_spt){
		os<<"[point_visibility]\n";
		os<<"has_eclipse="<<(res.spt.has_eclipse?"1":"0")<<"\n";
		os<<"visible="<<(res.spt.visible?"1":"0")<<"\n";
		os<<"central="<<(res.spt.central?"1":"0")<<"\n";
		os<<"max_mag="<<format_num(res.spt.max_mag)<<"\n";
		os<<"max_obscuration="<<format_num(res.spt.max_obscuration)<<"\n";
		os<<"max_sun_alt_deg="<<format_num(res.spt.max_sun_alt_deg)<<"\n";
		out_loc(os,"c1_loc_iso",res.spt.c1_jd_utc,res.tz_off);
		out_loc(os,"c2_loc_iso",res.spt.c2_jd_utc,res.tz_off);
		out_loc(os,"max_loc_iso",res.spt.max_jd_utc,res.tz_off);
		out_loc(os,"c3_loc_iso",res.spt.c3_jd_utc,res.tz_off);
		out_loc(os,"c4_loc_iso",res.spt.c4_jd_utc,res.tz_off);
		out_loc(os,"first_visible",res.spt.first_jd_utc,res.tz_off);
		out_loc(os,"last_visible",res.spt.last_jd_utc,res.tz_off);
		os<<"sample_count="<<res.spt.sample_count<<"\n";
	}
	if(res.has_sglb){
		os<<"[global_visibility]\n";
		os<<"points="<<res.sglb.points.size()<<"\n";
		os<<"lat\tlon\tmax_mag\tmax_sun_alt_deg\tfirst_visible\tlast_visible\n";
		for(const auto&pt : res.sglb.points){
			os<<format_num(pt.lat_deg)<<"\t"<<format_num(pt.lon_deg)<<"\t"
			  <<format_num(pt.max_mag)<<"\t"<<format_num(pt.max_sun_alt_deg)<<"\t"
			  <<fmt_iso(pt.first_jd_utc,res.tz_off,true)<<"\t"
			  <<fmt_iso(pt.last_jd_utc,res.tz_off,true)<<"\n";
		}
	}
}

void write_ecl(std::ostream&os,const EclOpt&opt,const EclRes&res){
	EphRead eph(opt.ephem);
	if(opt.format=="geojson"){
		if(res.lunar){
			if(!res.has_lglb){ throw std::runtime_error("geojson output requires global visibility"); }
			JsonWriter w(os,opt.pretty); wr_glbvis_geojson(w,res.lglb,res.tz_off); os<<"\n";
		}else{
			if(!res.has_sglb){ throw std::runtime_error("geojson output requires global visibility"); }
			JsonWriter w(os,opt.pretty); wr_sol_glbvis_geojson(w,res.sglb,res.tz_off); os<<"\n";
		}
		return;
	}
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 write_meta(w,opt.ephem,opt.tz,{"type=eclipse"});
			 write_in(w,opt);
			 w.key("data");
			 w.obj_begin();
			 if(res.visible_search){
				 w.key("visible_target");
				 w.obj_begin();
				 w.key("jd_utc");
				 w.value(res.visible_target_jd_utc);
				 w.key("utc_iso");
				 w.value(fmt_iso(res.visible_target_jd_utc,0,true));
				 w.key("loc_iso");
				 w.value(fmt_iso(res.visible_target_jd_utc,res.tz_off,true));
				 w.key("delta_days");
				 w.value(res.visible_delta_days);
				 w.obj_end();
			 }
			 w.key("event");
			 wr_ejson(w,res.ev,eph,false,res.tz_off);
			 if(res.lunar){
				 w.key("lunar_eclipse");
				 wr_ecljson(w,res.le,res.ev.year,res.tz_off);
				 w.key("point_visibility");
				 if(res.has_lpt){
					 wr_ptvis_json(w,res.lpt,res.tz_off);
				 }else{
					 w.null_val();
				 }
				 w.key("global_visibility");
				 if(res.has_lglb){
					 if(opt.global_fmt=="geojson"){
						 wr_glbvis_geojson(w,res.lglb,res.tz_off);
					 }else{
						 wr_glbvis_json(w,res.lglb,res.tz_off);
					 }
				 }else{
					 w.null_val();
				 }
			 }else{
				 w.key("solar_eclipse");
				 wr_sol_ecljson(w,res.se,res.ev.year,res.tz_off);
				 w.key("point_visibility");
				 if(res.has_spt){
					 wr_sol_ptvis_json(w,res.spt,res.tz_off);
				 }else{
					 w.null_val();
				 }
				 w.key("global_visibility");
				 if(res.has_sglb){
					 if(opt.global_fmt=="geojson"){
						 wr_sol_glbvis_geojson(w,res.sglb,res.tz_off);
					 }else{
						 wr_sol_glbvis_json(w,res.sglb,res.tz_off);
					 }
				 }else{
					 w.null_val();
				 }
			 }
			 w.obj_end();
			 w.obj_end();
			 os<<"\n";
		 }},
		{"txt",[&](){ if(res.lunar){ write_lun_txt(os,opt,res); }else{ write_sol_txt(os,opt,res); } }},
	};
	run_fmt(fmts,opt.format,"eclipse");
}

}

int cmd_eclipse(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_eclipse();
		return 0;
	}
	EclOpt opt=parse_ecl(args);
	EclRes res=run_ecl(opt);
	OutTgt out=open_out(opt.out_path);
	write_ecl(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}
