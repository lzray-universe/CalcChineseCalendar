// At-data builders and at-specific serializers.
namespace{

AtData at_fromjd(EphRead&eph,double jd_utc,int tz_disp,int lunar_day_tz_off,
				 const std::string&display_tz,const std::string&time_raw,
				 const std::string&tz_in,bool inc_ev,bool calc_eot,
				 double eot_lon_deg,double hli_lon_deg,
				 const HliRuleSet*hli_rules=nullptr,
				 QueryCache*cache=nullptr){
	AtData out;
	out.time_raw=time_raw;
	out.tz_in=tz_in;
	out.display_tz=display_tz;
	out.lunar_day_tz=fmt_tz(lunar_day_tz_off);
	out.jd_utc=jd_utc;
	out.jd_tdb=TimeScale::utc_to_tdb(jd_utc);

	AppLon local_app(eph);
	AppLon&app=cache?cache->app:local_app;
	auto sun=app.sun_calc(out.jd_tdb);
	auto moon=app.moon_calc(out.jd_tdb);
	out.lam_s=sun.first;
	out.lam_s_dot=sun.second;
	out.lam_m=moon.first;
	out.lam_m_dot=moon.second;

	out.elong=norm2pi(out.lam_m-out.lam_s);
	out.elong_deg=out.elong*180.0/PI;
	out.ill_frac=(1.0-std::cos(out.elong))*0.5;
	out.ill_pct=out.ill_frac*100.0;
	out.waxing=(out.lam_m_dot-out.lam_s_dot)>0.0;
	out.phase_name=phase_elo(out.elong);
	out.lunar_date=res_lun(eph,jd_utc,lunar_day_tz_off,cache);
	out.moon_xg=calc_moon_xg(eph,jd_utc);

	double lon_use=std::isfinite(hli_lon_deg)
					   ?hli_lon_deg
					   :static_cast<double>(tz_disp)/60.0*15.0;
	int ly=0;
	int lm=0;
	int ld=0;
	int lhh=0;
	int lmm=0;
	double lss=0.0;
	jd2greg(jd_utc+static_cast<double>(tz_disp)/1440.0,ly,lm,ld,lhh,lmm,lss);
	LunCal6 local_calc_hli(eph);
	LunCal6&lc_hli=cache?cache->calc:local_calc_hli;
	SolLunCal local_solver_hli(eph);
	SolLunCal&solver_hli=cache?cache->solver:local_solver_hli;
	HliInput hli_in;
	hli_in.jd_utc=jd_utc;
	hli_in.gy=ly;
	hli_in.gm=lm;
	hli_in.gd=ld;
	hli_in.hh=lhh;
	hli_in.mm=lmm;
	hli_in.ss=lss;
	hli_in.tz_off=tz_disp;
	hli_in.lon_deg=lon_use;
	hli_in.lun_year=out.lunar_date.lunar_year;
	hli_in.lun_month=out.lunar_date.lun_mno;
	hli_in.lun_day=out.lunar_date.lunar_day;
	hli_in.lun_leap=out.lunar_date.is_leap;
	hli_in.phase_name=out.phase_name;
	hli_in.moon_xg=out.moon_xg;
	hli_in.rules=
		hli_rules?*hli_rules:make_hli_rule_set(HliProfileCode::Folk);
	out.hli=calc_hli(eph,lc_hli,solver_hli,app,hli_in);
	lunar::i18n::localize_hli(&out.hli);
	lunar::i18n::localize_moon_xg(&out.moon_xg);
	out.inc_ev=inc_ev;
	if(inc_ev){
		out.near_ev=comp_near(eph,jd_utc,tz_disp,lunar_day_tz_off,cache);
	}
	out.has_eot=calc_eot;
	if(calc_eot){
		out.eot=app.eot_calc(jd_utc,eot_lon_deg);
	}

	out.utc_iso=fmt_iso(jd_utc,0,true);
	out.local_iso=fmt_iso(jd_utc,tz_disp,true);
	return out;
}

AtData at_ftxt(EphRead&eph,const std::string&time_raw,
			   const std::string&input_tz,int tz_disp,
			   const std::string&display_tz,int lunar_day_tz_off,
			   bool inc_ev,bool calc_eot,
			   double eot_lon_deg,double hli_lon_deg,
			   const HliRuleSet*hli_rules=nullptr,
			   QueryCache*cache=nullptr){
	IsoTime parsed=parse_iso(time_raw,input_tz);
	std::string tz_in=
		parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(input_tz));
	return at_fromjd(eph,parsed.jd_utc,tz_disp,lunar_day_tz_off,display_tz,
					 time_raw,tz_in,inc_ev,calc_eot,eot_lon_deg,hli_lon_deg,
					 hli_rules,cache);
}

