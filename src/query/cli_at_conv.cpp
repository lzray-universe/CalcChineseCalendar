void cli_at(const AtArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt"},"at");

	int tz_disp=parse_tz(args.tz);
	int lunar_day_tz_off=parse_tz(args.lunar_day_tz);
	EphRead eph(args.ephem);
	QueryCache cache(eph);
	double hli_lon=args.calc_eot
					   ?args.eot_lon_deg
					   :static_cast<double>(tz_disp)/60.0*15.0;
	AtData result=
		at_ftxt(eph,args.time_raw,args.input_tz,tz_disp,args.tz,
				lunar_day_tz_off,args.events,args.calc_eot,args.eot_lon_deg,
				hli_lon,&args.hli_rules,&cache);

	OutTgt out=open_out(args.out);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,args.pretty);
			 w.obj_begin();
			 write_meta(
				 w,args.ephem,args.tz,
				 {lunar_day_rule_note(args.lunar_day_tz),
				  lunar::i18n::pick("--input-tz仅用于解析无时区输入",
									"--input-tz only parses inputs without a timezone suffix.",
									"--input-tz はタイムゾーン無し入力の解釈にのみ使います。",
									"--input-tz 는 시간대 접미사가 없는 입력 해석에만 사용됩니다.")});
			 w.key("input");
			 wr_aijs(w,result);
			 w.key("data");
			 wr_adjs(w,result,eph);
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"txt",[&](){ wr_atxt(*out.stream,result,true); }},
	};
	run_fmt(fmt_handlers,format,"at");
	note_out(args.out,args.quiet);
}

