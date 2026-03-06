void wr_enode(JsonWriter&w,double jd_tdb,int tz_off,
			  const EclipsePointMeta*meta=nullptr){
	if(!std::isfinite(jd_tdb)){
		w.null_val();
		return;
	}
	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double jd_ut1=jd_utc;
	w.obj_begin();
	w.key("jd");
	w.value(jd_utc);
	w.key("jd_tdb");
	w.value(jd_tdb);
	w.key("jd_td");
	w.value(jd_td);
	w.key("jd_ut1");
	w.value(jd_ut1);
	w.key("jd_utc");
	w.value(jd_utc);
	w.key("utc_iso");
	w.value(fmt_iso(jd_utc,0,true));
	w.key("ut1_iso");
	w.value(fmt_iso(jd_ut1,0,true));
	w.key("td_iso");
	w.value(fmt_iso(jd_td,0,true));
	w.key("loc_iso");
	w.value(fmt_iso(jd_utc,tz_off,true));
	w.key("zen_lat_deg");
	if(meta&&std::isfinite(meta->zen_lat_deg)){
		w.value(meta->zen_lat_deg);
	}else{
		w.null_val();
	}
	w.key("zen_lon_deg");
	if(meta&&std::isfinite(meta->zen_lon_deg)){
		w.value(meta->zen_lon_deg);
	}else{
		w.null_val();
	}
	w.key("pa_deg");
	if(meta&&std::isfinite(meta->pa_deg)){
		w.value(meta->pa_deg);
	}else{
		w.null_val();
	}
	w.key("axis_deg");
	if(meta&&std::isfinite(meta->axis_deg)){
		w.value(meta->axis_deg);
	}else{
		w.null_val();
	}
	w.obj_end();
}

void wr_num_or_null(JsonWriter&w,double v){
	if(std::isfinite(v)){
		w.value(v);
	}else{
		w.null_val();
	}
}

void wr_geo_json(JsonWriter&w,const EclipseGeoCoord&g){
	w.obj_begin();
	w.key("ra_deg");
	wr_num_or_null(w,g.ra_deg);
	w.key("dec_deg");
	wr_num_or_null(w,g.dec_deg);
	w.key("sd_deg");
	wr_num_or_null(w,g.sd_deg);
	w.key("ehp_deg");
	wr_num_or_null(w,g.ehp_deg);
	w.obj_end();
}

void wr_lib_json(JsonWriter&w,const EclipseLibration&lib){
	w.obj_begin();
	w.key("l_deg");
	wr_num_or_null(w,lib.l_deg);
	w.key("b_deg");
	wr_num_or_null(w,lib.b_deg);
	w.key("c_deg");
	wr_num_or_null(w,lib.c_deg);
	w.obj_end();
}

void wr_num_txt(std::ostream&os,double v){
	if(std::isfinite(v)){
		os<<v;
	}else{
		os<<"null";
	}
}

void wr_node_txt(std::ostream&os,double jd_tdb,const EclipsePointMeta&meta){
	if(!std::isfinite(jd_tdb)){
		os<<"null\tnull\tnull\tnull\tnull\tnull\tnull";
		return;
	}
	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double jd_ut1=jd_utc;
	wr_num_txt(os,jd_ut1);
	os<<"\t";
	wr_num_txt(os,jd_td);
	os<<"\t";
	wr_num_txt(os,jd_utc);
	os<<"\t";
	wr_num_txt(os,meta.zen_lat_deg);
	os<<"\t";
	wr_num_txt(os,meta.zen_lon_deg);
	os<<"\t";
	wr_num_txt(os,meta.pa_deg);
	os<<"\t";
	wr_num_txt(os,meta.axis_deg);
}

void wr_node_kv(std::ostream&os,const std::string&tag,double jd_tdb,
				const EclipsePointMeta&meta){
	double jd_td=std::numeric_limits<double>::quiet_NaN();
	double jd_utc=std::numeric_limits<double>::quiet_NaN();
	double jd_ut1=std::numeric_limits<double>::quiet_NaN();
	if(std::isfinite(jd_tdb)){
		jd_td=TimeScale::tdb_to_tt(jd_tdb);
		jd_utc=TimeScale::tdb_to_utc(jd_tdb);
		jd_ut1=jd_utc;
	}
	os<<tag<<"_jd_ut1=";
	wr_num_txt(os,jd_ut1);
	os<<"\n";
	os<<tag<<"_jd_td=";
	wr_num_txt(os,jd_td);
	os<<"\n";
	os<<tag<<"_jd=";
	wr_num_txt(os,jd_utc);
	os<<"\n";
	os<<tag<<"_zen_lat_deg=";
	wr_num_txt(os,meta.zen_lat_deg);
	os<<"\n";
	os<<tag<<"_zen_lon_deg=";
	wr_num_txt(os,meta.zen_lon_deg);
	os<<"\n";
	os<<tag<<"_pa_deg=";
	wr_num_txt(os,meta.pa_deg);
	os<<"\n";
	os<<tag<<"_axis_deg=";
	wr_num_txt(os,meta.axis_deg);
	os<<"\n";
}

