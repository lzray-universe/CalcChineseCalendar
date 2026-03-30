namespace{

struct FestOpt{
	std::string ephem;
	int year=0;
	std::string tz;
	std::string lunar_day_tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
};

struct FestRes{
	std::vector<EventRec> rows;
};

struct AlmOpt{
	std::string ephem;
	std::string date_text;
	std::string tz;
	std::string lunar_day_tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
	double hli_lon_deg=120.0;
	HliRuleSet hli_rules;
};

struct AlmRes{
	AtData atd;
	std::vector<EventRec> evs;
	std::vector<EventRec> fests;
};

FestOpt parse_fest(const std::vector<std::string>&args){
	if(args.size()<2){
		throw std::invalid_argument("festival requires: <bsp> <year>");
	}
	InterCfg cfg=load_def();
	FestOpt opt;
	opt.ephem=args[0];
	opt.year=parse_int(args[1],"year");
	opt.tz=cfg.default_tz;
	opt.lunar_day_tz=resolve_lunar_day_tz(cfg);
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"){
		opt.format="txt";
	}
	opt.pretty=cfg.def_prety;
	const OptMap handlers={
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&i,
					const std::string&tag){ opt.tz=req_val(src,i,tag); }},
		{"--lunar-day-tz",[&](const std::vector<std::string>&src,
							  std::size_t&i,const std::string&tag){
			 opt.lunar_day_tz=req_val(src,i,tag);
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&i,
						const std::string&tag){
			 opt.format=to_low(req_val(src,i,tag));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&i,
					 const std::string&tag){ opt.out_path=req_val(src,i,tag); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&i,
						const std::string&tag){
			 opt.pretty=parse_bool01(req_val(src,i,tag),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ opt.quiet=true; }},
	};
	for(std::size_t i=2;i<args.size();++i){
		apply_opt(handlers,args,i,args[i],"festival");
	}
	chk_fmt(opt.format,{"json","txt","csv"},"festival");
	opt.lunar_day_tz=canonical_tz_text(opt.lunar_day_tz);
	return opt;
}

FestRes run_fest(const FestOpt&opt){
	FestRes res;
	int tz_off=parse_tz(opt.tz);
	int lunar_day_tz_off=parse_tz(opt.lunar_day_tz);
	EphRead eph(opt.ephem);
	QueryCache cache(eph);
	res.rows=bld_fest(eph,opt.year,tz_off,lunar_day_tz_off,&cache);
	return res;
}

void write_fest(std::ostream&os,const FestOpt&opt,const FestRes&res){
	EphRead eph(opt.ephem);
	const FmtMap fmts={
		{"json",[&](){
			 wr_eljs(os,opt.ephem,opt.tz,opt.pretty,res.rows,"festival",eph);
		 }},
		{"csv",[&](){ wr_elcsv(os,res.rows); }},
		{"txt",[&](){ wr_eltxt(os,opt.tz,res.rows,"festival"); }},
	};
	run_fmt(fmts,opt.format,"festival");
}