void cli_conv(const ConvArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt"},"convert");

	int tz_disp=parse_tz(args.tz);
	int lunar_day_tz_off=parse_tz(args.lunar_day_tz);

	EphRead eph(args.ephem);
	QueryCache cache(eph);

	bool forward=!args.from_lunar;
	std::string note=
		lunar_day_rule_note(args.lunar_day_tz)+"; "+
		lunar::i18n::pick("--input-tz仅用于解析无时区输入",
						  "--input-tz only parses inputs without a timezone suffix.",
						  "--input-tz はタイムゾーン無し入力の解釈にのみ使います。",
						  "--input-tz 는 시간대 접미사가 없는 입력 해석에만 사용됩니다.");

	OutTgt out=open_out(args.out);
	if(forward){
		IsoTime parsed=parse_iso(args.in_value,args.input_tz);
		std::string tz_in=
			parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(args.input_tz));

		LunDate lunar_date=res_lun(eph,parsed.jd_utc,lunar_day_tz_off,&cache);
		std::string utc_iso=fmt_iso(parsed.jd_utc,0,true);
		std::string local_iso=fmt_iso(parsed.jd_utc,tz_disp,true);

		const FmtMap fmt_handlers={
			{"json",[&](){
				 JsonWriter w(*out.stream,args.pretty);
				 w.obj_begin();
				 write_meta(w,args.ephem,args.tz,{note});

				 w.key("input");
				 w.obj_begin();
				 w.key("direction");
				 w.value("greg2lun");
				 w.key("value_raw");
				 w.value(args.in_value);
				 w.key("input_tz");
				 w.value(tz_in);
				 w.key("display_tz");
				 w.value(args.tz);
				 w.key("lunar_day_tz");
				 w.value(canonical_tz_text(args.lunar_day_tz));
				 w.key("jd_utc");
				 w.value(parsed.jd_utc);
				 w.key("utc_iso");
				 w.value(utc_iso);
				 w.key("loc_iso");
				 w.value(local_iso);
				 w.obj_end();

				 w.key("data");
				 w.obj_begin();
				 w.key("lunar_date");
				 wr_ljson(w,lunar_date);

				 w.key("greg_date");
				 w.obj_begin();
				 w.key("cst_date");
				 w.value(ymd_str(lunar_date.cst_year,lunar_date.cst_month,
								 lunar_date.cst_day));
				 w.key("cstday_jd");
				 w.value(lunar_date.cstday_jd);
				 w.key("cst_uiso");
				 w.value(fmt_iso(lunar_date.cstday_jd,0,true));
				 w.key("cst_liso");
				 w.value(fmt_iso(lunar_date.cstday_jd,tz_disp,true));
				 w.obj_end();
				 w.obj_end();

				 w.obj_end();
				 *out.stream<<"\n";
			 }},
			{"txt",[&](){
				 std::ostream&os=*out.stream;
				 os<<"tool=lunar format=txt type=convert tz_display="<<args.tz
				   <<"\n";
				 os<<"input.direction=greg2lun\n";
				 os<<"input.value_raw="<<args.in_value<<"\n";
				 os<<"input.input_tz="<<tz_in<<"\n";
				 os<<"input.lunar_day_tz="
				   <<canonical_tz_text(args.lunar_day_tz)<<"\n";
				 os<<"input.jd_utc="<<format_num(parsed.jd_utc)<<"\n";
				 os<<"input.utc_iso="<<utc_iso<<"\n";
				 os<<"input.loc_iso="<<local_iso<<"\n";
				 os<<"data.lunar_year="<<lunar_date.lunar_year<<"\n";
				 os<<"data.lun_mno="<<lunar_date.lun_mno<<"\n";
				 os<<"data.lun_leap="<<(lunar_date.is_leap?"1":"0")<<"\n";
				 os<<"data.lun_mlab="<<lunar_date.lun_mlab<<"\n";
				 os<<"data.lunar_day="<<lunar_date.lunar_day<<"\n";
				 os<<"data.lun_label="<<lunar_date.lun_label<<"\n";
				 os<<"data.gcst_date="
				   <<ymd_str(lunar_date.cst_year,lunar_date.cst_month,
							 lunar_date.cst_day)
				   <<"\n";
				 os<<"data.gcst_jd="<<format_num(lunar_date.cstday_jd)<<"\n";
			 }},
		};
		run_fmt(fmt_handlers,format,"convert");
	}else{
		GregDate g=res_greg(eph,args.lunar_year,args.lun_mno,args.lunar_day,
							args.leap,lunar_day_tz_off,&cache);
		LunDate l_check=res_lun(eph,g.cstday_jd,lunar_day_tz_off,&cache);

		const FmtMap fmt_handlers={
			{"json",[&](){
				 JsonWriter w(*out.stream,args.pretty);
				 w.obj_begin();
				 write_meta(w,args.ephem,args.tz,{note});

				 w.key("input");
				 w.obj_begin();
				 w.key("direction");
				 w.value("lun2greg");
				 w.key("lunar_year");
				 w.value(args.lunar_year);
				 w.key("lun_mno");
				 w.value(args.lun_mno);
				 w.key("lunar_day");
				 w.value(args.lunar_day);
				 w.key("is_leap");
				 w.value(args.leap);
				 w.key("lunar_day_tz");
				 w.value(canonical_tz_text(args.lunar_day_tz));
				 w.obj_end();

				 w.key("data");
				 w.obj_begin();
				 w.key("greg_date");
				 w.obj_begin();
				 w.key("cst_date");
				 w.value(ymd_str(g.year,g.month,g.day));
				 w.key("cstday_jd");
				 w.value(g.cstday_jd);
				 w.key("cst_uiso");
				 w.value(fmt_iso(g.cstday_jd,0,true));
				 w.key("cst_liso");
				 w.value(fmt_iso(g.cstday_jd,tz_disp,true));
				 w.obj_end();
				 w.key("lunar_date");
				 wr_ljson(w,l_check);
				 w.obj_end();

				 w.obj_end();
				 *out.stream<<"\n";
			 }},
			{"txt",[&](){
				 std::ostream&os=*out.stream;
				 os<<"tool=lunar format=txt type=convert tz_display="<<args.tz
				   <<"\n";
				 os<<"input.direction=lun2greg\n";
				 os<<"input.lunar_year="<<args.lunar_year<<"\n";
				 os<<"input.lun_mno="<<args.lun_mno<<"\n";
				 os<<"input.lunar_day="<<args.lunar_day<<"\n";
				 os<<"input.lun_leap="<<(args.leap?"1":"0")<<"\n";
				 os<<"input.lunar_day_tz="
				   <<canonical_tz_text(args.lunar_day_tz)<<"\n";
				 os<<"data.gcst_date="<<ymd_str(g.year,g.month,g.day)<<"\n";
				 os<<"data.gcst_jd="<<format_num(g.cstday_jd)<<"\n";
				 os<<"data.gcst_uiso="<<fmt_iso(g.cstday_jd,0,true)<<"\n";
				 os<<"data.gcst_liso="<<fmt_iso(g.cstday_jd,tz_disp,true)
				   <<"\n";
				 os<<"data.lun_label="<<l_check.lun_label<<"\n";
			 }},
		};
		run_fmt(fmt_handlers,format,"convert");
	}

	note_out(args.out,args.quiet);
}

