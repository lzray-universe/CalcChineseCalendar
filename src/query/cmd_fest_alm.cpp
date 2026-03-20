namespace{

std::string alm_hex_mask64(std::uint64_t value){
	std::ostringstream oss;
	oss<<"0x"<<std::hex<<std::nouppercase<<std::setw(16)<<std::setfill('0')
	   <<value;
	return oss.str();
}

std::string alm_join_code_pipe(const std::vector<int>&codes){
	std::string out;
	for(std::size_t i=0;i<codes.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=std::to_string(codes[i]);
	}
	return out;
}

std::string alm_join_mask_hex_pipe(const std::array<std::uint64_t,2>&masks){
	std::string out;
	for(std::size_t i=0;i<masks.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=alm_hex_mask64(masks[i]);
	}
	return out;
}

template<class Fn>
std::string alm_join_hour_pipe(const std::vector<HliHour>&hours,Fn&&fn){
	std::string out;
	for(std::size_t i=0;i<hours.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=fn(hours[i]);
	}
	return out;
}

}

int cmd_fest(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_fest();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("festival requires: <bsp> <year>");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	int year=parse_int(args[1],"year");
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
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
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"festival");
	}
	chk_fmt(format,{"json","txt","csv"},"festival");

	int tz_off=parse_tz(tz);
	EphRead eph(ephem);
	QueryCache cache(eph);
	std::vector<EventRec> festivals=bld_fest(eph,year,tz_off,&cache);

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 wr_eljs(*out.stream,ephem,tz,pretty,festivals,"festival",eph);
		 }},
		{"csv",[&](){ wr_elcsv(*out.stream,festivals); }},
		{"txt",[&](){ wr_eltxt(*out.stream,tz,festivals,"festival"); }},
	};
	run_fmt(fmt_handlers,format,"festival");
	note_out(out_path,quiet);
	return 0;
}

