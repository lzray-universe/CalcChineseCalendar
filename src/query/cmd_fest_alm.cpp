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
	AtData atd=
		at_fromjd(eph,smp_jdutc,tz_off,tz,date_text+"T12:00:00","+08:00",false,
				  false,0.0,hli_lon_deg,&cache);
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
						  "events,festivals,y_lun_gz,y_lchun_gz,m_gz,d_gz,"
						  "h_gz,h_true_gz,jianchu,bazi_clock,bazi_true,duty_god,"
						  "duty_tag,clash,"
						  "chong_sha,zodiac_day,six_he,three_he,pengzu,nayin,"
						  "wuxing_day,fetal_god,meridian,lucky_dir,wealth_dir,"
						  "mascot_dir,sun_noble_dir,moon_noble_dir,xiu28,"
						  "xiu_star,yi_ji_level,yi_ji_rule,good_gods,bad_gods,"
						  "yi,ji\n";
			 *out.stream<<csv_quote(date_text)<<","
						<<csv_quote(atd.lunar_date.lun_label)<<","
						<<format_num(atd.ill_pct)<<","
						<<csv_quote(atd.phase_name)<<","
						<<csv_quote(ev_sum2)<<","<<csv_quote(fest_sum)<<","
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
			 os<<"tool=lunar format=txt type=almanac tz_display="<<tz<<"\n";
			 os<<"input.date="<<date_text<<"\n";
			 os<<"input.lon_deg="<<format_num(hli_lon_deg)<<"\n";
			 os<<"data.lun_label="<<atd.lunar_date.lun_label<<"\n";
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
			 os<<"slot\tgz\tluck\n";
			 for(const auto&x : atd.hli.hour_jx){
				 os<<x.slot<<"\t"<<x.gz<<"\t"<<x.luck<<"\n";
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"almanac");
	note_out(out_path,quiet);
	return 0;
}

