// Eclipse payload writers and event-linked eclipse calculators.
namespace{

void wr_ptvis_json(JsonWriter&w,const LunarEclipsePointVis&pv,int tz_off){
	w.obj_begin();
	w.key("stage_window");
	w.value(pv.stage_window);
	w.key("lat_deg");
	w.value(pv.lat_deg);
	w.key("lon_deg");
	w.value(pv.lon_deg);
	w.key("height_m");
	w.value(pv.height_m);
	w.key("visible");
	w.value(pv.visible);
	w.key("max_alt_deg");
	if(std::isfinite(pv.max_alt_deg)){
		w.value(pv.max_alt_deg);
	}else{
		w.null_val();
	}
	w.key("first_visible");
	if(std::isfinite(pv.first_jd_utc)){
		w.obj_begin();
		w.key("jd_utc");
		w.value(pv.first_jd_utc);
		w.key("utc_iso");
		w.value(fmt_iso(pv.first_jd_utc,0,true));
		w.key("loc_iso");
		w.value(fmt_iso(pv.first_jd_utc,tz_off,true));
		w.obj_end();
	}else{
		w.null_val();
	}
	w.key("last_visible");
	if(std::isfinite(pv.last_jd_utc)){
		w.obj_begin();
		w.key("jd_utc");
		w.value(pv.last_jd_utc);
		w.key("utc_iso");
		w.value(fmt_iso(pv.last_jd_utc,0,true));
		w.key("loc_iso");
		w.value(fmt_iso(pv.last_jd_utc,tz_off,true));
		w.obj_end();
	}else{
		w.null_val();
	}
	w.key("sample_count");
	w.value(pv.sample_count);
	w.obj_end();
}

template<typename GlobalVis>
void wr_glbvis_meta(JsonWriter&w,const GlobalVis&gv,int tz_off,
					bool include_jd_utc){
	w.key("stage_window");
	w.value(gv.stage_window);
	if(include_jd_utc){
		w.key("jd_start_utc");
		w.value(gv.jd_start_utc);
		w.key("jd_end_utc");
		w.value(gv.jd_end_utc);
	}
	w.key("utc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,0,true));
	w.key("utc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,0,true));
	w.key("loc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,tz_off,true));
	w.key("loc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,tz_off,true));
	w.key("lat_step_deg");
	w.value(gv.lat_step_deg);
	w.key("lon_step_deg");
	w.value(gv.lon_step_deg);
	w.key("sample_count");
	w.value(gv.sample_count);
}

void wr_glbvis_json(JsonWriter&w,const LunarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	wr_glbvis_meta(w,gv,tz_off,true);
	w.key("points");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("lat");
		w.value(pt.lat_deg);
		w.key("lon");
		w.value(pt.lon_deg);
		w.key("max_alt_deg");
		w.value(pt.max_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

void wr_glbvis_geojson(JsonWriter&w,const LunarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	w.key("type");
	w.value("FeatureCollection");
	wr_glbvis_meta(w,gv,tz_off,false);
	w.key("features");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("type");
		w.value("Feature");
		w.key("geometry");
		w.obj_begin();
		w.key("type");
		w.value("Point");
		w.key("coordinates");
		w.arr_begin();
		w.value(pt.lon_deg);
		w.value(pt.lat_deg);
		w.arr_end();
		w.obj_end();
		w.key("properties");
		w.obj_begin();
		w.key("max_alt_deg");
		w.value(pt.max_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

void wr_num_array(JsonWriter&w,const std::array<double,4>&items){
	w.arr_begin();
	for(double item : items){
		wr_num_or_null(w,item);
	}
	w.arr_end();
}

void wr_sol_besselian_json(JsonWriter&w,const SolarBesselianElements&b,
						   int tz_off){
	if(!b.has){
		w.null_val();
		return;
	}
	w.obj_begin();
	w.key("epoch");
	if(std::isfinite(b.jd_tdb_epoch)){
		double jd_td=TimeScale::tdb_to_tt(b.jd_tdb_epoch);
		double jd_utc=TimeScale::tdb_to_utc(b.jd_tdb_epoch);
		w.obj_begin();
		w.key("jd_tdb");
		w.value(b.jd_tdb_epoch);
		w.key("jd_td");
		w.value(jd_td);
		w.key("jd_utc");
		w.value(jd_utc);
		w.key("utc_iso");
		w.value(fmt_iso(jd_utc,0,true));
		w.key("td_iso");
		w.value(fmt_iso(jd_td,0,true));
		w.key("loc_iso");
		w.value(fmt_iso(jd_utc,tz_off,true));
		w.obj_end();
	}else{
		w.null_val();
	}
	w.key("time_argument");
	w.value("hours_from_epoch_tdb");
	w.key("polynomial_order");
	w.value(3);
	w.key("x");
	wr_num_or_null(w,b.x);
	w.key("y");
	wr_num_or_null(w,b.y);
	w.key("d_deg");
	wr_num_or_null(w,b.d_deg);
	w.key("mu_deg");
	wr_num_or_null(w,b.mu_deg);
	w.key("l1");
	wr_num_or_null(w,b.l1);
	w.key("l2");
	wr_num_or_null(w,b.l2);
	w.key("tan_f1");
	wr_num_or_null(w,b.tan_f1);
	w.key("tan_f2");
	wr_num_or_null(w,b.tan_f2);
	w.key("x_dot");
	wr_num_or_null(w,b.x_dot);
	w.key("y_dot");
	wr_num_or_null(w,b.y_dot);
	w.key("d_dot_deg");
	wr_num_or_null(w,b.d_dot_deg);
	w.key("mu_dot_deg");
	wr_num_or_null(w,b.mu_dot_deg);
	w.key("l1_dot");
	wr_num_or_null(w,b.l1_dot);
	w.key("l2_dot");
	wr_num_or_null(w,b.l2_dot);
	w.key("coefficients");
	w.obj_begin();
	w.key("x");
	wr_num_array(w,b.x_coeff);
	w.key("y");
	wr_num_array(w,b.y_coeff);
	w.key("d_deg");
	wr_num_array(w,b.d_coeff_deg);
	w.key("mu_deg");
	wr_num_array(w,b.mu_coeff_deg);
	w.key("l1");
	wr_num_array(w,b.l1_coeff);
	w.key("l2");
	wr_num_array(w,b.l2_coeff);
	w.obj_end();
	w.obj_end();
}

void wr_sol_ecljson(JsonWriter&w,const SolarEclipse&ecl,int year,int tz_off){
	w.obj_begin();
	w.key("kind");
	w.value("solar_eclipse");
	w.key("year");
	w.value(year);
	w.key("has");
	w.value(ecl.has);
	w.key("type");
	w.value(ecl.type);
	w.key("mag");
	wr_num_or_null(w,ecl.mag);
	w.key("obscuration");
	wr_num_or_null(w,ecl.obscuration);
	w.key("gamma");
	wr_num_or_null(w,ecl.gamma);
	w.key("sep_max_deg");
	wr_num_or_null(w,ecl.sep_max_deg);
	w.key("sun_sd_max_deg");
	wr_num_or_null(w,ecl.sun_sd_max_deg);
	w.key("moon_sd_max_deg");
	wr_num_or_null(w,ecl.moon_sd_max_deg);
	w.key("sun_dist_km");
	wr_num_or_null(w,ecl.sun_dist_km);
	w.key("moon_dist_km");
	wr_num_or_null(w,ecl.moon_dist_km);
	w.key("dt_max_sec");
	wr_num_or_null(w,ecl.dt_max_sec);
	w.key("rp_re");
	wr_num_or_null(w,ecl.rp_re);
	w.key("ru_re");
	wr_num_or_null(w,ecl.ru_re);
	w.key("besselian");
	wr_sol_besselian_json(w,ecl.besselian,tz_off);
	w.key("c1");
	wr_enode(w,ecl.jd_tdb_c1,tz_off);
	w.key("c2");
	wr_enode(w,ecl.jd_tdb_c2,tz_off);
	w.key("max");
	wr_enode(w,ecl.jd_tdb_max,tz_off);
	w.key("c3");
	wr_enode(w,ecl.jd_tdb_c3,tz_off);
	w.key("c4");
	wr_enode(w,ecl.jd_tdb_c4,tz_off);
	w.obj_end();
}

void wr_sol_ptvis_json(JsonWriter&w,const SolarEclipsePointVis&pv,int tz_off){
	w.obj_begin();
	w.key("stage_window");
	w.value(pv.stage_window);
	w.key("lat_deg");
	w.value(pv.lat_deg);
	w.key("lon_deg");
	w.value(pv.lon_deg);
	w.key("height_m");
	w.value(pv.height_m);
	w.key("has_eclipse");
	w.value(pv.has_eclipse);
	w.key("visible");
	w.value(pv.visible);
	w.key("central");
	w.value(pv.central);
	w.key("max_mag");
	wr_num_or_null(w,pv.max_mag);
	w.key("max_obscuration");
	wr_num_or_null(w,pv.max_obscuration);
	w.key("max_sun_alt_deg");
	wr_num_or_null(w,pv.max_sun_alt_deg);
	w.key("max_loc_iso");
	if(std::isfinite(pv.max_jd_utc)){
		w.value(fmt_iso(pv.max_jd_utc,tz_off,true));
	}else{
		w.null_val();
	}
	auto wr_contact=[&](const char*key,double jd_utc){
		w.key(key);
		if(std::isfinite(jd_utc)){
			w.obj_begin();
			w.key("jd_utc");
			w.value(jd_utc);
			w.key("utc_iso");
			w.value(fmt_iso(jd_utc,0,true));
			w.key("loc_iso");
			w.value(fmt_iso(jd_utc,tz_off,true));
			w.obj_end();
		}else{
			w.null_val();
		}
	};
	wr_contact("c1",pv.c1_jd_utc);
	wr_contact("c2",pv.c2_jd_utc);
	wr_contact("max",pv.max_jd_utc);
	wr_contact("c3",pv.c3_jd_utc);
	wr_contact("c4",pv.c4_jd_utc);
	w.key("first_visible");
	if(std::isfinite(pv.first_jd_utc)){
		w.value(fmt_iso(pv.first_jd_utc,tz_off,true));
	}else{
		w.null_val();
	}
	w.key("last_visible");
	if(std::isfinite(pv.last_jd_utc)){
		w.value(fmt_iso(pv.last_jd_utc,tz_off,true));
	}else{
		w.null_val();
	}
	w.key("sample_count");
	w.value(pv.sample_count);
	w.obj_end();
}

void wr_sol_glbvis_json(JsonWriter&w,const SolarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	wr_glbvis_meta(w,gv,tz_off,true);
	w.key("points");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("lat");
		w.value(pt.lat_deg);
		w.key("lon");
		w.value(pt.lon_deg);
		w.key("max_mag");
		w.value(pt.max_mag);
		w.key("max_sun_alt_deg");
		w.value(pt.max_sun_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

void wr_sol_glbvis_geojson(JsonWriter&w,const SolarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	w.key("type");
	w.value("FeatureCollection");
	wr_glbvis_meta(w,gv,tz_off,false);
	w.key("features");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("type");
		w.value("Feature");
		w.key("geometry");
		w.obj_begin();
		w.key("type");
		w.value("Point");
		w.key("coordinates");
		w.arr_begin();
		w.value(pt.lon_deg);
		w.value(pt.lat_deg);
		w.arr_end();
		w.obj_end();
		w.key("properties");
		w.obj_begin();
		w.key("max_mag");
		w.value(pt.max_mag);
		w.key("max_sun_alt_deg");
		w.value(pt.max_sun_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

LunarEclipse calc_ecl_for_event(EphRead&eph,const EventRec&ev){
	LunarEclipse ecl;
	if(is_full_moon_ev(ev)||ev.kind=="lunar_eclipse"){
		double jd_tdb=
			std::isfinite(ev.jd_tdb)?ev.jd_tdb:TimeScale::utc_to_tdb(ev.jd_utc);
		calc_lunar_eclipse(eph,jd_tdb,&ecl);
	}
	return ecl;
}

SolarEclipse calc_sol_ecl_for_event(EphRead&eph,const EventRec&ev){
	SolarEclipse ecl;
	if(is_new_moon_ev(ev)||ev.kind=="solar_eclipse"){
		double jd_tdb=
			std::isfinite(ev.jd_tdb)?ev.jd_tdb:TimeScale::utc_to_tdb(ev.jd_utc);
		calc_solar_eclipse(eph,jd_tdb,&ecl);
	}
	return ecl;
}

}