int cmd_alm(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_alm();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("almanac requires: <bsp> <YYYY-MM-DD>");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string date_text=args[1];
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	double hli_lon_deg=120.0;
	HliRuleSet hli_rules=hli_rules_from_cfg(cfg);
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
		{"--lon",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){
			 hli_lon_deg=parse_double(req_val(src,idx,opt),opt);
		 }},
		{"--trad",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 std::string value=req_val(src,idx,opt);
			 HliProfileCode trad=HliProfileCode::Folk;
			 if(!parse_hli_profile(value,&trad)){
				 throw std::invalid_argument(
					 "invalid --trad: "+value+
					 " (expected folk|ziping|purple|xieji)");
			 }
			 hli_rules=make_hli_rule_set(trad);
		 }},
		{"--year-boundary",[&](const std::vector<std::string>&src,
							   std::size_t&idx,const std::string&opt){
			 std::string value=req_val(src,idx,opt);
			 HliYearBoundary parsed=HliYearBoundary::LunarNewYear;
			 if(!parse_hli_year_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --year-boundary: "+value+
					 " (expected lichun|lunar_new_year|dongzhi)");
			 }
			 hli_rules.year_boundary=static_cast<int>(parsed);
		 }},
		{"--month-boundary",[&](const std::vector<std::string>&src,
								std::size_t&idx,const std::string&opt){
			 std::string value=req_val(src,idx,opt);
			 HliMonthBoundary parsed=HliMonthBoundary::LunarFirstDay;
			 if(!parse_hli_month_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --month-boundary: "+value+
					 " (expected solar_term|lunar_first_day)");
			 }
			 hli_rules.month_boundary=static_cast<int>(parsed);
		 }},
		{"--leap-month-mode",[&](const std::vector<std::string>&src,
								 std::size_t&idx,const std::string&opt){
			 std::string value=req_val(src,idx,opt);
			 HliLeapMonthMode parsed=HliLeapMonthMode::InheritPrevious;
			 if(!parse_hli_leap_month_mode(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --leap-month-mode: "+value+
					 " (expected ignore|inherit_previous|split_midway|"
					 "shift_to_next)");
			 }
			 hli_rules.leap_month_mode=static_cast<int>(parsed);
		 }},
		{"--day-boundary",[&](const std::vector<std::string>&src,
							  std::size_t&idx,const std::string&opt){
			 std::string value=req_val(src,idx,opt);
			 HliDayBoundary parsed=HliDayBoundary::Hour23;
			 if(!parse_hli_day_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --day-boundary: "+value+
					 " (expected hour23|hour0)");
			 }
			 hli_rules.day_boundary=static_cast<int>(parsed);
		 }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"almanac");
	}
	chk_fmt(format,{"json","txt","csv"},"almanac");

	int y=0,m=0,d=0;
	std::tie(y,m,d)=parse_ymd(date_text);
	int tz_off=parse_tz(tz);
	double smp_jdutc=greg2jd(y,m,d,12,0,0.0)-UTC8DAY;
	double day_sutc=cst_midjd(y,m,d);
	double day_eutc=day_sutc+1.0;

	EphRead eph(ephem);
	QueryCache cache(eph);
	hli_rules=normalize_hli_rule_set(hli_rules);
	AtData atd=
		at_fromjd(eph,smp_jdutc,tz_off,tz,date_text+"T12:00:00","+08:00",false,
				  false,0.0,hli_lon_deg,&hli_rules,&cache);
	std::set<int> years={y-1,y,y+1};
	std::vector<EventRec> all_events=
		col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
	std::vector<EventRec> day_events;
	for(const auto&ev : all_events){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			day_events.push_back(ev);
		}
	}
	std::vector<EventRec> festivals=
		bld_fest(eph,atd.lunar_date.lunar_year,tz_off,&cache);
	std::vector<EventRec> day_fest;
	for(const auto&ev : festivals){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			day_fest.push_back(ev);
		}
	}

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,pretty);
			 w.obj_begin();
			 write_meta(w,ephem,tz,{"type=almanac",lunar::i18n::day_rule_note()});
			 w.key("input");
			 w.obj_begin();
			 w.key("date");
			 w.value(date_text);
			 w.key("lon_deg");
			 w.value(hli_lon_deg);
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
			 w.key("events");
			 w.arr_begin();
			 for(const auto&ev : day_events){
				 wr_ejson(w,ev,eph);
			 }
			 w.arr_end();
			 w.key("festivals");
			 w.arr_begin();
			 for(const auto&ev : day_fest){
				 wr_ejson(w,ev,eph);
			 }
			 w.arr_end();
			 w.obj_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"csv",[&](){
			 std::string ev_sum2;
			 for(std::size_t i=0;i<day_events.size();++i){
				 if(i!=0){
					 ev_sum2+="|";
				 }
				 ev_sum2+=day_events[i].name;
			 }
			 std::string fest_sum;
			 for(std::size_t i=0;i<day_fest.size();++i){
				 if(i!=0){
					 fest_sum+="|";
				 }
				 fest_sum+=day_fest[i].name;
			 }
			 *out.stream<<"date,lun_label,ill_pct,phase_name,"
						 "events,festivals,y_lun_gz,y_lchun_gz,y_rule_gz,m_gz,d_gz,"
						 "h_gz,h_true_gz,rule_profile,rule_profile_code,"
						 "y_lun_index,y_lun_stem,y_lun_branch,y_lchun_index,y_lchun_stem,y_lchun_branch,"
						 "y_rule_index,y_rule_stem,y_rule_branch,m_gz_index,m_gz_stem,m_gz_branch,"
						 "d_gz_index,d_gz_stem,d_gz_branch,h_gz_index,h_gz_stem,h_gz_branch,"
						 "h_true_gz_index,h_true_gz_stem,h_true_gz_branch,"
						 "year_boundary,year_boundary_code,"
						 "month_boundary,month_boundary_code,leap_month_mode,leap_month_mode_code,"
						 "day_boundary,day_boundary_code,jianchu,bazi_clock,bazi_true,duty_god,"
						 "jianchu_code,duty_god_code,duty_is_yellow,duty_tag,clash,"
						 "chong_sha,zodiac_day,six_he,three_he,pengzu,nayin,"
						 "wuxing_day,fetal_god,meridian,lucky_dir,wealth_dir,"
						 "mascot_dir,sun_noble_dir,moon_noble_dir,xiu28,"
						 "xiu28_code,xiu28_mod28,xiu28_mod28_code,xiu_star,"
						 "yi_ji_level,yi_ji_rule,yi_ji_rule_code,good_gods,bad_gods,"
						 "yi,ji,"
						 "rule_profile_key,year_boundary_key,month_boundary_key,leap_month_mode_key,day_boundary_key,"
						 "duty_tag_code,clash_branch_code,sha_dir_code,zodiac_day_code,six_he_branch_code,"
						 "three_he_group_code,nayin_code,fetal_god_code,meridian_code,lucky_dir_code,"
						 "wealth_dir_code,mascot_dir_code,sun_noble_dir_code,moon_noble_dir_code,"
						 "good_god_codes,bad_god_codes,yi_codes,ji_codes,"
						 "good_god_mask_hex,bad_god_mask_hex,yi_mask_hex,ji_mask_hex,"
						 "hour_slots,hour_slot_indexes,hour_gzs,hour_gz_indexes,hour_lucks,hour_is_bad\n";
			 *out.stream<<csv_quote(date_text)<<","
						<<csv_quote(atd.lunar_date.lun_label)<<","
						<<format_num(atd.ill_pct)<<","
						<<csv_quote(atd.phase_name)<<","
						<<csv_quote(ev_sum2)<<","<<csv_quote(fest_sum)<<","
						<<csv_quote(atd.hli.y_lun.text)<<","
						<<csv_quote(atd.hli.y_lchun.text)<<","
						<<csv_quote(atd.hli.y_rule.text)<<","
						<<csv_quote(atd.hli.m_gz.text)<<","
						<<csv_quote(atd.hli.d_gz.text)<<","
						<<csv_quote(atd.hli.h_gz.text)<<","
						<<csv_quote(atd.hli.h_gz_true.text)<<","
						<<csv_quote(atd.hli.rule_profile)<<","
						<<atd.hli.rule_profile_code<<","
						<<gz_index_of(atd.hli.y_lun)<<","
						<<atd.hli.y_lun.stem<<","
						<<atd.hli.y_lun.branch<<","
						<<gz_index_of(atd.hli.y_lchun)<<","
						<<atd.hli.y_lchun.stem<<","
						<<atd.hli.y_lchun.branch<<","
						<<gz_index_of(atd.hli.y_rule)<<","
						<<atd.hli.y_rule.stem<<","
						<<atd.hli.y_rule.branch<<","
						<<gz_index_of(atd.hli.m_gz)<<","
						<<atd.hli.m_gz.stem<<","
						<<atd.hli.m_gz.branch<<","
						<<gz_index_of(atd.hli.d_gz)<<","
						<<atd.hli.d_gz.stem<<","
						<<atd.hli.d_gz.branch<<","
						<<gz_index_of(atd.hli.h_gz)<<","
						<<atd.hli.h_gz.stem<<","
						<<atd.hli.h_gz.branch<<","
						<<gz_index_of(atd.hli.h_gz_true)<<","
						<<atd.hli.h_gz_true.stem<<","
						<<atd.hli.h_gz_true.branch<<","
						<<csv_quote(atd.hli.year_boundary_text)<<","
						<<atd.hli.year_boundary_code<<","
						<<csv_quote(atd.hli.month_boundary_text)<<","
						<<atd.hli.month_boundary_code<<","
						<<csv_quote(atd.hli.leap_month_mode_text)<<","
						<<atd.hli.leap_month_mode_code<<","
						<<csv_quote(atd.hli.day_boundary_text)<<","
						<<atd.hli.day_boundary_code<<","
						<<csv_quote(atd.hli.jianchu)<<","
						<<csv_quote(atd.hli.bazi_clock)<<","
						<<csv_quote(atd.hli.bazi_true)<<","
						<<csv_quote(atd.hli.duty_god)<<","
						<<atd.hli.jianchu_code<<","
						<<atd.hli.duty_god_code<<","
						<<(atd.hli.duty_is_yellow?"1":"0")<<","
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
						<<atd.hli.xiu28_code<<","
						<<csv_quote(atd.hli.xiu28_mod28)<<","
						<<atd.hli.xiu28_mod28_code<<","
						<<csv_quote(atd.hli.xiu_id)<<","
						<<atd.hli.yi_ji_level<<","
						<<csv_quote(atd.hli.yi_ji_rule)<<","
						<<atd.hli.yi_ji_rule_code<<","
						<<csv_quote(join_pipe(atd.hli.good_gods))<<","
						<<csv_quote(join_pipe(atd.hli.bad_gods))<<","
						<<csv_quote(join_pipe(atd.hli.yi))<<","
						<<csv_quote(join_pipe(atd.hli.ji))<<","
						<<csv_quote(hli_profile_key(static_cast<HliProfileCode>(
							   atd.hli.rule_profile_code)))<<","
						<<csv_quote(hli_year_boundary_key(static_cast<HliYearBoundary>(
							   atd.hli.year_boundary_code)))<<","
						<<csv_quote(hli_month_boundary_key(static_cast<HliMonthBoundary>(
							   atd.hli.month_boundary_code)))<<","
						<<csv_quote(hli_leap_month_mode_key(
							   static_cast<HliLeapMonthMode>(
								   atd.hli.leap_month_mode_code)))<<","
						<<csv_quote(hli_day_boundary_key(static_cast<HliDayBoundary>(
							   atd.hli.day_boundary_code)))<<","
						<<atd.hli.duty_tag_code<<","
						<<atd.hli.clash_branch_code<<","
						<<atd.hli.sha_dir_code<<","
						<<atd.hli.zodiac_day_code<<","
						<<atd.hli.six_he_branch_code<<","
						<<atd.hli.three_he_group_code<<","
						<<atd.hli.nayin_code<<","
						<<atd.hli.fetal_god_code<<","
						<<atd.hli.meridian_code<<","
						<<atd.hli.lucky_dir_code<<","
						<<atd.hli.wealth_dir_code<<","
						<<atd.hli.mascot_dir_code<<","
						<<atd.hli.sun_noble_dir_code<<","
						<<atd.hli.moon_noble_dir_code<<","
						<<csv_quote(alm_join_code_pipe(atd.hli.good_god_codes))<<","
						<<csv_quote(alm_join_code_pipe(atd.hli.bad_god_codes))<<","
						<<csv_quote(alm_join_code_pipe(atd.hli.yi_codes))<<","
						<<csv_quote(alm_join_code_pipe(atd.hli.ji_codes))<<","
						<<csv_quote(alm_hex_mask64(atd.hli.good_god_mask))<<","
						<<csv_quote(alm_hex_mask64(atd.hli.bad_god_mask))<<","
						<<csv_quote(alm_join_mask_hex_pipe(atd.hli.yi_mask))<<","
						<<csv_quote(alm_join_mask_hex_pipe(atd.hli.ji_mask))<<","
						<<csv_quote(alm_join_hour_pipe(atd.hli.hour_jx,
											 [](const HliHour&x){ return x.slot; }))<<","
						<<csv_quote(alm_join_hour_pipe(atd.hli.hour_jx,
											 [](const HliHour&x){
												 return std::to_string(
													 x.slot_index);
											 }))<<","
						<<csv_quote(alm_join_hour_pipe(atd.hli.hour_jx,
											 [](const HliHour&x){ return x.gz; }))<<","
						<<csv_quote(alm_join_hour_pipe(atd.hli.hour_jx,
											 [](const HliHour&x){
												 return std::to_string(
													 x.gz_index);
											 }))<<","
						<<csv_quote(alm_join_hour_pipe(atd.hli.hour_jx,
											 [](const HliHour&x){ return x.luck; }))<<","
						<<csv_quote(alm_join_hour_pipe(atd.hli.hour_jx,
											 [](const HliHour&x){
												 return x.is_bad?"1":"0";
											 }))<<"\n";
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=almanac tz_display="<<tz<<"\n";
			 os<<"input.date="<<date_text<<"\n";
			 os<<"input.lon_deg="<<format_num(hli_lon_deg)<<"\n";
			 os<<"data.lun_label="<<atd.lunar_date.lun_label<<"\n";
			 os<<"data.hli.y_lun="<<atd.hli.y_lun.text<<"\n";
			 os<<"data.hli.y_lun_index="<<gz_index_of(atd.hli.y_lun)<<"\n";
			 os<<"data.hli.y_lun_stem="<<atd.hli.y_lun.stem<<"\n";
			 os<<"data.hli.y_lun_branch="<<atd.hli.y_lun.branch<<"\n";
			 os<<"data.hli.y_lchun="<<atd.hli.y_lchun.text<<"\n";
			 os<<"data.hli.y_lchun_index="<<gz_index_of(atd.hli.y_lchun)<<"\n";
			 os<<"data.hli.y_lchun_stem="<<atd.hli.y_lchun.stem<<"\n";
			 os<<"data.hli.y_lchun_branch="<<atd.hli.y_lchun.branch<<"\n";
			 os<<"data.hli.y_rule="<<atd.hli.y_rule.text<<"\n";
			 os<<"data.hli.y_rule_index="<<gz_index_of(atd.hli.y_rule)<<"\n";
			 os<<"data.hli.y_rule_stem="<<atd.hli.y_rule.stem<<"\n";
			 os<<"data.hli.y_rule_branch="<<atd.hli.y_rule.branch<<"\n";
			 os<<"data.hli.month="<<atd.hli.m_gz.text<<"\n";
			 os<<"data.hli.month_index="<<gz_index_of(atd.hli.m_gz)<<"\n";
			 os<<"data.hli.month_stem="<<atd.hli.m_gz.stem<<"\n";
			 os<<"data.hli.month_branch="<<atd.hli.m_gz.branch<<"\n";
			 os<<"data.hli.day="<<atd.hli.d_gz.text<<"\n";
			 os<<"data.hli.day_index="<<gz_index_of(atd.hli.d_gz)<<"\n";
			 os<<"data.hli.day_stem="<<atd.hli.d_gz.stem<<"\n";
			 os<<"data.hli.day_branch="<<atd.hli.d_gz.branch<<"\n";
			 os<<"data.hli.hour="<<atd.hli.h_gz.text<<"\n";
			 os<<"data.hli.hour_index="<<gz_index_of(atd.hli.h_gz)<<"\n";
			 os<<"data.hli.hour_stem="<<atd.hli.h_gz.stem<<"\n";
			 os<<"data.hli.hour_branch="<<atd.hli.h_gz.branch<<"\n";
			 os<<"data.hli.hour_true="<<atd.hli.h_gz_true.text<<"\n";
			 os<<"data.hli.hour_true_index="<<gz_index_of(atd.hli.h_gz_true)
			   <<"\n";
			 os<<"data.hli.hour_true_stem="<<atd.hli.h_gz_true.stem<<"\n";
			 os<<"data.hli.hour_true_branch="<<atd.hli.h_gz_true.branch<<"\n";
			 os<<"data.hli.bazi_clock="<<atd.hli.bazi_clock<<"\n";
			 os<<"data.hli.bazi_true="<<atd.hli.bazi_true<<"\n";
			 os<<"data.hli.rule_profile="<<atd.hli.rule_profile<<"\n";
			 os<<"data.hli.rule_profile_code="<<atd.hli.rule_profile_code<<"\n";
			 os<<"data.hli.rule_profile_key="
			   <<hli_profile_key(
					  static_cast<HliProfileCode>(atd.hli.rule_profile_code))
			   <<"\n";
			 os<<"data.hli.year_boundary="<<atd.hli.year_boundary_text<<"\n";
			 os<<"data.hli.year_boundary_code="<<atd.hli.year_boundary_code<<"\n";
			 os<<"data.hli.year_boundary_key="
			   <<hli_year_boundary_key(
					  static_cast<HliYearBoundary>(atd.hli.year_boundary_code))
			   <<"\n";
			 os<<"data.hli.month_boundary="<<atd.hli.month_boundary_text<<"\n";
			 os<<"data.hli.month_boundary_code="<<atd.hli.month_boundary_code<<"\n";
			 os<<"data.hli.month_boundary_key="
			   <<hli_month_boundary_key(
					  static_cast<HliMonthBoundary>(atd.hli.month_boundary_code))
			   <<"\n";
			 os<<"data.hli.leap_month_mode="<<atd.hli.leap_month_mode_text<<"\n";
			 os<<"data.hli.leap_month_mode_code="<<atd.hli.leap_month_mode_code<<"\n";
			 os<<"data.hli.leap_month_mode_key="
			   <<hli_leap_month_mode_key(static_cast<HliLeapMonthMode>(
					  atd.hli.leap_month_mode_code))
			   <<"\n";
			 os<<"data.hli.day_boundary="<<atd.hli.day_boundary_text<<"\n";
			 os<<"data.hli.day_boundary_code="<<atd.hli.day_boundary_code<<"\n";
			 os<<"data.hli.day_boundary_key="
			   <<hli_day_boundary_key(
					  static_cast<HliDayBoundary>(atd.hli.day_boundary_code))
			   <<"\n";
			 os<<"data.hli.jianchu="<<atd.hli.jianchu<<"\n";
			 os<<"data.hli.jianchu_code="<<atd.hli.jianchu_code<<"\n";
			 os<<"data.hli.duty_god="<<atd.hli.duty_god<<"\n";
			 os<<"data.hli.duty_god_code="<<atd.hli.duty_god_code<<"\n";
			 os<<"data.hli.duty_is_yellow="<<(atd.hli.duty_is_yellow?"1":"0")
			   <<"\n";
			 os<<"data.hli.duty_tag="<<atd.hli.duty_tag<<"\n";
			 os<<"data.hli.duty_tag_code="<<atd.hli.duty_tag_code<<"\n";
			 os<<"data.hli.clash="<<atd.hli.clash<<"\n";
			 os<<"data.hli.clash_branch_code="<<atd.hli.clash_branch_code<<"\n";
			 os<<"data.hli.chong_sha="<<atd.hli.chong_sha<<"\n";
			 os<<"data.hli.sha_dir_code="<<atd.hli.sha_dir_code<<"\n";
			 os<<"data.hli.zodiac_day="<<atd.hli.zodiac_day<<"\n";
			 os<<"data.hli.zodiac_day_code="<<atd.hli.zodiac_day_code<<"\n";
			 os<<"data.hli.six_he="<<atd.hli.six_he<<"\n";
			 os<<"data.hli.six_he_branch_code="<<atd.hli.six_he_branch_code
			   <<"\n";
			 os<<"data.hli.three_he="<<atd.hli.three_he<<"\n";
			 os<<"data.hli.three_he_group_code="<<atd.hli.three_he_group_code
			   <<"\n";
			 os<<"data.hli.pengzu="<<atd.hli.pengzu<<"\n";
			 os<<"data.hli.nayin="<<atd.hli.nayin<<"\n";
			 os<<"data.hli.nayin_code="<<atd.hli.nayin_code<<"\n";
			 os<<"data.hli.wuxing_day="<<atd.hli.wx_day<<"\n";
			 os<<"data.hli.fetal_god="<<atd.hli.fetal_god<<"\n";
			 os<<"data.hli.fetal_god_code="<<atd.hli.fetal_god_code<<"\n";
			 os<<"data.hli.meridian="<<atd.hli.meridian<<"\n";
			 os<<"data.hli.meridian_code="<<atd.hli.meridian_code<<"\n";
			 os<<"data.hli.lucky_dir="<<atd.hli.lucky_dir<<"\n";
			 os<<"data.hli.lucky_dir_code="<<atd.hli.lucky_dir_code<<"\n";
			 os<<"data.hli.wealth_dir="<<atd.hli.wealth_dir<<"\n";
			 os<<"data.hli.wealth_dir_code="<<atd.hli.wealth_dir_code<<"\n";
			 os<<"data.hli.mascot_dir="<<atd.hli.mascot_dir<<"\n";
			 os<<"data.hli.mascot_dir_code="<<atd.hli.mascot_dir_code<<"\n";
			 os<<"data.hli.sun_noble_dir="<<atd.hli.sun_noble_dir<<"\n";
			 os<<"data.hli.sun_noble_dir_code="<<atd.hli.sun_noble_dir_code
			   <<"\n";
			 os<<"data.hli.moon_noble_dir="<<atd.hli.moon_noble_dir<<"\n";
			 os<<"data.hli.moon_noble_dir_code="<<atd.hli.moon_noble_dir_code
			   <<"\n";
			 os<<"data.hli.xiu28="<<atd.hli.xiu28<<"\n";
			 os<<"data.hli.xiu28_code="<<atd.hli.xiu28_code<<"\n";
			 os<<"data.hli.xiu28_mod28="<<atd.hli.xiu28_mod28<<"\n";
			 os<<"data.hli.xiu28_mod28_code="<<atd.hli.xiu28_mod28_code<<"\n";
			 os<<"data.hli.xiu_star="<<atd.hli.xiu_id<<"\n";
			 os<<"data.hli.yi_ji_level="<<atd.hli.yi_ji_level<<"\n";
			 os<<"data.hli.yi_ji_rule="<<atd.hli.yi_ji_rule<<"\n";
			 os<<"data.hli.yi_ji_rule_code="<<atd.hli.yi_ji_rule_code<<"\n";
			 os<<"data.hli.good_gods="<<join_pipe(atd.hli.good_gods)<<"\n";
			 os<<"data.hli.bad_gods="<<join_pipe(atd.hli.bad_gods)<<"\n";
			 os<<"data.hli.yi="<<join_pipe(atd.hli.yi)<<"\n";
			 os<<"data.hli.ji="<<join_pipe(atd.hli.ji)<<"\n";
			 os<<"data.hli.good_god_codes="<<alm_join_code_pipe(atd.hli.good_god_codes)
			   <<"\n";
			 os<<"data.hli.bad_god_codes="<<alm_join_code_pipe(atd.hli.bad_god_codes)
			   <<"\n";
			 os<<"data.hli.yi_codes="<<alm_join_code_pipe(atd.hli.yi_codes)<<"\n";
			 os<<"data.hli.ji_codes="<<alm_join_code_pipe(atd.hli.ji_codes)<<"\n";
			 os<<"data.hli.good_god_mask_hex="<<alm_hex_mask64(atd.hli.good_god_mask)
			   <<"\n";
			 os<<"data.hli.bad_god_mask_hex="<<alm_hex_mask64(atd.hli.bad_god_mask)
			   <<"\n";
			 os<<"data.hli.yi_mask_hex="<<alm_join_mask_hex_pipe(atd.hli.yi_mask)
			   <<"\n";
			 os<<"data.hli.ji_mask_hex="<<alm_join_mask_hex_pipe(atd.hli.ji_mask)
			   <<"\n";
			 os<<"data.ill_pct="<<format_num(atd.ill_pct)<<"\n";
			 os<<"data.phase_name="<<atd.phase_name<<"\n";
			 os<<"[events]\n";
			 for(const auto&ev : day_events){
				 os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.loc_iso
				   <<"\n";
			 }
			 os<<"[festivals]\n";
			 for(const auto&ev : day_fest){
				 os<<ev.name<<"\t"<<ev.loc_iso<<"\n";
			 }
			 os<<"[hour_jx]\n";
			 os<<"slot\tslot_index\tgz\tgz_index\tluck\tis_bad\n";
			 for(const auto&x : atd.hli.hour_jx){
				 os<<x.slot<<"\t"<<x.slot_index<<"\t"<<x.gz<<"\t"<<x.gz_index
				   <<"\t"<<x.luck<<"\t"<<(x.is_bad?"1":"0")<<"\n";
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"almanac");
	note_out(out_path,quiet);
	return 0;
}

