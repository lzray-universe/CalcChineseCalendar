void wr_ejson(JsonWriter&w,const EventRec&ev,EphRead&eph){
	w.obj_begin();
	w.key("kind");
	w.value(ev.kind);
	w.key("code");
	w.value(ev.code);
	w.key("name");
	w.value(ev.name);
	w.key("year");
	w.value(ev.year);
	w.key("jd_tdb");
	if(std::isfinite(ev.jd_tdb)){
		w.value(ev.jd_tdb);
	}else{
		w.null_val();
	}
	w.key("jd_utc");
	w.value(ev.jd_utc);
	w.key("utc_iso");
	w.value(ev.utc_iso);
	w.key("loc_iso");
	w.value(ev.loc_iso);
	if(is_full_moon_ev(ev)){
		w.key("moon_dist_km");
		w.value(full_moon_dist_km(eph,ev.jd_utc));
	}
	w.obj_end();
}

void wr_mj(JsonWriter&w,const MonthRec&m){
	w.obj_begin();
	w.key("label");
	w.value(m.label);
	w.key("month_no");
	w.value(m.month_no);
	w.key("is_leap");
	w.value(m.is_leap);
	w.key("st_jdutc");
	w.value(m.st_jdutc);
	w.key("ed_jdutc");
	w.value(m.ed_jdutc);
	w.key("st_utc");
	w.value(m.st_utc);
	w.key("st_loc");
	w.value(m.st_loc);
	w.key("ed_utc");
	w.value(m.ed_utc);
	w.key("ed_loc");
	w.value(m.ed_loc);
	w.obj_end();
}

void wr_cyjs(JsonWriter&w,const CalYrData&item,bool inc_mode,EphRead&eph,
			 int tz_off){
	w.obj_begin();
	w.key("year");
	w.value(item.year);
	if(inc_mode){
		w.key("mode");
		w.value(item.mode);
	}
	w.key("sol_terms");
	w.arr_begin();
	for(const auto&ev : item.sol_terms){
		wr_ejson(w,ev,eph);
	}
	w.arr_end();
	w.key("lun_phase");
	w.arr_begin();
	for(const auto&ev : item.lun_phase){
		wr_ejson(w,ev,eph);
	}
	w.arr_end();
	if(item.inc_eclipse){
		w.key("lunar_eclipses");
		w.arr_begin();
		for(const auto&ecl : item.eclipses){
			wr_ecljson(w,ecl,item.year,tz_off);
		}
		w.arr_end();
	}
	if(item.inc_month){
		w.key("months");
		w.arr_begin();
		for(const auto&m : item.months){
			wr_mj(w,m);
		}
		w.arr_end();
	}
	w.obj_end();
}

