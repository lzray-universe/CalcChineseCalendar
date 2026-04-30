namespace{

void wr_csv_num(CsvWriter&w,const std::string&key,double v){
	if(std::isfinite(v)){
		w.write_raw(key,format_num(v));
	}else{
		w.write_raw(key,"");
	}
}

std::vector<std::string> el_csv_hdr(bool calc_eclipse){
	std::vector<std::string> out={
		"kind","code","name","year","jd_tdb","jd_utc","utc_iso","loc_iso"
	};
	if(!calc_eclipse){
		return out;
	}
	const std::array<const char*,89> extra={{
		"eclipse_type","eclipse_gamma","eclipse_eps_deg","eclipse_dt_max_sec",
		"eclipse_dur_pen_sec","eclipse_dur_umb_sec","eclipse_dur_tot_sec",
		"eclipse_rp_re","eclipse_ru_re","eclipse_opp_rp_re","eclipse_opp_ru_re",
		"eclipse_moon_dist_km","eclipse_sun_ra_deg","eclipse_sun_dec_deg",
		"eclipse_sun_sd_deg","eclipse_sun_ehp_deg","eclipse_moon_ra_deg",
		"eclipse_moon_dec_deg","eclipse_moon_sd_deg","eclipse_moon_ehp_deg",
		"eclipse_lib_l_deg","eclipse_lib_b_deg","eclipse_lib_c_deg",
		"eclipse_opp_loc_iso","eclipse_max_loc_iso","eclipse_pen_mag",
		"eclipse_umb_mag","eclipse_p1_loc_iso","eclipse_u1_loc_iso",
		"eclipse_u2_loc_iso","eclipse_u3_loc_iso","eclipse_u4_loc_iso",
		"eclipse_p4_loc_iso",
		"p1_jd_ut1","p1_jd_td","p1_jd","p1_zen_lat_deg","p1_zen_lon_deg",
		"p1_pa_deg","p1_axis_deg",
		"u1_jd_ut1","u1_jd_td","u1_jd","u1_zen_lat_deg","u1_zen_lon_deg",
		"u1_pa_deg","u1_axis_deg",
		"u2_jd_ut1","u2_jd_td","u2_jd","u2_zen_lat_deg","u2_zen_lon_deg",
		"u2_pa_deg","u2_axis_deg",
		"opp_jd_ut1","opp_jd_td","opp_jd","opp_zen_lat_deg","opp_zen_lon_deg",
		"opp_pa_deg","opp_axis_deg",
		"max_jd_ut1","max_jd_td","max_jd","max_zen_lat_deg","max_zen_lon_deg",
		"max_pa_deg","max_axis_deg",
		"u3_jd_ut1","u3_jd_td","u3_jd","u3_zen_lat_deg","u3_zen_lon_deg",
		"u3_pa_deg","u3_axis_deg",
		"u4_jd_ut1","u4_jd_td","u4_jd","u4_zen_lat_deg","u4_zen_lon_deg",
		"u4_pa_deg","u4_axis_deg",
		"p4_jd_ut1","p4_jd_td","p4_jd","p4_zen_lat_deg","p4_zen_lon_deg",
		"p4_pa_deg","p4_axis_deg"
	}};
	out.insert(out.end(),extra.begin(),extra.end());
	const std::array<const char*,17> solar_extra={{
		"solar_eclipse_type","solar_eclipse_mag",
		"solar_eclipse_obscuration","solar_eclipse_gamma",
		"solar_eclipse_sep_max_deg","solar_eclipse_sun_sd_max_deg",
		"solar_eclipse_moon_sd_max_deg","solar_eclipse_sun_dist_km",
		"solar_eclipse_moon_dist_km","solar_eclipse_dt_max_sec",
		"solar_eclipse_rp_re","solar_eclipse_ru_re",
		"solar_eclipse_c1_loc_iso","solar_eclipse_c2_loc_iso",
		"solar_eclipse_max_loc_iso","solar_eclipse_c3_loc_iso",
		"solar_eclipse_c4_loc_iso"
	}};
	out.insert(out.end(),solar_extra.begin(),solar_extra.end());
	return out;
}

void wr_csv_liso(CsvWriter&w,const std::string&key,double jd_tdb,int tz_off){
	if(std::isfinite(jd_tdb)){
		w.write_field(key,node_liso(jd_tdb,tz_off));
	}else{
		w.write_raw(key,"");
	}
}

void wr_csv_node_blank(CsvWriter&w,const std::string&tag){
	w.write_raw(tag+"_jd_ut1","");
	w.write_raw(tag+"_jd_td","");
	w.write_raw(tag+"_jd","");
	w.write_raw(tag+"_zen_lat_deg","");
	w.write_raw(tag+"_zen_lon_deg","");
	w.write_raw(tag+"_pa_deg","");
	w.write_raw(tag+"_axis_deg","");
}

void wr_csv_node(CsvWriter&w,const std::string&tag,double jd_tdb,
				 const EclipsePointMeta&meta){
	if(!std::isfinite(jd_tdb)){
		wr_csv_node_blank(w,tag);
		return;
	}
	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double jd_ut1=jd_utc;
	w.write_raw(tag+"_jd_ut1",format_num(jd_ut1));
	w.write_raw(tag+"_jd_td",format_num(jd_td));
	w.write_raw(tag+"_jd",format_num(jd_utc));
	w.write_raw(tag+"_zen_lat_deg",node_num(meta.zen_lat_deg));
	w.write_raw(tag+"_zen_lon_deg",node_num(meta.zen_lon_deg));
	w.write_raw(tag+"_pa_deg",node_num(meta.pa_deg));
	w.write_raw(tag+"_axis_deg",node_num(meta.axis_deg));
}

void wr_csv_ecl(CsvWriter&w,const LunarEclipse&ecl,int tz_off){
	w.write_field("eclipse_type",ecl.type);
	wr_csv_num(w,"eclipse_gamma",ecl.gamma);
	wr_csv_num(w,"eclipse_eps_deg",ecl.eps_deg);
	wr_csv_num(w,"eclipse_dt_max_sec",ecl.dt_max_sec);
	wr_csv_num(w,"eclipse_dur_pen_sec",ecl.dur_pen_sec);
	wr_csv_num(w,"eclipse_dur_umb_sec",ecl.dur_umb_sec);
	wr_csv_num(w,"eclipse_dur_tot_sec",ecl.dur_tot_sec);
	wr_csv_num(w,"eclipse_rp_re",ecl.rp_re);
	wr_csv_num(w,"eclipse_ru_re",ecl.ru_re);
	wr_csv_num(w,"eclipse_opp_rp_re",ecl.opp_rp_re);
	wr_csv_num(w,"eclipse_opp_ru_re",ecl.opp_ru_re);
	wr_csv_num(w,"eclipse_moon_dist_km",ecl.moon_dist_km);
	wr_csv_num(w,"eclipse_sun_ra_deg",ecl.sun_geo.ra_deg);
	wr_csv_num(w,"eclipse_sun_dec_deg",ecl.sun_geo.dec_deg);
	wr_csv_num(w,"eclipse_sun_sd_deg",ecl.sun_geo.sd_deg);
	wr_csv_num(w,"eclipse_sun_ehp_deg",ecl.sun_geo.ehp_deg);
	wr_csv_num(w,"eclipse_moon_ra_deg",ecl.moon_geo.ra_deg);
	wr_csv_num(w,"eclipse_moon_dec_deg",ecl.moon_geo.dec_deg);
	wr_csv_num(w,"eclipse_moon_sd_deg",ecl.moon_geo.sd_deg);
	wr_csv_num(w,"eclipse_moon_ehp_deg",ecl.moon_geo.ehp_deg);
	wr_csv_num(w,"eclipse_lib_l_deg",ecl.lib.l_deg);
	wr_csv_num(w,"eclipse_lib_b_deg",ecl.lib.b_deg);
	wr_csv_num(w,"eclipse_lib_c_deg",ecl.lib.c_deg);
	wr_csv_liso(w,"eclipse_opp_loc_iso",ecl.jd_tdb_opp,tz_off);
	wr_csv_liso(w,"eclipse_max_loc_iso",ecl.jd_tdb_max,tz_off);
	wr_csv_num(w,"eclipse_pen_mag",ecl.pen_mag);
	wr_csv_num(w,"eclipse_umb_mag",ecl.umb_mag);
	wr_csv_liso(w,"eclipse_p1_loc_iso",ecl.jd_tdb_p1,tz_off);
	wr_csv_liso(w,"eclipse_u1_loc_iso",ecl.jd_tdb_u1,tz_off);
	wr_csv_liso(w,"eclipse_u2_loc_iso",ecl.jd_tdb_u2,tz_off);
	wr_csv_liso(w,"eclipse_u3_loc_iso",ecl.jd_tdb_u3,tz_off);
	wr_csv_liso(w,"eclipse_u4_loc_iso",ecl.jd_tdb_u4,tz_off);
	wr_csv_liso(w,"eclipse_p4_loc_iso",ecl.jd_tdb_p4,tz_off);
	wr_csv_node(w,"p1",ecl.jd_tdb_p1,ecl.p1_meta);
	wr_csv_node(w,"u1",ecl.jd_tdb_u1,ecl.u1_meta);
	wr_csv_node(w,"u2",ecl.jd_tdb_u2,ecl.u2_meta);
	wr_csv_node(w,"opp",ecl.jd_tdb_opp,ecl.opp_meta);
	wr_csv_node(w,"max",ecl.jd_tdb_max,ecl.max_meta);
	wr_csv_node(w,"u3",ecl.jd_tdb_u3,ecl.u3_meta);
	wr_csv_node(w,"u4",ecl.jd_tdb_u4,ecl.u4_meta);
	wr_csv_node(w,"p4",ecl.jd_tdb_p4,ecl.p4_meta);
}

void wr_csv_ecl_blank(CsvWriter&w){
	w.write_raw("eclipse_type","");
	wr_csv_num(w,"eclipse_gamma",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_eps_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_dt_max_sec",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_dur_pen_sec",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_dur_umb_sec",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_dur_tot_sec",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_rp_re",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_ru_re",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_opp_rp_re",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_opp_ru_re",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_moon_dist_km",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_sun_ra_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_sun_dec_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_sun_sd_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_sun_ehp_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_moon_ra_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_moon_dec_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_moon_sd_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_moon_ehp_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_lib_l_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_lib_b_deg",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_lib_c_deg",std::numeric_limits<double>::quiet_NaN());
	w.write_raw("eclipse_opp_loc_iso","");
	w.write_raw("eclipse_max_loc_iso","");
	wr_csv_num(w,"eclipse_pen_mag",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"eclipse_umb_mag",std::numeric_limits<double>::quiet_NaN());
	w.write_raw("eclipse_p1_loc_iso","");
	w.write_raw("eclipse_u1_loc_iso","");
	w.write_raw("eclipse_u2_loc_iso","");
	w.write_raw("eclipse_u3_loc_iso","");
	w.write_raw("eclipse_u4_loc_iso","");
	w.write_raw("eclipse_p4_loc_iso","");
	wr_csv_node_blank(w,"p1");
	wr_csv_node_blank(w,"u1");
	wr_csv_node_blank(w,"u2");
	wr_csv_node_blank(w,"opp");
	wr_csv_node_blank(w,"max");
	wr_csv_node_blank(w,"u3");
	wr_csv_node_blank(w,"u4");
	wr_csv_node_blank(w,"p4");
}

void wr_csv_sol_ecl(CsvWriter&w,const SolarEclipse&ecl,int tz_off){
	w.write_field("solar_eclipse_type",ecl.type);
	wr_csv_num(w,"solar_eclipse_mag",ecl.mag);
	wr_csv_num(w,"solar_eclipse_obscuration",ecl.obscuration);
	wr_csv_num(w,"solar_eclipse_gamma",ecl.gamma);
	wr_csv_num(w,"solar_eclipse_sep_max_deg",ecl.sep_max_deg);
	wr_csv_num(w,"solar_eclipse_sun_sd_max_deg",ecl.sun_sd_max_deg);
	wr_csv_num(w,"solar_eclipse_moon_sd_max_deg",ecl.moon_sd_max_deg);
	wr_csv_num(w,"solar_eclipse_sun_dist_km",ecl.sun_dist_km);
	wr_csv_num(w,"solar_eclipse_moon_dist_km",ecl.moon_dist_km);
	wr_csv_num(w,"solar_eclipse_dt_max_sec",ecl.dt_max_sec);
	wr_csv_num(w,"solar_eclipse_rp_re",ecl.rp_re);
	wr_csv_num(w,"solar_eclipse_ru_re",ecl.ru_re);
	wr_csv_liso(w,"solar_eclipse_c1_loc_iso",ecl.jd_tdb_c1,tz_off);
	wr_csv_liso(w,"solar_eclipse_c2_loc_iso",ecl.jd_tdb_c2,tz_off);
	wr_csv_liso(w,"solar_eclipse_max_loc_iso",ecl.jd_tdb_max,tz_off);
	wr_csv_liso(w,"solar_eclipse_c3_loc_iso",ecl.jd_tdb_c3,tz_off);
	wr_csv_liso(w,"solar_eclipse_c4_loc_iso",ecl.jd_tdb_c4,tz_off);
}

void wr_csv_sol_ecl_blank(CsvWriter&w){
	w.write_raw("solar_eclipse_type","");
	wr_csv_num(w,"solar_eclipse_mag",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_obscuration",
			   std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_gamma",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_sep_max_deg",
			   std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_sun_sd_max_deg",
			   std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_moon_sd_max_deg",
			   std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_sun_dist_km",
			   std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_moon_dist_km",
			   std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_dt_max_sec",
			   std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_rp_re",std::numeric_limits<double>::quiet_NaN());
	wr_csv_num(w,"solar_eclipse_ru_re",std::numeric_limits<double>::quiet_NaN());
	w.write_raw("solar_eclipse_c1_loc_iso","");
	w.write_raw("solar_eclipse_c2_loc_iso","");
	w.write_raw("solar_eclipse_max_loc_iso","");
	w.write_raw("solar_eclipse_c3_loc_iso","");
	w.write_raw("solar_eclipse_c4_loc_iso","");
}

void wr_eljs(std::ostream&os,const std::string&ephem,const std::string&tz,
			 bool pretty,const std::vector<EventRec>&events,
			 const std::string&type,EphRead&eph,bool calc_eclipse=false,
			 int tz_off=0){
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz,{"type="+type});
	w.key("data");
	w.arr_begin();
	for(const auto&ev : events){
		wr_ejson(w,ev,eph,calc_eclipse,tz_off);
	}
	w.arr_end();
	w.obj_end();
	os<<"\n";
}

void wr_eltxt(std::ostream&os,const std::string&tz,
			  const std::vector<EventRec>&events,const std::string&type,
			  EphRead*eph=nullptr,bool calc_eclipse=false,int tz_off=0){
	constexpr int kEclSummaryCols=33;
	constexpr int kEclNodeCols=8*7;
	constexpr int kEclExtraCols=kEclSummaryCols+kEclNodeCols;
	constexpr int kSolarEclExtraCols=17;
	os<<"tool=lunar format=txt type="<<type<<" tz_display="<<tz<<"\n";
	os<<"kind\tcode\tname\tyear\tjd_tdb\tjd_utc\tutc_iso\tloc_iso";
	if(calc_eclipse){
		os<<"\tecl_type\tecl_gamma\tecl_eps_deg\tecl_dt_max_sec\tecl_dur_pen_sec"
		  <<"\tecl_dur_umb_sec\tecl_dur_tot_sec\tecl_rp_re\tecl_ru_re"
		  <<"\tecl_opp_rp_re\tecl_opp_ru_re\tecl_moon_dist_km\tecl_sun_ra_deg"
		  <<"\tecl_sun_dec_deg\tecl_sun_sd_deg\tecl_sun_ehp_deg"
		  <<"\tecl_moon_ra_deg\tecl_moon_dec_deg\tecl_moon_sd_deg"
		  <<"\tecl_moon_ehp_deg\tecl_lib_l_deg\tecl_lib_b_deg\tecl_lib_c_deg"
		  <<"\tecl_opp_liso\tecl_max_liso\tecl_pen_mag\tecl_umb_mag"
		  <<"\tecl_p1_liso\tecl_u1_liso\tecl_u2_liso\tecl_u3_liso\tecl_u4_liso"
		  <<"\tecl_p4_liso\t"
		  <<"p1_jd_ut1\tp1_jd_td\tp1_jd\tp1_zen_lat_deg\tp1_zen_lon_deg\t"
		  <<"p1_pa_deg\tp1_axis_deg\tu1_jd_ut1\tu1_jd_td\tu1_jd\tu1_zen_lat_deg\t"
		  <<"u1_zen_lon_deg\tu1_pa_deg\tu1_axis_deg\tu2_jd_ut1\tu2_jd_td\tu2_jd\t"
		  <<"u2_zen_lat_deg\tu2_zen_lon_deg\tu2_pa_deg\tu2_axis_deg\t"
		  <<"opp_jd_ut1\topp_jd_td\topp_jd\topp_zen_lat_deg\topp_zen_lon_deg\t"
		  <<"opp_pa_deg\topp_axis_deg\t"
		  <<"max_jd_ut1\tmax_jd_td\tmax_jd\tmax_zen_lat_deg\tmax_zen_lon_deg\t"
		  <<"max_pa_deg\tmax_axis_deg\tu3_jd_ut1\tu3_jd_td\tu3_jd\tu3_zen_lat_deg\t"
		  <<"u3_zen_lon_deg\tu3_pa_deg\tu3_axis_deg\tu4_jd_ut1\tu4_jd_td\tu4_jd\t"
		  <<"u4_zen_lat_deg\tu4_zen_lon_deg\tu4_pa_deg\tu4_axis_deg\t"
		  <<"p4_jd_ut1\tp4_jd_td\tp4_jd\tp4_zen_lat_deg\tp4_zen_lon_deg\t"
		  <<"p4_pa_deg\tp4_axis_deg"
		  <<"\tsolar_ecl_type\tsolar_ecl_mag\tsolar_ecl_obscuration"
		  <<"\tsolar_ecl_gamma\tsolar_ecl_sep_max_deg"
		  <<"\tsolar_ecl_sun_sd_max_deg\tsolar_ecl_moon_sd_max_deg"
		  <<"\tsolar_ecl_sun_dist_km\tsolar_ecl_moon_dist_km"
		  <<"\tsolar_ecl_dt_max_sec\tsolar_ecl_rp_re\tsolar_ecl_ru_re"
		  <<"\tsolar_ecl_c1_liso\tsolar_ecl_c2_liso\tsolar_ecl_max_liso"
		  <<"\tsolar_ecl_c3_liso\tsolar_ecl_c4_liso";
	}
	os<<"\n";
	for(const auto&ev : events){
		os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t"
		  <<node_num(event_jd_tdb(ev))<<"\t"<<format_num(ev.jd_utc)<<"\t"
		  <<ev.utc_iso<<"\t"<<ev.loc_iso;
		if(calc_eclipse){
			if(eph&&(is_full_moon_ev(ev)||ev.kind=="lunar_eclipse")){
				LunarEclipse ecl=calc_ecl_for_event(*eph,ev);
				auto out_num=[&](double v){
					if(std::isfinite(v)){
						os<<format_num(v);
					}else{
						os<<"null";
					}
				};
				os<<"\t"<<ecl.type;
				os<<"\t";
				out_num(ecl.gamma);
				os<<"\t";
				out_num(ecl.eps_deg);
				os<<"\t";
				out_num(ecl.dt_max_sec);
				os<<"\t";
				out_num(ecl.dur_pen_sec);
				os<<"\t";
				out_num(ecl.dur_umb_sec);
				os<<"\t";
				out_num(ecl.dur_tot_sec);
				os<<"\t";
				out_num(ecl.rp_re);
				os<<"\t";
				out_num(ecl.ru_re);
				os<<"\t";
				out_num(ecl.opp_rp_re);
				os<<"\t";
				out_num(ecl.opp_ru_re);
				os<<"\t";
				out_num(ecl.moon_dist_km);
				os<<"\t";
				out_num(ecl.sun_geo.ra_deg);
				os<<"\t";
				out_num(ecl.sun_geo.dec_deg);
				os<<"\t";
				out_num(ecl.sun_geo.sd_deg);
				os<<"\t";
				out_num(ecl.sun_geo.ehp_deg);
				os<<"\t";
				out_num(ecl.moon_geo.ra_deg);
				os<<"\t";
				out_num(ecl.moon_geo.dec_deg);
				os<<"\t";
				out_num(ecl.moon_geo.sd_deg);
				os<<"\t";
				out_num(ecl.moon_geo.ehp_deg);
				os<<"\t";
				out_num(ecl.lib.l_deg);
				os<<"\t";
				out_num(ecl.lib.b_deg);
				os<<"\t";
				out_num(ecl.lib.c_deg);
				os<<"\t"<<node_liso(ecl.jd_tdb_opp,tz_off)
				  <<"\t"<<node_liso(ecl.jd_tdb_max,tz_off);
				os<<"\t";
				out_num(ecl.pen_mag);
				os<<"\t";
				out_num(ecl.umb_mag);
				os<<"\t"<<node_liso(ecl.jd_tdb_p1,tz_off)
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
				for(int i=0;i<kSolarEclExtraCols;++i){
					os<<"\tnull";
				}
			}else if(eph&&(is_new_moon_ev(ev)||ev.kind=="solar_eclipse")){
				for(int i=0;i<kEclExtraCols;++i){
					os<<"\tnull";
				}
				SolarEclipse ecl=calc_sol_ecl_for_event(*eph,ev);
				auto out_num=[&](double v){
					if(std::isfinite(v)){
						os<<format_num(v);
					}else{
						os<<"null";
					}
				};
				os<<"\t"<<ecl.type;
				os<<"\t";
				out_num(ecl.mag);
				os<<"\t";
				out_num(ecl.obscuration);
				os<<"\t";
				out_num(ecl.gamma);
				os<<"\t";
				out_num(ecl.sep_max_deg);
				os<<"\t";
				out_num(ecl.sun_sd_max_deg);
				os<<"\t";
				out_num(ecl.moon_sd_max_deg);
				os<<"\t";
				out_num(ecl.sun_dist_km);
				os<<"\t";
				out_num(ecl.moon_dist_km);
				os<<"\t";
				out_num(ecl.dt_max_sec);
				os<<"\t";
				out_num(ecl.rp_re);
				os<<"\t";
				out_num(ecl.ru_re);
				os<<"\t"<<node_liso(ecl.jd_tdb_c1,tz_off)
				  <<"\t"<<node_liso(ecl.jd_tdb_c2,tz_off)
				  <<"\t"<<node_liso(ecl.jd_tdb_max,tz_off)
				  <<"\t"<<node_liso(ecl.jd_tdb_c3,tz_off)
				  <<"\t"<<node_liso(ecl.jd_tdb_c4,tz_off);
			}else{
				for(int i=0;i<kEclExtraCols+kSolarEclExtraCols;++i){
					os<<"\tnull";
				}
			}
		}
		os<<"\n";
	}
}

void wr_elcsv(std::ostream&os,const std::vector<EventRec>&events,
			  EphRead*eph=nullptr,bool calc_eclipse=false,int tz_off=0){
	CsvWriter csv(os);
	csv.write_header(el_csv_hdr(calc_eclipse));
	for(const auto&ev : events){
		csv.write_field("kind",ev.kind);
		csv.write_field("code",ev.code);
		csv.write_field("name",ev.name);
		csv.write_field("year",ev.year);
		wr_csv_num(csv,"jd_tdb",event_jd_tdb(ev));
		csv.write_raw("jd_utc",format_num(ev.jd_utc));
		csv.write_field("utc_iso",ev.utc_iso);
		csv.write_field("loc_iso",ev.loc_iso);
		if(calc_eclipse){
			if(eph&&(is_full_moon_ev(ev)||ev.kind=="lunar_eclipse")){
				wr_csv_ecl(csv,calc_ecl_for_event(*eph,ev),tz_off);
				wr_csv_sol_ecl_blank(csv);
			}else if(eph&&(is_new_moon_ev(ev)||ev.kind=="solar_eclipse")){
				wr_csv_ecl_blank(csv);
				wr_csv_sol_ecl(csv,calc_sol_ecl_for_event(*eph,ev),tz_off);
			}else{
				wr_csv_ecl_blank(csv);
				wr_csv_sol_ecl_blank(csv);
			}
		}
		csv.finish_row();
	}
}

void wr_eljsl(std::ostream&os,const std::string&ephem,const std::string&tz,
			  const std::vector<EventRec>&events,const std::string&type,
			  EphRead&eph,bool calc_eclipse=false,int tz_off=0){
	for(const auto&ev : events){
		JsonWriter w(os,false);
		w.obj_begin();
		write_meta(w,ephem,tz,{"type="+type});
		w.key("data");
		wr_ejson(w,ev,eph,calc_eclipse,tz_off);
		w.obj_end();
		os<<"\n";
	}
}

std::vector<EventRec> filt_evs(const std::vector<EventRec>&events,
							   const EvtFilt&filter,double jd_from,double jd_to,
							   bool has_ub,bool gt_from){
	std::vector<EventRec> out;
	for(const auto&ev : events){
		if(!pass_flt(ev,filter)){
			continue;
		}
		if(gt_from){
			if(!(ev.jd_utc>jd_from)){
				continue;
			}
		}else if(ev.jd_utc<jd_from){
			continue;
		}
		if(has_ub&&ev.jd_utc>jd_to){
			continue;
		}
		out.push_back(ev);
	}
	return out;
}

std::vector<EventRec> load_evs(EphRead&eph,double jd_from,double jd_to,
							   const EvtFilt&filter,int tz_off,bool quiet,
							   bool gt_from){
	int y1=0,m1=0,d1=0;
	int y2=0,m2=0,d2=0;
	utc2cst(jd_from,y1,m1,d1);
	utc2cst(jd_to,y2,m2,d2);
	int y_start=std::min(y1,y2)-1;
	int y_end=std::max(y1,y2)+1;
	std::set<int> years;
	for(int y=y_start;y<=y_end;++y){
		years.insert(y);
	}
	std::vector<EventRec> events=
		col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr,filter);
	std::sort(events.begin(),events.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return filt_evs(events,filter,jd_from,jd_to,true,gt_from);
}

EventRec nearest_ecl(EphRead&eph,double jd_utc,int tz_off,bool quiet,
					 const std::string&ecl_kind){
	if(ecl_kind!="lunar_eclipse"&&ecl_kind!="solar_eclipse"){
		throw std::invalid_argument("eclipse kind must be lunar_eclipse or solar_eclipse");
	}
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2cst(jd_utc,cst_year,cst_month,cst_day);

	EventRec best;
	bool has_best=false;
	double best_abs=std::numeric_limits<double>::infinity();
	std::set<int> loaded_years;
	std::vector<EventRec> evs;
	EvtFilt filter;
	filter.inc_st=false;
	filter.inc_lph=false;
	filter.inc_lecl=(ecl_kind=="lunar_eclipse");
	filter.inc_secl=(ecl_kind=="solar_eclipse");
	filter.inc_ecl=true;

	for(int span : {2,4,8}){
		std::set<int> years=
			add_years(loaded_years,cst_year-span,cst_year+span);
		merge_evs(evs,col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr,filter));
		for(const auto&ev : evs){
			if(ev.kind!=ecl_kind){
				continue;
			}
			double delta=std::fabs(ev.jd_utc-jd_utc);
			if(delta<best_abs){
				best_abs=delta;
				best=ev;
				has_best=true;
			}
		}
		if(has_best){
			return best;
		}
	}
	throw std::runtime_error("failed to locate nearby eclipse event");
}

std::vector<EventRec> bld_fest(EphRead&eph,int lunar_year,int tz_off,
							   int lunar_day_tz_off,QueryCache*cache=nullptr){
	struct FDef{
		const char*name;
		int m;
		int d;
	};
	static const std::array<FDef,7> defs={{
		{"春节",1,1},
		{"元宵",1,15},
		{"端午",5,5},
		{"七夕",7,7},
		{"中秋",8,15},
		{"重阳",9,9},
		{"腊八",12,8},
	}};

	std::vector<EventRec> out;
	out.reserve(defs.size()+1);
	for(const auto&def : defs){
		GregDate g=
			res_greg(eph,lunar_year,def.m,def.d,false,lunar_day_tz_off,cache);
		EventRec ev;
		ev.kind="festival";
		ev.code=std::to_string(def.m)+"-"+std::to_string(def.d);
		ev.name=lunar::i18n::tr_event_name("festival",ev.code,def.name);
		ev.year=lunar_year;
		ev.jd_utc=g.cstday_jd;
		ev.utc_iso=fmt_iso(ev.jd_utc,0,true);
		ev.loc_iso=fmt_iso(ev.jd_utc,tz_off,true);
		out.push_back(std::move(ev));
	}

	GregDate cny_next=
		res_greg(eph,lunar_year+1,1,1,false,lunar_day_tz_off,cache);
	EventRec eve;
	eve.kind="festival";
	eve.code="12-last";
	eve.name=lunar::i18n::tr_event_name("festival",eve.code,"除夕");
	eve.year=lunar_year;
	eve.jd_utc=cny_next.cstday_jd-1.0;
	eve.utc_iso=fmt_iso(eve.jd_utc,0,true);
	eve.loc_iso=fmt_iso(eve.jd_utc,tz_off,true);
	out.push_back(std::move(eve));

	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return out;
}

}