void wr_adjs(JsonWriter&w,const AtData&d,EphRead&eph){
	w.obj_begin();
	w.key("sun_lam");
	w.value(d.lam_s);
	w.key("moon_lam");
	w.value(d.lam_m);
	w.key("elongation_rad");
	w.value(d.elong);
	w.key("elongation_deg");
	w.value(d.elong_deg);
	w.key("ill_frac");
	w.value(d.ill_frac);
	w.key("ill_pct");
	w.value(d.ill_pct);
	w.key("waxing");
	w.value(d.waxing);
	w.key("phase_name");
	w.value(d.phase_name);
	w.key("moon_xg");
	w.obj_begin();
	w.key("region");
	w.value(d.moon_xg.region);
	w.key("star_id");
	w.value(d.moon_xg.star_id);
	w.key("star_name");
	w.value(d.moon_xg.star_name);
	w.key("sep_deg");
	wr_num_or_null(w,d.moon_xg.sep_deg);
	w.obj_end();
	w.key("lunar_date");
	wr_ljson(w,d.lunar_date);
	w.key("huangli");
	wr_hli_json(w,d.hli);
	w.key("eot");
	if(!d.has_eot){
		w.null_val();
	}else{
		w.obj_begin();
		w.key("longitude_deg");
		w.value(d.eot.lon_deg);
		w.key("longitude_rad");
		w.value(d.eot.lon_rad);
		w.key("apparent_solar_time_rad");
		w.value(d.eot.apparent_solar_time_rad);
		w.key("mean_solar_time_rad");
		w.value(d.eot.mean_solar_time_rad);
		w.key("eot_rad");
		w.value(d.eot.eot_rad);
		w.key("eot_minutes");
		w.value(d.eot.eot_minutes);
		w.key("eot_seconds");
		w.value(d.eot.eot_seconds);
		w.obj_end();
	}
	w.key("near_ev");
	if(!d.inc_ev){
		w.null_val();
	}else{
		w.obj_begin();
		w.key("st_prev");
		wr_nslot(w,d.near_ev.solar_prev,eph);
		w.key("st_next");
		wr_nslot(w,d.near_ev.solar_next,eph);
		w.key("lp_prev");
		wr_nslot(w,d.near_ev.phase_prev,eph);
		w.key("lp_next");
		wr_nslot(w,d.near_ev.phase_next,eph);
		w.obj_end();
	}
	w.obj_end();
}

void wr_aijs(JsonWriter&w,const AtData&d){
	w.obj_begin();
	w.key("time_raw");
	w.value(d.time_raw);
	w.key("input_tz");
	w.value(d.tz_in);
	w.key("display_tz");
	w.value(d.display_tz);
	w.key("lunar_day_tz");
	w.value(d.lunar_day_tz);
	w.key("jd_utc");
	w.value(d.jd_utc);
	w.key("jd_tdb");
	w.value(d.jd_tdb);
	w.key("utc_iso");
	w.value(d.utc_iso);
	w.key("loc_iso");
	w.value(d.local_iso);
	w.key("eot_lon_deg");
	if(d.has_eot){
		w.value(d.eot.lon_deg);
	}else{
		w.null_val();
	}
	w.key("hli_lon_deg");
	w.value(d.hli.lon_deg);
	w.obj_end();
}

void wr_etln(std::ostream&os,const std::string&slot,const NearEvt&ne){
	os<<slot<<"\t";
	if(!ne.has){
		os<<"null\tnull\tnull\tnull\tnull\tnull\tnull\tnull\n";
		return;
	}
	const EventRec&ev=ne.event;
	os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t"
	  <<node_num(event_jd_tdb(ev))<<"\t"<<format_num(ev.jd_utc)<<"\t"
	  <<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
}

