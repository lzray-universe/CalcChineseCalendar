namespace{

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
	os<<"tool=lunar format=txt type="<<type<<" tz_display="<<tz<<"\n";
	os<<"kind\tcode\tname\tyear\tjd_utc\ttm_uiso\ttm_liso";
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
		  <<"p4_pa_deg\tp4_axis_deg";
	}
	os<<"\n";
	for(const auto&ev : events){
		os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t"
		  <<format_num(ev.jd_utc)<<"\t"<<ev.utc_iso<<"\t"<<ev.loc_iso;
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
			}else{
				for(int i=0;i<kEclExtraCols;++i){
					os<<"\tnull";
				}
			}
		}
		os<<"\n";
	}
}

void wr_elcsv(std::ostream&os,const std::vector<EventRec>&events,
			  EphRead*eph=nullptr,bool calc_eclipse=false,int tz_off=0){
	constexpr int kEclSummaryCols=33;
	constexpr int kEclNodeCols=8*7;
	constexpr int kEclExtraCols=kEclSummaryCols+kEclNodeCols;
	os<<"kind,code,name,year,jd_utc,utc_iso,loc_iso";
	if(calc_eclipse){
		os<<",eclipse_type,eclipse_gamma,eclipse_eps_deg,eclipse_dt_max_sec,"
		  <<"eclipse_dur_pen_sec,eclipse_dur_umb_sec,eclipse_dur_tot_sec,"
		  <<"eclipse_rp_re,eclipse_ru_re,eclipse_opp_rp_re,eclipse_opp_ru_re,"
		  <<"eclipse_moon_dist_km,eclipse_sun_ra_deg,eclipse_sun_dec_deg,"
		  <<"eclipse_sun_sd_deg,eclipse_sun_ehp_deg,eclipse_moon_ra_deg,"
		  <<"eclipse_moon_dec_deg,eclipse_moon_sd_deg,eclipse_moon_ehp_deg,"
		  <<"eclipse_lib_l_deg,eclipse_lib_b_deg,eclipse_lib_c_deg,"
		  <<"eclipse_opp_loc_iso,eclipse_max_loc_iso,eclipse_pen_mag,"
		  <<"eclipse_umb_mag,eclipse_p1_loc_iso,eclipse_u1_loc_iso,"
		  <<"eclipse_u2_loc_iso,eclipse_u3_loc_iso,eclipse_u4_loc_iso,"
		  <<"eclipse_p4_loc_iso,"
		  <<"p1_jd_ut1,p1_jd_td,p1_jd,p1_zen_lat_deg,p1_zen_lon_deg,p1_pa_deg,"
		  <<"p1_axis_deg,u1_jd_ut1,u1_jd_td,u1_jd,u1_zen_lat_deg,u1_zen_lon_deg,"
		  <<"u1_pa_deg,u1_axis_deg,u2_jd_ut1,u2_jd_td,u2_jd,u2_zen_lat_deg,"
		  <<"u2_zen_lon_deg,u2_pa_deg,u2_axis_deg,opp_jd_ut1,opp_jd_td,opp_jd,"
		  <<"opp_zen_lat_deg,opp_zen_lon_deg,opp_pa_deg,opp_axis_deg,max_jd_ut1,max_jd_td,max_jd,"
		  <<"max_zen_lat_deg,max_zen_lon_deg,max_pa_deg,max_axis_deg,u3_jd_ut1,"
		  <<"u3_jd_td,u3_jd,u3_zen_lat_deg,u3_zen_lon_deg,u3_pa_deg,u3_axis_deg,"
		  <<"u4_jd_ut1,u4_jd_td,u4_jd,u4_zen_lat_deg,u4_zen_lon_deg,u4_pa_deg,"
		  <<"u4_axis_deg,p4_jd_ut1,p4_jd_td,p4_jd,p4_zen_lat_deg,p4_zen_lon_deg,"
		  <<"p4_pa_deg,p4_axis_deg";
	}
	os<<"\n";
	for(const auto&ev : events){
		os<<csv_quote(ev.kind)<<","<<csv_quote(ev.code)<<","<<csv_quote(ev.name)
		  <<","<<ev.year<<","<<format_num(ev.jd_utc)<<","<<csv_quote(ev.utc_iso)
		  <<","<<csv_quote(ev.loc_iso);
		if(calc_eclipse){
			if(eph&&(is_full_moon_ev(ev)||ev.kind=="lunar_eclipse")){
				LunarEclipse ecl=calc_ecl_for_event(*eph,ev);
				auto out_num=[&](double v){
					if(std::isfinite(v)){
						os<<format_num(v);
					}
				};
				os<<","<<csv_quote(ecl.type)<<",";
				out_num(ecl.gamma);
				os<<",";
				out_num(ecl.eps_deg);
				os<<",";
				out_num(ecl.dt_max_sec);
				os<<",";
				out_num(ecl.dur_pen_sec);
				os<<",";
				out_num(ecl.dur_umb_sec);
				os<<",";
				out_num(ecl.dur_tot_sec);
				os<<",";
				out_num(ecl.rp_re);
				os<<",";
				out_num(ecl.ru_re);
				os<<",";
				out_num(ecl.opp_rp_re);
				os<<",";
				out_num(ecl.opp_ru_re);
				os<<",";
				out_num(ecl.moon_dist_km);
				os<<",";
				out_num(ecl.sun_geo.ra_deg);
				os<<",";
				out_num(ecl.sun_geo.dec_deg);
				os<<",";
				out_num(ecl.sun_geo.sd_deg);
				os<<",";
				out_num(ecl.sun_geo.ehp_deg);
				os<<",";
				out_num(ecl.moon_geo.ra_deg);
				os<<",";
				out_num(ecl.moon_geo.dec_deg);
				os<<",";
				out_num(ecl.moon_geo.sd_deg);
				os<<",";
				out_num(ecl.moon_geo.ehp_deg);
				os<<",";
				out_num(ecl.lib.l_deg);
				os<<",";
				out_num(ecl.lib.b_deg);
				os<<",";
				out_num(ecl.lib.c_deg);
				os<<","<<csv_quote(node_liso(ecl.jd_tdb_opp,tz_off))
				  <<","<<csv_quote(node_liso(ecl.jd_tdb_max,tz_off))<<",";
				out_num(ecl.pen_mag);
				os<<",";
				out_num(ecl.umb_mag);
				os<<","<<csv_quote(node_liso(ecl.jd_tdb_p1,tz_off))
				  <<","<<csv_quote(node_liso(ecl.jd_tdb_u1,tz_off))
				  <<","<<csv_quote(node_liso(ecl.jd_tdb_u2,tz_off))
				  <<","<<csv_quote(node_liso(ecl.jd_tdb_u3,tz_off))
				  <<","<<csv_quote(node_liso(ecl.jd_tdb_u4,tz_off))
				  <<","<<csv_quote(node_liso(ecl.jd_tdb_p4,tz_off));
				auto out_node_csv=[&](double jd_tdb,const EclipsePointMeta&meta){
					if(!std::isfinite(jd_tdb)){
						os<<",,,,,,,";
						return;
					}
					double jd_td=TimeScale::tdb_to_tt(jd_tdb);
					double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
					double jd_ut1=jd_utc;
					os<<","<<format_num(jd_ut1)
					  <<","<<format_num(jd_td)
					  <<","<<format_num(jd_utc)
					  <<","<<node_num(meta.zen_lat_deg)
					  <<","<<node_num(meta.zen_lon_deg)
					  <<","<<node_num(meta.pa_deg)
					  <<","<<node_num(meta.axis_deg);
				};
				out_node_csv(ecl.jd_tdb_p1,ecl.p1_meta);
				out_node_csv(ecl.jd_tdb_u1,ecl.u1_meta);
				out_node_csv(ecl.jd_tdb_u2,ecl.u2_meta);
				out_node_csv(ecl.jd_tdb_opp,ecl.opp_meta);
				out_node_csv(ecl.jd_tdb_max,ecl.max_meta);
				out_node_csv(ecl.jd_tdb_u3,ecl.u3_meta);
				out_node_csv(ecl.jd_tdb_u4,ecl.u4_meta);
				out_node_csv(ecl.jd_tdb_p4,ecl.p4_meta);
			}else{
				for(int i=0;i<kEclExtraCols;++i){
					os<<",";
				}
			}
		}
		os<<"\n";
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
		col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr,filter.inc_ecl);
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

	for(int span : {2,4,8}){
		std::set<int> years;
		for(int y=cst_year-span;y<=cst_year+span;++y){
			years.insert(y);
		}
		std::vector<EventRec> evs=
			col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr,true);
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