void wr_mjs(std::ostream&os,const std::vector<MonYrData>&data,
			const std::string&ephem,const std::string&tz_display,bool pretty){
	int tz_off=parse_tz(tz_display);
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	w.arr_begin();
	for(const auto&bundle : data){
		w.obj_begin();
		w.key("year");
		w.value(bundle.year);
		w.key("mode");
		w.value(bundle.mode);
		w.key("months");
		w.arr_begin();
		for(const auto&m : bundle.months){
			wr_mj(w,m);
		}
		w.arr_end();
		if(bundle.inc_eclipse){
			w.key("lunar_eclipses");
			w.arr_begin();
			for(const auto&ecl : bundle.eclipses){
				wr_ecljson(w,ecl,bundle.year,tz_off);
			}
			w.arr_end();
		}
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
	os<<"\n";
}

void wr_caljs(std::ostream&os,const std::vector<CalYrData>&years,
			  const std::string&ephem,const std::string&tz_display,bool pretty,
			  EphRead&eph){
	int tz_off=parse_tz(tz_display);
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	if(years.size()==1){
		wr_cyjs(w,years.front(),false,eph,tz_off);
	}else{
		w.arr_begin();
		for(const auto&item : years){
			wr_cyjs(w,item,false,eph,tz_off);
		}
		w.arr_end();
	}
	w.obj_end();
	os<<"\n";
}

void wr_yjs(std::ostream&os,const CalYrData&year_data,const std::string&ephem,
			const std::string&tz_display,bool pretty,EphRead&eph){
	int tz_off=parse_tz(tz_display);
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	wr_cyjs(w,year_data,true,eph,tz_off);
	w.obj_end();
	os<<"\n";
}

void wr_ejdoc(std::ostream&os,const EventRec&event,const std::string&ephem,
			  const std::string&tz_display,bool pretty,EphRead&eph,
			  const LunarEclipse*ecl,bool inc_ecl){
	int tz_off=parse_tz(tz_display);
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	wr_ejson(w,event,eph);
	if(inc_ecl){
		w.key("lunar_eclipse");
		if(ecl){
			wr_ecljson(w,*ecl,event.year,tz_off);
		}else{
			w.null_val();
		}
	}
	w.obj_end();
	os<<"\n";
}

std::string csv_quote(const std::string&s){
	bool need_q=false;
	for(char c : s){
		if(c==','||c=='"'||c=='\n'||c=='\r'){
			need_q=true;
			break;
		}
	}
	if(!need_q){
		return s;
	}
	std::string out="\"";
	for(char c : s){
		if(c=='"'){
			out+="\"\"";
		}else{
			out.push_back(c);
		}
	}
	out.push_back('"');
	return out;
}

IcsEvent ev_toics(const EventRec&ev){
	IcsEvent out;
	std::ostringstream uid;
	uid<<"lunar-"<<ev.kind<<"-"<<ev.code<<"-"<<std::setprecision(12)<<ev.jd_utc;
	out.uid=uid.str();
	out.summary=ev.name;
	std::ostringstream desc;
	desc<<"kind="<<ev.kind<<"; code="<<ev.code
		<<"; jd_utc="<<std::setprecision(17)<<ev.jd_utc;
	if(std::isfinite(ev.jd_tdb)){
		desc<<"; jd_tdb="<<ev.jd_tdb;
	}
	out.desc=desc.str();
	out.jd_utc=ev.jd_utc;
	return out;
}

void wr_eics(std::ostream&os,const std::string&ephem,const std::string&cal_name,
			 const std::vector<EventRec>&events){
	std::vector<IcsEvent> ics_events;
	ics_events.reserve(events.size());
	for(const auto&ev : events){
		ics_events.push_back(ev_toics(ev));
	}
	write_ics(os,"lunar-cli//"+tool_ver(),cal_name,ics_events);
}

void wr_mtxt(std::ostream&os,const std::vector<MonYrData>&data,
			 const std::string&tz_display){
	int tz_off=parse_tz(tz_display);
	os<<"tool=lunar format=txt type=months tz_display="<<tz_display<<"\n";
	os<<"note=--tz仅影响显示，不改变计算\n";
	for(const auto&bundle : data){
		os<<"\n[year="<<bundle.year<<" mode="<<bundle.mode<<"]\n";
		os<<"label\tmonth_no\tis_leap\tst_jd\ted_jd\tst_utc"
			"iso\tst_liso\ted_uiso\ted_liso\n";
		os<<std::setprecision(17);
		for(const auto&m : bundle.months){
			os<<m.label<<"\t"<<m.month_no<<"\t"<<(m.is_leap?1:0)<<"\t"
			  <<m.st_jdutc<<"\t"<<m.ed_jdutc<<"\t"<<m.st_utc<<"\t"<<m.st_loc
			  <<"\t"<<m.ed_utc<<"\t"<<m.ed_loc<<"\n";
		}
		if(bundle.inc_eclipse){
			os<<"## lunar_eclipses\n";
			wr_ecltxt(os,bundle.eclipses,tz_off);
		}
	}
}

void wr_mcsv(std::ostream&os,const std::vector<MonYrData>&data){
	os<<"year,mode,label,month_no,is_leap,st_jdutc,ed_jdutc,start_utc_"
		"iso,st_loc,ed_utc,ed_loc\n";
	os<<std::setprecision(17);
	for(const auto&bundle : data){
		for(const auto&m : bundle.months){
			os<<bundle.year<<","<<csv_quote(bundle.mode)<<","
			  <<csv_quote(m.label)<<","<<m.month_no<<","<<(m.is_leap?1:0)<<","
			  <<m.st_jdutc<<","<<m.ed_jdutc<<","<<csv_quote(m.st_utc)<<","
			  <<csv_quote(m.st_loc)<<","<<csv_quote(m.ed_utc)<<","
			  <<csv_quote(m.ed_loc)<<"\n";
		}
	}
}

void wr_etxt(std::ostream&os,const std::vector<EventRec>&events){
	os<<"kind\tcode\tname\tyear\tjd_tdb\tjd_utc\ttm_uiso\ttm_loc"
		"iso\n";
	os<<std::setprecision(17);
	for(const auto&ev : events){
		os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t";
		if(std::isfinite(ev.jd_tdb)){
			os<<ev.jd_tdb;
		}else{
			os<<"null";
		}
		os<<"\t"<<ev.jd_utc<<"\t"<<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
	}
}

void wr_caltx(std::ostream&os,const std::vector<CalYrData>&years,
			  const std::string&tz_display){
	int tz_off=parse_tz(tz_display);
	os<<"tool=lunar format=txt type=calendar tz_display="<<tz_display<<"\n";
	os<<"note=--tz仅影响显示，不改变计算\n";
	for(const auto&item : years){
		os<<"\n[year="<<item.year<<"]\n";
		os<<"## sol_terms\n";
		wr_etxt(os,item.sol_terms);
		os<<"## lun_phase\n";
		wr_etxt(os,item.lun_phase);
		if(item.inc_month){
			os<<"## months\n";
			os<<"label\tmonth_no\tis_leap\tst_jd\ted_jd\tst_utc"
				"iso\tst_liso\ted_uiso\ted_liso\n";
			os<<std::setprecision(17);
			for(const auto&m : item.months){
				os<<m.label<<"\t"<<m.month_no<<"\t"<<(m.is_leap?1:0)<<"\t"
				  <<m.st_jdutc<<"\t"<<m.ed_jdutc<<"\t"<<m.st_utc<<"\t"<<m.st_loc
				  <<"\t"<<m.ed_utc<<"\t"<<m.ed_loc<<"\n";
			}
		}
		if(item.inc_eclipse){
			os<<"## lunar_eclipses\n";
			wr_ecltxt(os,item.eclipses,tz_off);
		}
	}
}

void wr_ytxt(std::ostream&os,const CalYrData&item,const std::string&tz_display){
	int tz_off=parse_tz(tz_display);
	os<<"tool=lunar format=txt type=year mode="<<item.mode
	  <<" tz_display="<<tz_display<<"\n";
	os<<"note=--tz仅影响显示，不改变计算\n";
	os<<"\n[year="<<item.year<<" mode="<<item.mode<<"]\n";
	os<<"## sol_terms\n";
	wr_etxt(os,item.sol_terms);
	os<<"## lun_phase\n";
	wr_etxt(os,item.lun_phase);
	os<<"## months\n";
	os<<"label\tmonth_no\tis_leap\tst_jd\ted_jd\tst_utc"
		"iso\tst_liso\ted_uiso\ted_liso\n";
	os<<std::setprecision(17);
	for(const auto&m : item.months){
		os<<m.label<<"\t"<<m.month_no<<"\t"<<(m.is_leap?1:0)<<"\t"<<m.st_jdutc
		  <<"\t"<<m.ed_jdutc<<"\t"<<m.st_utc<<"\t"<<m.st_loc<<"\t"<<m.ed_utc
		  <<"\t"<<m.ed_loc<<"\n";
	}
	if(item.inc_eclipse){
		os<<"## lunar_eclipses\n";
		wr_ecltxt(os,item.eclipses,tz_off);
	}
}

void wr_setxt(std::ostream&os,const EventRec&ev,const std::string&tz_display,
			  const LunarEclipse*ecl,bool inc_ecl){
	int tz_off=parse_tz(tz_display);
	os<<"tool=lunar format=txt type=event tz_display="<<tz_display<<"\n";
	os<<"kind\tcode\tname\tyear\tjd_tdb\tjd_utc\ttm_uiso\ttm_loc"
		"iso\n";
	os<<std::setprecision(17);
	os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t";
	if(std::isfinite(ev.jd_tdb)){
		os<<ev.jd_tdb;
	}else{
		os<<"null";
	}
	os<<"\t"<<ev.jd_utc<<"\t"<<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
	if(inc_ecl){
		os<<"[lunar_eclipse]\n";
		if(ecl==nullptr){
			os<<"null\n";
			return;
		}
		os<<"has="<<(ecl->has?1:0)<<"\n";
		os<<"type="<<ecl->type<<"\n";
		os<<"pen_mag=";
		wr_num_txt(os,ecl->pen_mag);
		os<<"\n";
		os<<"umb_mag=";
		wr_num_txt(os,ecl->umb_mag);
		os<<"\n";
		os<<"rp_re=";
		wr_num_txt(os,ecl->rp_re);
		os<<"\n";
		os<<"ru_re=";
		wr_num_txt(os,ecl->ru_re);
		os<<"\n";
		os<<"opp_rp_re=";
		wr_num_txt(os,ecl->opp_rp_re);
		os<<"\n";
		os<<"opp_ru_re=";
		wr_num_txt(os,ecl->opp_ru_re);
		os<<"\n";
		os<<"dur_pen_sec=";
		wr_num_txt(os,ecl->dur_pen_sec);
		os<<"\n";
		os<<"dur_umb_sec=";
		wr_num_txt(os,ecl->dur_umb_sec);
		os<<"\n";
		os<<"dur_tot_sec=";
		wr_num_txt(os,ecl->dur_tot_sec);
		os<<"\n";
		os<<"dt_max_sec=";
		wr_num_txt(os,ecl->dt_max_sec);
		os<<"\n";
		os<<"moon_dist_km=";
		wr_num_txt(os,ecl->moon_dist_km);
		os<<"\n";
		os<<"gamma=";
		wr_num_txt(os,ecl->gamma);
		os<<"\n";
		os<<"eps_deg=";
		wr_num_txt(os,ecl->eps_deg);
		os<<"\n";
		os<<"sun_ra_deg=";
		wr_num_txt(os,ecl->sun_geo.ra_deg);
		os<<"\n";
		os<<"sun_dec_deg=";
		wr_num_txt(os,ecl->sun_geo.dec_deg);
		os<<"\n";
		os<<"sun_sd_deg=";
		wr_num_txt(os,ecl->sun_geo.sd_deg);
		os<<"\n";
		os<<"sun_ehp_deg=";
		wr_num_txt(os,ecl->sun_geo.ehp_deg);
		os<<"\n";
		os<<"moon_ra_deg=";
		wr_num_txt(os,ecl->moon_geo.ra_deg);
		os<<"\n";
		os<<"moon_dec_deg=";
		wr_num_txt(os,ecl->moon_geo.dec_deg);
		os<<"\n";
		os<<"moon_sd_deg=";
		wr_num_txt(os,ecl->moon_geo.sd_deg);
		os<<"\n";
		os<<"moon_ehp_deg=";
		wr_num_txt(os,ecl->moon_geo.ehp_deg);
		os<<"\n";
		os<<"lib_l_deg=";
		wr_num_txt(os,ecl->lib.l_deg);
		os<<"\n";
		os<<"lib_b_deg=";
		wr_num_txt(os,ecl->lib.b_deg);
		os<<"\n";
		os<<"lib_c_deg=";
		wr_num_txt(os,ecl->lib.c_deg);
		os<<"\n";
		os<<"p1_loc="<<node_liso(ecl->jd_tdb_p1,tz_off)<<"\n";
		os<<"u1_loc="<<node_liso(ecl->jd_tdb_u1,tz_off)<<"\n";
		os<<"opp_loc="<<node_liso(ecl->jd_tdb_opp,tz_off)<<"\n";
		os<<"max_loc="<<node_liso(ecl->jd_tdb_max,tz_off)<<"\n";
		os<<"u4_loc="<<node_liso(ecl->jd_tdb_u4,tz_off)<<"\n";
		os<<"p4_loc="<<node_liso(ecl->jd_tdb_p4,tz_off)<<"\n";
		os<<"u2_loc="<<node_liso(ecl->jd_tdb_u2,tz_off)<<"\n";
		os<<"u3_loc="<<node_liso(ecl->jd_tdb_u3,tz_off)<<"\n";
		wr_node_kv(os,"p1",ecl->jd_tdb_p1,ecl->p1_meta);
		wr_node_kv(os,"u1",ecl->jd_tdb_u1,ecl->u1_meta);
		wr_node_kv(os,"u2",ecl->jd_tdb_u2,ecl->u2_meta);
		wr_node_kv(os,"max",ecl->jd_tdb_max,ecl->max_meta);
		wr_node_kv(os,"u3",ecl->jd_tdb_u3,ecl->u3_meta);
		wr_node_kv(os,"u4",ecl->jd_tdb_u4,ecl->u4_meta);
		wr_node_kv(os,"p4",ecl->jd_tdb_p4,ecl->p4_meta);
	}
}

void chk_mode(const std::string&mode){
	if(mode!="lunar"&&mode!="gregorian"){
		throw std::invalid_argument("mode must be lunar or gregorian");
	}
}

bool all_digits(const std::string&s){
	if(s.empty()){
		return false;
	}
	for(char c : s){
		if(!std::isdigit(static_cast<unsigned char>(c))){
			return false;
		}
	}
	return true;
}

std::tuple<int,int,int> parse_ld(const std::string&s){
	if(s.empty()){
		throw std::invalid_argument("invalid date, expected YEAR-MM-DD: "+s);
	}
	std::size_t year_sep=s.find('-',((s[0]=='+'||s[0]=='-')?1u:0u));
	std::size_t month_sep=
		(year_sep==std::string::npos)?std::string::npos:s.find('-',year_sep+1);
	if(year_sep==std::string::npos||month_sep==std::string::npos){
		throw std::invalid_argument("invalid date, expected YEAR-MM-DD: "+s);
	}
	std::string ytxt=s.substr(0,year_sep);
	std::string mtxt=s.substr(year_sep+1,month_sep-year_sep-1);
	std::string dtxt=s.substr(month_sep+1);
	if(mtxt.size()!=2||dtxt.size()!=2||!all_digits(mtxt)||!all_digits(dtxt)){
		throw std::invalid_argument("invalid date, expected YEAR-MM-DD: "+s);
	}
	int y=parse_int(ytxt,"year");
	int m=parse_int(mtxt,"month");
	int d=parse_int(dtxt,"day");
	if(m<1||m>12||d<1||d>31){
		throw std::invalid_argument("invalid date value: "+s);
	}
	return {y,m,d};
}

void run_mout(const MonthsArgs&args,const std::vector<MonYrData>&data,
			  const std::string&format,const std::string&out_path){
	OutTgt out=open_out(out_path);
	const FmtMap handlers={
		{"json",[&](){ wr_mjs(*out.stream,data,args.ephem,args.tz,args.pretty); }},
		{"txt",[&](){ wr_mtxt(*out.stream,data,args.tz); }},
		{"csv",[&](){ wr_mcsv(*out.stream,data); }},
	};
	run_fmt(handlers,format,"months");
	note_out(out_path,args.quiet);
}

}