AlmOpt parse_alm(const std::vector<std::string>&args){
	if(args.size()<2){
		throw std::invalid_argument("almanac requires: <bsp> <YYYY-MM-DD>");
	}
	InterCfg cfg=load_def();
	AlmOpt opt;
	opt.ephem=args[0];
	opt.date_text=args[1];
	opt.tz=cfg.default_tz;
	opt.lunar_day_tz=resolve_lunar_day_tz(cfg);
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"){
		opt.format="txt";
	}
	opt.pretty=cfg.def_prety;
	opt.hli_rules=hli_rules_from_cfg(cfg);
	const OptMap handlers={
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&i,
					const std::string&tag){ opt.tz=req_val(src,i,tag); }},
		{"--lunar-day-tz",[&](const std::vector<std::string>&src,
							  std::size_t&i,const std::string&tag){
			 opt.lunar_day_tz=req_val(src,i,tag);
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&i,
						const std::string&tag){
			 opt.format=to_low(req_val(src,i,tag));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&i,
					 const std::string&tag){ opt.out_path=req_val(src,i,tag); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&i,
						const std::string&tag){
			 opt.pretty=parse_bool01(req_val(src,i,tag),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ opt.quiet=true; }},
		{"--lon",[&](const std::vector<std::string>&src,std::size_t&i,
					 const std::string&tag){
			 opt.hli_lon_deg=parse_double(req_val(src,i,tag),tag);
		 }},
		{"--trad",[&](const std::vector<std::string>&src,std::size_t&i,
					  const std::string&tag){
			 std::string value=req_val(src,i,tag);
			 HliProfileCode trad=HliProfileCode::Folk;
			 if(!parse_hli_profile(value,&trad)){
				 throw std::invalid_argument(
					 "invalid --trad: "+value+
					 " (expected folk|ziping|purple|xieji)");
			 }
			 opt.hli_rules=make_hli_rule_set(trad);
		 }},
		{"--year-boundary",[&](const std::vector<std::string>&src,
							   std::size_t&i,const std::string&tag){
			 std::string value=req_val(src,i,tag);
			 HliYearBoundary parsed=HliYearBoundary::LunarNewYear;
			 if(!parse_hli_year_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --year-boundary: "+value+
					 " (expected lichun|lunar_new_year|dongzhi)");
			 }
			 opt.hli_rules.year_boundary=static_cast<int>(parsed);
		 }},
		{"--month-boundary",[&](const std::vector<std::string>&src,
								std::size_t&i,const std::string&tag){
			 std::string value=req_val(src,i,tag);
			 HliMonthBoundary parsed=HliMonthBoundary::LunarFirstDay;
			 if(!parse_hli_month_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --month-boundary: "+value+
					 " (expected solar_term|lunar_first_day)");
			 }
			 opt.hli_rules.month_boundary=static_cast<int>(parsed);
		 }},
		{"--leap-month-mode",[&](const std::vector<std::string>&src,
								 std::size_t&i,const std::string&tag){
			 std::string value=req_val(src,i,tag);
			 HliLeapMonthMode parsed=HliLeapMonthMode::InheritPrevious;
			 if(!parse_hli_leap_month_mode(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --leap-month-mode: "+value+
					 " (expected ignore|inherit_previous|split_midway|"
					 "shift_to_next)");
			 }
			 opt.hli_rules.leap_month_mode=static_cast<int>(parsed);
		 }},
		{"--day-boundary",[&](const std::vector<std::string>&src,
							  std::size_t&i,const std::string&tag){
			 std::string value=req_val(src,i,tag);
			 HliDayBoundary parsed=HliDayBoundary::Hour23;
			 if(!parse_hli_day_boundary(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --day-boundary: "+value+
					 " (expected hour23|hour0)");
			 }
			 opt.hli_rules.day_boundary=static_cast<int>(parsed);
		 }},
	};
	for(std::size_t i=2;i<args.size();++i){
		apply_opt(handlers,args,i,args[i],"almanac");
	}
	chk_fmt(opt.format,{"json","txt","csv"},"almanac");
	opt.lunar_day_tz=canonical_tz_text(opt.lunar_day_tz);
	opt.hli_rules=normalize_hli_rule_set(opt.hli_rules);
	return opt;
}

AlmRes run_alm(const AlmOpt&opt){
	int y=0;
	int m=0;
	int d=0;
	std::tie(y,m,d)=parse_ymd(opt.date_text);
	int tz_off=parse_tz(opt.tz);
	int lunar_day_tz_off=parse_tz(opt.lunar_day_tz);
	double smp_jdutc=civil_midjd(y,m,d,lunar_day_tz_off)+0.5;
	double day_sutc=civil_midjd(y,m,d,lunar_day_tz_off);
	double day_eutc=day_sutc+1.0;

	EphRead eph(opt.ephem);
	QueryCache cache(eph);
	AlmRes res;
	res.atd=at_fromjd(eph,smp_jdutc,tz_off,lunar_day_tz_off,opt.tz,
					  opt.date_text+"T12:00:00",opt.lunar_day_tz,false,false,
					  0.0,opt.hli_lon_deg,&opt.hli_rules,&cache);

	std::set<int> years={y-1,y,y+1};
	std::vector<EventRec> all=col_eyrs(eph,years,tz_off,opt.quiet?nullptr:&std::cerr);
	for(const auto&ev : all){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			res.evs.push_back(ev);
		}
	}

	std::vector<EventRec> f_all=
		bld_fest(eph,res.atd.lunar_date.lunar_year,tz_off,lunar_day_tz_off,&cache);
	for(const auto&ev : f_all){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			res.fests.push_back(ev);
		}
	}
	return res;
}

std::string join_ev_name(const std::vector<EventRec>&rows){
	std::string out;
	for(std::size_t i=0;i<rows.size();++i){
		if(i!=0){
			out.push_back('|');
		}
		out+=rows[i].name;
	}
	return out;
}