void wr_atxt(std::ostream&os,const AtData&d,bool hdr_on){
	if(hdr_on){
		os<<"tool=lunar format=txt type=at tz_display="<<d.display_tz<<"\n";
	}
	os<<"input.time_raw="<<d.time_raw<<"\n";
	os<<"input.input_tz="<<d.tz_in<<"\n";
	os<<"input.display_tz="<<d.display_tz<<"\n";
	os<<"input.lunar_day_tz="<<d.lunar_day_tz<<"\n";
	os<<"input.jd_utc="<<format_num(d.jd_utc)<<"\n";
	os<<"input.jd_tdb="<<format_num(d.jd_tdb)<<"\n";
	os<<"input.utc_iso="<<d.utc_iso<<"\n";
	os<<"input.loc_iso="<<d.local_iso<<"\n";
	if(d.has_eot){
		os<<"input.eot_lon_deg="<<format_num(d.eot.lon_deg)<<"\n";
	}
	os<<"data.sun_lam="<<format_num(d.lam_s)<<"\n";
	os<<"data.moon_lam="<<format_num(d.lam_m)<<"\n";
	os<<"data.elongation_rad="<<format_num(d.elong)<<"\n";
	os<<"data.elongation_deg="<<format_num(d.elong_deg)<<"\n";
	os<<"data.ill_frac="<<format_num(d.ill_frac)<<"\n";
	os<<"data.ill_pct="<<format_num(d.ill_pct)<<"\n";
	os<<"data.waxing="<<(d.waxing?"1":"0")<<"\n";
	os<<"data.phase_name="<<d.phase_name<<"\n";
	os<<"data.moon_xg.region="<<d.moon_xg.region<<"\n";
	os<<"data.moon_xg.star_id="<<d.moon_xg.star_id<<"\n";
	os<<"data.moon_xg.star_name="<<d.moon_xg.star_name<<"\n";
	os<<"data.moon_xg.sep_deg="<<format_num(d.moon_xg.sep_deg)<<"\n";
	os<<"data.lunar_year="<<d.lunar_date.lunar_year<<"\n";
	os<<"data.lun_mno="<<d.lunar_date.lun_mno<<"\n";
	os<<"data.lun_leap="<<(d.lunar_date.is_leap?"1":"0")<<"\n";
	os<<"data.lun_mlab="<<d.lunar_date.lun_mlab<<"\n";
	os<<"data.lunar_day="<<d.lunar_date.lunar_day<<"\n";
	os<<"data.lun_label="<<d.lunar_date.lun_label<<"\n";
	wr_hli_txt(os,d.hli,HliTxtLayout::At);
	os<<"data.hli.lon_deg="<<format_num(d.hli.lon_deg)<<"\n";
	os<<"data.hli.eot_min="<<format_num(d.hli.eot_min)<<"\n";
	os<<"data.hli.tst_min="<<format_num(d.hli.tst_min)<<"\n";
	if(d.has_eot){
		os<<"data.eot.longitude_deg="<<format_num(d.eot.lon_deg)<<"\n";
		os<<"data.eot.longitude_rad="<<format_num(d.eot.lon_rad)<<"\n";
		os<<"data.eot.apparent_solar_time_rad="
		  <<format_num(d.eot.apparent_solar_time_rad)<<"\n";
		os<<"data.eot.mean_solar_time_rad="
		  <<format_num(d.eot.mean_solar_time_rad)<<"\n";
		os<<"data.eot.eot_rad="<<format_num(d.eot.eot_rad)<<"\n";
		os<<"data.eot.eot_minutes="<<format_num(d.eot.eot_minutes)<<"\n";
		os<<"data.eot.eot_seconds="<<format_num(d.eot.eot_seconds)<<"\n";
	}
	if(d.inc_ev){
		os<<"[near_ev]\n";
		os<<"slot\tkind\tcode\tname\tyear\tjd_tdb\tjd_utc\tutc_iso\tloc_iso\n";
		wr_etln(os,"st_prev",d.near_ev.solar_prev);
		wr_etln(os,"st_next",d.near_ev.solar_next);
		wr_etln(os,"lp_prev",d.near_ev.phase_prev);
		wr_etln(os,"lp_next",d.near_ev.phase_next);
	}
	wr_hli_hour_txt(os,d.hli);
}

}