std::string node_liso(double jd_tdb,int tz_off){
	if(!std::isfinite(jd_tdb)){
		return "null";
	}
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	return fmt_iso(jd_utc,tz_off,true);
}

void wr_ecljson(JsonWriter&w,const LunarEclipse&ecl,int year,int tz_off){
	w.obj_begin();
	w.key("kind");
	w.value("lunar_eclipse");
	w.key("year");
	w.value(year);
	w.key("has");
	w.value(ecl.has);
	w.key("type");
	w.value(ecl.type);
	w.key("pen_mag");
	wr_num_or_null(w,ecl.pen_mag);
	w.key("umb_mag");
	wr_num_or_null(w,ecl.umb_mag);
	w.key("rp_re");
	wr_num_or_null(w,ecl.rp_re);
	w.key("ru_re");
	wr_num_or_null(w,ecl.ru_re);
	w.key("opp_rp_re");
	wr_num_or_null(w,ecl.opp_rp_re);
	w.key("opp_ru_re");
	wr_num_or_null(w,ecl.opp_ru_re);
	w.key("dur_pen_sec");
	wr_num_or_null(w,ecl.dur_pen_sec);
	w.key("dur_umb_sec");
	wr_num_or_null(w,ecl.dur_umb_sec);
	w.key("dur_tot_sec");
	wr_num_or_null(w,ecl.dur_tot_sec);
	w.key("dt_max_sec");
	wr_num_or_null(w,ecl.dt_max_sec);
	w.key("moon_dist_km");
	wr_num_or_null(w,ecl.moon_dist_km);
	w.key("gamma");
	wr_num_or_null(w,ecl.gamma);
	w.key("eps_deg");
	wr_num_or_null(w,ecl.eps_deg);
	w.key("sun_geo");
	wr_geo_json(w,ecl.sun_geo);
	w.key("moon_geo");
	wr_geo_json(w,ecl.moon_geo);
	w.key("lib");
	wr_lib_json(w,ecl.lib);
	w.key("p1");
	wr_enode(w,ecl.jd_tdb_p1,tz_off,&ecl.p1_meta);
	w.key("u1");
	wr_enode(w,ecl.jd_tdb_u1,tz_off,&ecl.u1_meta);
	w.key("opp");
	wr_enode(w,ecl.jd_tdb_opp,tz_off,&ecl.opp_meta);
	w.key("max");
	wr_enode(w,ecl.jd_tdb_max,tz_off,&ecl.max_meta);
	w.key("u4");
	wr_enode(w,ecl.jd_tdb_u4,tz_off,&ecl.u4_meta);
	w.key("p4");
	wr_enode(w,ecl.jd_tdb_p4,tz_off,&ecl.p4_meta);
	w.key("u2");
	wr_enode(w,ecl.jd_tdb_u2,tz_off,&ecl.u2_meta);
	w.key("u3");
	wr_enode(w,ecl.jd_tdb_u3,tz_off,&ecl.u3_meta);
	w.obj_end();
}