void write_alm(std::ostream&os,const AlmOpt&opt,const AlmRes&res){
	EphRead eph(opt.ephem);
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 write_meta(w,opt.ephem,opt.tz,
						{"type=almanac",lunar_day_rule_note(opt.lunar_day_tz)});
			 w.key("input");
			 w.obj_begin();
			 w.key("date");
			 w.value(opt.date_text);
			 w.key("lunar_day_tz");
			 w.value(opt.lunar_day_tz);
			 w.key("lon_deg");
			 w.value(opt.hli_lon_deg);
			 w.obj_end();
			 w.key("data");
			 w.obj_begin();
			 w.key("lunar_date");
			 wr_ljson(w,res.atd.lunar_date);
			 w.key("huangli");
			 wr_hli_json(w,res.atd.hli);
			 w.key("ill_pct");
			 w.value(res.atd.ill_pct);
			 w.key("phase_name");
			 w.value(res.atd.phase_name);
			 w.key("events");
			 w.arr_begin();
			 for(const auto&ev : res.evs){
				 wr_ejson(w,ev,eph);
			 }
			 w.arr_end();
			 w.key("festivals");
			 w.arr_begin();
			 for(const auto&ev : res.fests){
				 wr_ejson(w,ev,eph);
			 }
			 w.arr_end();
			 w.obj_end();
			 w.obj_end();
			 os<<"\n";
		 }},
		{"csv",[&](){
			 os<<"date,lun_label,ill_pct,phase_name,events,festivals,y_lun_gz,"
				  "y_lchun_gz,y_rule_gz,m_gz,d_gz,h_gz,h_true_gz,rule_profile,"
				  "rule_profile_code,y_lun_index,y_lun_stem,y_lun_branch,"
				  "y_lchun_index,y_lchun_stem,y_lchun_branch,y_rule_index,"
				  "y_rule_stem,y_rule_branch,m_gz_index,m_gz_stem,m_gz_branch,"
				  "d_gz_index,d_gz_stem,d_gz_branch,h_gz_index,h_gz_stem,"
				  "h_gz_branch,h_true_gz_index,h_true_gz_stem,h_true_gz_branch,"
				  "year_boundary,year_boundary_code,month_boundary,"
				  "month_boundary_code,leap_month_mode,leap_month_mode_code,"
				  "day_boundary,day_boundary_code,jianchu,bazi_clock,bazi_true,"
				  "duty_god,jianchu_code,duty_god_code,duty_is_yellow,duty_tag,"
				  "clash,chong_sha,zodiac_day,six_he,three_he,pengzu,nayin,"
				  "wuxing_day,fetal_god,meridian,lucky_dir,wealth_dir,mascot_dir,"
				  "sun_noble_dir,moon_noble_dir,xiu28,xiu28_code,xiu28_mod28,"
				  "xiu28_mod28_code,xiu_star,yi_ji_level,yi_ji_rule,"
				  "yi_ji_rule_code,good_gods,bad_gods,yi,ji,rule_profile_key,"
				  "year_boundary_key,month_boundary_key,leap_month_mode_key,"
				  "day_boundary_key,duty_tag_code,clash_branch_code,sha_dir_code,"
				  "zodiac_day_code,six_he_branch_code,three_he_group_code,"
				  "nayin_code,fetal_god_code,meridian_code,lucky_dir_code,"
				  "wealth_dir_code,mascot_dir_code,sun_noble_dir_code,"
				  "moon_noble_dir_code,good_god_codes,bad_god_codes,yi_codes,"
				  "ji_codes,good_god_mask_hex,bad_god_mask_hex,yi_mask_hex,"
				  "ji_mask_hex,hour_slots,hour_slot_indexes,hour_gzs,"
				  "hour_gz_indexes,hour_lucks,hour_is_bad\n";
			 os<<csv_quote(opt.date_text)<<","
			   <<csv_quote(res.atd.lunar_date.lun_label)<<","
			   <<format_num(res.atd.ill_pct)<<","
			   <<csv_quote(res.atd.phase_name)<<","
			   <<csv_quote(join_ev_name(res.evs))<<","
			   <<csv_quote(join_ev_name(res.fests))<<",";
			 wr_hli_csv(os,res.atd.hli,HliCsvLayout::Almanac);
			 os<<"\n";
		 }},
		{"txt",[&](){
			 os<<"tool=lunar format=txt type=almanac tz_display="<<opt.tz<<"\n";
			 os<<"input.date="<<opt.date_text<<"\n";
			 os<<"input.lunar_day_tz="<<opt.lunar_day_tz<<"\n";
			 os<<"input.lon_deg="<<format_num(opt.hli_lon_deg)<<"\n";
			 os<<"data.lun_label="<<res.atd.lunar_date.lun_label<<"\n";
			 wr_hli_txt(os,res.atd.hli,HliTxtLayout::Day);
			 os<<"data.ill_pct="<<format_num(res.atd.ill_pct)<<"\n";
			 os<<"data.phase_name="<<res.atd.phase_name<<"\n";
			 os<<"[events]\n";
			 for(const auto&ev : res.evs){
				 os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.loc_iso
				   <<"\n";
			 }
			 os<<"[festivals]\n";
			 for(const auto&ev : res.fests){
				 os<<ev.name<<"\t"<<ev.loc_iso<<"\n";
			 }
			 wr_hli_hour_txt(os,res.atd.hli);
		 }},
	};
	run_fmt(fmts,opt.format,"almanac");
}

}

int cmd_fest(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_fest();
		return 0;
	}
	FestOpt opt=parse_fest(args);
	FestRes res=run_fest(opt);
	OutTgt out=open_out(opt.out_path);
	write_fest(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}

int cmd_alm(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_alm();
		return 0;
	}
	AlmOpt opt=parse_alm(args);
	AlmRes res=run_alm(opt);
	OutTgt out=open_out(opt.out_path);
	write_alm(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}