void wr_ecltxt(std::ostream&os,const std::vector<LunarEclipse>&items,int tz_off){
	os<<"type\tpen_mag\tumb_mag\tgamma\teps_deg\tdt_max_sec\tdur_pen_sec\t"
		 "dur_umb_sec\tdur_tot_sec\trp_re\tru_re\topp_rp_re\topp_ru_re\t"
		 "moon_dist_km\tsun_ra_deg\tsun_dec_deg\tsun_sd_deg\tsun_ehp_deg\t"
		 "moon_ra_deg\tmoon_dec_deg\tmoon_sd_deg\tmoon_ehp_deg\tlib_l_deg\t"
		 "lib_b_deg\tlib_c_deg\topp_liso\tmax_liso\tp1_liso\tu1_liso\tu2_liso\t"
		 "u3_liso\tu4_liso\tp4_liso\t"
		 "p1_jd_ut1\tp1_jd_td\tp1_jd\tp1_zen_lat_deg\tp1_zen_lon_deg\t"
		 "p1_pa_deg\tp1_axis_deg\tu1_jd_ut1\tu1_jd_td\tu1_jd\tu1_zen_lat_deg\t"
		 "u1_zen_lon_deg\tu1_pa_deg\tu1_axis_deg\tu2_jd_ut1\tu2_jd_td\tu2_jd\t"
		 "u2_zen_lat_deg\tu2_zen_lon_deg\tu2_pa_deg\tu2_axis_deg\t"
		 "opp_jd_ut1\topp_jd_td\topp_jd\topp_zen_lat_deg\topp_zen_lon_deg\t"
		 "opp_pa_deg\topp_axis_deg\t"
		 "max_jd_ut1\tmax_jd_td\tmax_jd\tmax_zen_lat_deg\tmax_zen_lon_deg\t"
		 "max_pa_deg\tmax_axis_deg\tu3_jd_ut1\tu3_jd_td\tu3_jd\tu3_zen_lat_deg\t"
		 "u3_zen_lon_deg\tu3_pa_deg\tu3_axis_deg\tu4_jd_ut1\tu4_jd_td\tu4_jd\t"
		 "u4_zen_lat_deg\tu4_zen_lon_deg\tu4_pa_deg\tu4_axis_deg\t"
		 "p4_jd_ut1\tp4_jd_td\tp4_jd\tp4_zen_lat_deg\tp4_zen_lon_deg\t"
		 "p4_pa_deg\tp4_axis_deg\n";
	os<<std::setprecision(17);
	for(const auto&ecl : items){
		os<<ecl.type<<"\t";
		wr_num_txt(os,ecl.pen_mag);
		os<<"\t";
		wr_num_txt(os,ecl.umb_mag);
		os<<"\t";
		wr_num_txt(os,ecl.gamma);
		os<<"\t";
		wr_num_txt(os,ecl.eps_deg);
		os<<"\t";
		wr_num_txt(os,ecl.dt_max_sec);
		os<<"\t";
		wr_num_txt(os,ecl.dur_pen_sec);
		os<<"\t";
		wr_num_txt(os,ecl.dur_umb_sec);
		os<<"\t";
		wr_num_txt(os,ecl.dur_tot_sec);
		os<<"\t";
		wr_num_txt(os,ecl.rp_re);
		os<<"\t";
		wr_num_txt(os,ecl.ru_re);
		os<<"\t";
		wr_num_txt(os,ecl.opp_rp_re);
		os<<"\t";
		wr_num_txt(os,ecl.opp_ru_re);
		os<<"\t";
		wr_num_txt(os,ecl.moon_dist_km);
		os<<"\t";
		wr_num_txt(os,ecl.sun_geo.ra_deg);
		os<<"\t";
		wr_num_txt(os,ecl.sun_geo.dec_deg);
		os<<"\t";
		wr_num_txt(os,ecl.sun_geo.sd_deg);
		os<<"\t";
		wr_num_txt(os,ecl.sun_geo.ehp_deg);
		os<<"\t";
		wr_num_txt(os,ecl.moon_geo.ra_deg);
		os<<"\t";
		wr_num_txt(os,ecl.moon_geo.dec_deg);
		os<<"\t";
		wr_num_txt(os,ecl.moon_geo.sd_deg);
		os<<"\t";
		wr_num_txt(os,ecl.moon_geo.ehp_deg);
		os<<"\t";
		wr_num_txt(os,ecl.lib.l_deg);
		os<<"\t";
		wr_num_txt(os,ecl.lib.b_deg);
		os<<"\t";
		wr_num_txt(os,ecl.lib.c_deg);
		os<<"\t"<<node_liso(ecl.jd_tdb_opp,tz_off)
		  <<"\t"<<node_liso(ecl.jd_tdb_max,tz_off)
		  <<"\t"<<node_liso(ecl.jd_tdb_p1,tz_off)
		  <<"\t"<<node_liso(ecl.jd_tdb_u1,tz_off)
		  <<"\t"<<node_liso(ecl.jd_tdb_u2,tz_off)
		  <<"\t"<<node_liso(ecl.jd_tdb_u3,tz_off)
		  <<"\t"<<node_liso(ecl.jd_tdb_u4,tz_off)
		  <<"\t"<<node_liso(ecl.jd_tdb_p4,tz_off)
		  <<"\t";
		wr_node_txt(os,ecl.jd_tdb_p1,ecl.p1_meta);
		os<<"\t";
		wr_node_txt(os,ecl.jd_tdb_u1,ecl.u1_meta);
		os<<"\t";
		wr_node_txt(os,ecl.jd_tdb_u2,ecl.u2_meta);
		os<<"\t";
		wr_node_txt(os,ecl.jd_tdb_opp,ecl.opp_meta);
		os<<"\t";
		wr_node_txt(os,ecl.jd_tdb_max,ecl.max_meta);
		os<<"\t";
		wr_node_txt(os,ecl.jd_tdb_u3,ecl.u3_meta);
		os<<"\t";
		wr_node_txt(os,ecl.jd_tdb_u4,ecl.u4_meta);
		os<<"\t";
		wr_node_txt(os,ecl.jd_tdb_p4,ecl.p4_meta);
		os<<"\n";
	}
}

