namespace{

struct AtOpt{
	AtArgs run;
	bool batch=false;
	bool warn_time=false;
};

struct AtRow{
	int line_no=0;
	std::string raw;
	bool ok=false;
	std::string error;
	AtData data;
};

struct AtRes{
	bool batch=false;
	bool warn_jobs=false;
	int err_cnt=0;
	AtData one;
	std::vector<AtRow> rows;
};

struct ConvOpt{
	ConvArgs run;
	bool batch=false;
};

struct ConvRow{
	int line_no=0;
	std::string raw;
	bool ok=false;
	std::string error;
	std::string dir;
	double jd_utc=std::numeric_limits<double>::quiet_NaN();
	std::string tz_in;
	LunDate lunar;
	GregDate greg;
};

struct ConvRes{
	bool batch=false;
	bool forward=true;
	bool warn_jobs=false;
	int err_cnt=0;
	double jd_utc=std::numeric_limits<double>::quiet_NaN();
	std::string tz_in;
	LunDate lunar;
	GregDate greg;
	std::vector<ConvRow> rows;
};

double at_hli_lon(const AtArgs&args,int tz_disp){
	return args.calc_eot?args.eot_lon_deg:static_cast<double>(tz_disp)/60.0*15.0;
}

std::string conv_note(const ConvArgs&args){
	return lunar_day_rule_note(args.lunar_day_tz)+"; "+
		   lunar::i18n::pick("--input-tz仅用于解析无时区输入",
							 "--input-tz only parses inputs without a timezone suffix.",
							 "--input-tz はタイムゾーン無し入力の解釈にのみ使います。",
							 "--input-tz 는 시간대 접미사가 없는 입력 해석에만 사용됩니다.");
}

void parse_lun_line(const std::string&raw,int&y,int&m,int&d,bool&leap){
	std::istringstream iss(raw);
	std::string a,b,c,flag,extra;
	if(!(iss>>a>>b>>c)){
		throw std::invalid_argument(
			"expected: <lunar_year> <month_no> <day> [leap]");
	}
	y=parse_int(a,"lunar_year");
	m=parse_int(b,"lun_mno");
	d=parse_int(c,"lunar_day");
	leap=false;
	if(!(iss>>flag)){
		return;
	}
	std::string low=to_low(flag);
	if(flag=="1"||low=="true"||low=="leap"){
		leap=true;
	}else if(flag=="0"||low=="false"||low=="normal"){
		leap=false;
	}else{
		throw std::invalid_argument(
			"invalid leap flag, expected 0/1/true/false");
	}
	if(iss>>extra){
		throw std::invalid_argument(
			"too many fields, expected: <lunar_year> <month_no> <day> [leap]");
	}
}

AtOpt parse_at(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument("at requires: <bsp> <time> or --time <time>");
	}
	InterCfg cfg=load_def();
	AtOpt opt;
	AtArgs&a=opt.run;
	a.ephem=args[0];
	a.input_tz=cfg.default_tz;
	a.tz=cfg.default_tz;
	a.lunar_day_tz=resolve_lunar_day_tz(cfg);
	a.format=to_low(cfg.def_fmt);
	a.pretty=cfg.def_prety;
	if(a.format!="txt"&&a.format!="json"&&a.format!="jsonl"){
		a.format="txt";
	}
	a.hli_rules=hli_rules_from_cfg(cfg);
	a.hli_trad=hli_profile_key(static_cast<HliProfileCode>(a.hli_rules.profile_code));
	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		a.time_raw=args[i++];
	}
	lunar::ArgParser parser;
	parser.add_value("--time",[&](const std::string&v){ a.time_raw=v; })
		.add_flag("--stdin",[&](){ a.from_stdin=true; })
		.add_value("--file",[&](const std::string&v){ a.input_file=v; })
		.add_value("--input-tz",[&](const std::string&v){ a.input_tz=v; })
		.add_value("--tz",[&](const std::string&v){ a.tz=v; })
		.add_value("--lunar-day-tz",[&](const std::string&v){
			a.lunar_day_tz=v;
		})
		.add_value("--format",[&](const std::string&v){ a.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ a.out=v; })
		.add_value("--jobs",[&](const std::string&v){
			a.jobs=parse_int(v,"--jobs");
			if(a.jobs<1){
				throw std::invalid_argument("--jobs must be >= 1");
			}
		})
		.add_value("--meta-once",[&](const std::string&v){
			a.meta_once=parse_bool01(v,"--meta-once");
		})
		.add_value("--pretty",[&](const std::string&v){
			a.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ a.quiet=true; })
		.add_value("--events",[&](const std::string&v){
			a.events=parse_bool01(v,"--events");
		})
		.add_value("--eot-lon",[&](const std::string&v){
			a.eot_lon_deg=parse_double(v,"--eot-lon");
			a.calc_eot=true;
		})
		.add_value("--trad",[&](const std::string&v){
			HliProfileCode p=parse_hli_profile_arg(v,"--trad");
			a.hli_trad=hli_profile_key(p);
			a.hli_rules=make_hli_rule_set(p);
		})
		.add_value("--year-boundary",[&](const std::string&v){
			set_hli_year_boundary(a.hli_rules,v);
		})
		.add_value("--month-boundary",[&](const std::string&v){
			set_hli_month_boundary(a.hli_rules,v);
		})
		.add_value("--leap-month-mode",[&](const std::string&v){
			set_hli_leap_month_mode(a.hli_rules,v);
		})
		.add_value("--day-boundary",[&](const std::string&v){
			set_hli_day_boundary(a.hli_rules,v);
		});
	parser.parse_all(args,i,"at");
	if(a.from_stdin&&!a.input_file.empty()){
		throw std::invalid_argument("--stdin and --file cannot be used together");
	}
	opt.batch=a.from_stdin||!a.input_file.empty();
	opt.warn_time=opt.batch&&!a.time_raw.empty();
	if(opt.warn_time){
		a.time_raw.clear();
	}
	if(a.time_raw.empty()&&!opt.batch){
		throw std::invalid_argument("at requires a <time> or --time <time>");
	}
	if(!opt.batch&&a.format=="jsonl"){
		a.format="json";
		a.pretty=false;
	}
	chk_fmt(a.format,opt.batch?std::set<std::string>{"jsonl","json","txt"}
							 :std::set<std::string>{"json","txt"},
			"at");
	a.lunar_day_tz=canonical_tz_text(a.lunar_day_tz);
	a.hli_rules=normalize_hli_rule_set(a.hli_rules);
	a.hli_trad=hli_profile_key(static_cast<HliProfileCode>(a.hli_rules.profile_code));
	return opt;
}

AtRes run_at(const AtOpt&opt){
	const AtArgs&args=opt.run;
	AtRes res;
	res.batch=opt.batch;
	res.warn_jobs=opt.batch&&args.jobs>1;
	int tz_disp=parse_tz(args.tz);
	int lunar_day_tz_off=parse_tz(args.lunar_day_tz);
	EphRead eph(args.ephem);
	QueryCache cache(eph);
	if(!opt.batch){
		res.one=at_ftxt(eph,args.time_raw,args.input_tz,tz_disp,args.tz,
						lunar_day_tz_off,args.events,args.calc_eot,args.eot_lon_deg,
						at_hli_lon(args,tz_disp),&args.hli_rules,&cache);
		return res;
	}
	std::vector<BatchLine> lines=read_bat(args.from_stdin,args.input_file);
	if(lines.empty()){
		throw std::invalid_argument("batch input is empty");
	}
	res.rows.reserve(lines.size());
	for(const auto&line : lines){
		AtRow row;
		row.line_no=line.line_no;
		row.raw=line.raw;
		try{
			row.data=at_ftxt(eph,line.raw,args.input_tz,tz_disp,args.tz,
							 lunar_day_tz_off,args.events,args.calc_eot,args.eot_lon_deg,
							 at_hli_lon(args,tz_disp),&args.hli_rules,&cache);
			row.ok=true;
		}catch(const std::exception&ex){
			row.error=ex.what();
			++res.err_cnt;
		}
		res.rows.push_back(std::move(row));
	}
	return res;
}

std::vector<std::string> batch_meta_notes(){
	return {"batch=true","schema="+tool_ver()};
}

void write_at(std::ostream&os,const AtOpt&opt,const AtRes&res){
	const AtArgs&args=opt.run;
	if(opt.warn_time&&!args.quiet){
		std::cerr<<"note: positional <time> is ignored in batch mode\n";
	}
	if(res.warn_jobs&&!args.quiet){
		std::cerr<<"note: --jobs is accepted; current batch executor runs seq_run for det_mode behavior.\n";
	}
	if(!opt.batch){
		EphRead eph(args.ephem);
		const FmtMap fmts={
			{"json",[&](){ JsonWriter w(os,args.pretty); w.obj_begin(); write_meta(w,args.ephem,args.tz,{lunar_day_rule_note(args.lunar_day_tz),lunar::i18n::pick("--input-tz仅用于解析无时区输入","--input-tz only parses inputs without a timezone suffix.","--input-tz はタイムゾーン無し入力の解釈にのみ使います。","--input-tz 는 시간대 접미사가 없는 입력 해석에만 사용됩니다.")}); w.key("input"); wr_aijs(w,res.one); w.key("data"); wr_adjs(w,res.one,eph); w.obj_end(); os<<"\n"; }},
			{"txt",[&](){ wr_atxt(os,res.one,true); }},
		};
		run_fmt(fmts,args.format,"at");
		return;
	}
	EphRead eph(args.ephem);
	const FmtMap fmts={
		{"jsonl",[&](){ if(args.meta_once){ JsonWriter w(os,false); w.obj_begin(); w.key("meta"); write_meta(w,args.ephem,args.tz,batch_meta_notes()); w.obj_end(); os<<"\n"; } for(const auto&row : res.rows){ JsonWriter w(os,false); w.obj_begin(); if(!args.meta_once){ write_meta(w,args.ephem,args.tz,batch_meta_notes()); } w.key("line_no"); w.value(row.line_no); w.key("raw"); w.value(row.raw); if(row.ok){ w.key("input"); wr_aijs(w,row.data); w.key("data"); wr_adjs(w,row.data,eph); }else{ w.key("error"); w.obj_begin(); w.key("message"); w.value(row.error); w.obj_end(); } w.obj_end(); os<<"\n"; } }},
		{"json",[&](){ JsonWriter w(os,args.pretty); w.obj_begin(); write_meta(w,args.ephem,args.tz,batch_meta_notes()); w.key("data"); w.arr_begin(); for(const auto&row : res.rows){ w.obj_begin(); w.key("line_no"); w.value(row.line_no); w.key("raw"); w.value(row.raw); if(row.ok){ w.key("input"); wr_aijs(w,row.data); w.key("data"); wr_adjs(w,row.data,eph); }else{ w.key("error"); w.obj_begin(); w.key("message"); w.value(row.error); w.obj_end(); } w.obj_end(); } w.arr_end(); w.obj_end(); os<<"\n"; }},
		{"txt",[&](){ os<<"tool=lunar format=txt type=at-batch tz_display="<<args.tz<<"\n"; os<<"line_no\tstatus\traw\till_pct\tphase_name\tlunar_date\ty_lun_gz\ty_lchun_gz\ty_rule_gz\tm_gz\td_gz\th_gz\th_true_gz\trule_profile\tyear_boundary\tmonth_boundary\tleap_month_mode\tday_boundary\tjianchu\tchong_sha\tyi\tji"; if(args.calc_eot){ os<<"\teot_minutes"; } os<<"\tmessage\n"; for(const auto&row : res.rows){ os<<row.line_no<<"\t"; if(row.ok){ os<<"ok\t"<<row.raw<<"\t"<<format_num(row.data.ill_pct)<<"\t"<<row.data.phase_name<<"\t"<<row.data.lunar_date.lun_label<<"\t"<<row.data.hli.y_lun.text<<"\t"<<row.data.hli.y_lchun.text<<"\t"<<row.data.hli.y_rule.text<<"\t"<<row.data.hli.m_gz.text<<"\t"<<row.data.hli.d_gz.text<<"\t"<<row.data.hli.h_gz.text<<"\t"<<row.data.hli.h_gz_true.text<<"\t"<<row.data.hli.rule_profile<<"\t"<<row.data.hli.year_boundary_text<<"\t"<<row.data.hli.month_boundary_text<<"\t"<<row.data.hli.leap_month_mode_text<<"\t"<<row.data.hli.day_boundary_text<<"\t"<<row.data.hli.jianchu<<"\t"<<row.data.hli.chong_sha<<"\t"<<join_pipe(row.data.hli.yi)<<"\t"<<join_pipe(row.data.hli.ji); if(args.calc_eot){ os<<"\t"<<format_num(row.data.eot.eot_minutes); } os<<"\t\n"; }else{ os<<"error\t"<<row.raw<<"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"; if(args.calc_eot){ os<<"\t"; } os<<"\t"<<row.error<<"\n"; } } }},
	};
	run_fmt(fmts,args.format,"at");
}

ConvOpt parse_conv(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument("convert requires: <bsp> <dt_or_tm> or --from-lunar ...");
	}
	InterCfg cfg=load_def();
	ConvOpt opt;
	ConvArgs&c=opt.run;
	c.ephem=args[0];
	c.input_tz=cfg.default_tz;
	c.tz=cfg.default_tz;
	c.lunar_day_tz=resolve_lunar_day_tz(cfg);
	c.format=to_low(cfg.def_fmt);
	c.pretty=cfg.def_prety;
	if(c.format!="txt"&&c.format!="json"&&c.format!="jsonl"){
		c.format="txt";
	}
	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		c.in_value=args[i++];
		c.has_in=true;
	}
	lunar::ArgParser parser;
	parser.add("--from-lunar",[&](const std::vector<std::string>&src,
								  std::size_t&j,const std::string&){
			c.from_lunar=true;
			if(j+1>=src.size()||is_opt(src[j+1])){
				return;
			}
			if(j+3>=src.size()||is_opt(src[j+2])||is_opt(src[j+3])){
				throw std::invalid_argument(
					"--from-lunar requires: <year> <month_no> <day>");
			}
			c.lunar_year=parse_int(src[++j],"lunar_year");
			c.lun_mno=parse_int(src[++j],"lun_mno");
			c.lunar_day=parse_int(src[++j],"lunar_day");
			c.has_lunar_input=true;
		})
		.add_flag("--stdin",[&](){ c.from_stdin=true; })
		.add_value("--file",[&](const std::string&v){ c.input_file=v; })
		.add_value("--leap",[&](const std::string&v){
			c.leap=parse_bool01(v,"--leap");
		})
		.add_value("--input-tz",[&](const std::string&v){ c.input_tz=v; })
		.add_value("--tz",[&](const std::string&v){ c.tz=v; })
		.add_value("--lunar-day-tz",[&](const std::string&v){
			c.lunar_day_tz=v;
		})
		.add_value("--format",[&](const std::string&v){ c.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ c.out=v; })
		.add_value("--jobs",[&](const std::string&v){
			c.jobs=parse_int(v,"--jobs");
			if(c.jobs<1){
				throw std::invalid_argument("--jobs must be >= 1");
			}
		})
		.add_value("--meta-once",[&](const std::string&v){
			c.meta_once=parse_bool01(v,"--meta-once");
		})
		.add_value("--pretty",[&](const std::string&v){
			c.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ c.quiet=true; });
	parser.parse_all(args,i,"convert");
	if(c.from_lunar&&c.has_in){
		throw std::invalid_argument("do not pass positional <dt_or_tm> when using --from-lunar");
	}
	if(c.from_stdin&&!c.input_file.empty()){
		throw std::invalid_argument("--stdin and --file cannot be used together");
	}
	opt.batch=c.from_stdin||!c.input_file.empty();
	if(!c.from_lunar&&!c.has_in&&!opt.batch){
		throw std::invalid_argument("convert requires positional <dt_or_tm> when not using --from-lunar");
	}
	if(c.from_lunar&&opt.batch&&c.has_lunar_input){
		throw std::invalid_argument("do not pass inline lunar date when using --stdin/--file");
	}
	if(c.from_lunar&&!opt.batch){
		if(!c.has_lunar_input){
			throw std::invalid_argument("--from-lunar requires: <year> <month_no> <day>");
		}
		if(c.lun_mno<1||c.lun_mno>12){
			throw std::invalid_argument("lunar month must be 1..12");
		}
	}
	if(!opt.batch&&c.format=="jsonl"){
		c.format="json";
		c.pretty=false;
	}
	chk_fmt(c.format,opt.batch?std::set<std::string>{"jsonl","json","txt"}
							 :std::set<std::string>{"json","txt"},
			"convert");
	c.lunar_day_tz=canonical_tz_text(c.lunar_day_tz);
	return opt;
}

ConvRes run_conv(const ConvOpt&opt){
	const ConvArgs&args=opt.run;
	ConvRes res;
	res.batch=opt.batch;
	res.forward=!args.from_lunar;
	res.warn_jobs=opt.batch&&args.jobs>1;
	EphRead eph(args.ephem);
	int lunar_day_tz_off=parse_tz(args.lunar_day_tz);
	QueryCache cache(eph);
	if(!opt.batch){
		if(res.forward){
			IsoTime parsed=parse_iso(args.in_value,args.input_tz);
			res.jd_utc=parsed.jd_utc;
			res.tz_in=parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(args.input_tz));
			res.lunar=res_lun(eph,parsed.jd_utc,lunar_day_tz_off,&cache);
			res.greg.year=res.lunar.cst_year;
			res.greg.month=res.lunar.cst_month;
			res.greg.day=res.lunar.cst_day;
			res.greg.cstday_jd=res.lunar.cstday_jd;
		}else{
			res.greg=res_greg(eph,args.lunar_year,args.lun_mno,args.lunar_day,args.leap,lunar_day_tz_off,&cache);
			res.jd_utc=res.greg.cstday_jd;
			res.tz_in=args.tz;
			res.lunar=res_lun(eph,res.greg.cstday_jd,lunar_day_tz_off,&cache);
		}
		return res;
	}
	std::vector<BatchLine> lines=read_bat(args.from_stdin,args.input_file);
	if(lines.empty()){
		throw std::invalid_argument("batch input is empty");
	}
	res.rows.reserve(lines.size());
	for(const auto&line : lines){
		ConvRow row;
		row.line_no=line.line_no;
		row.raw=line.raw;
		try{
			if(res.forward){
				IsoTime parsed=parse_iso(line.raw,args.input_tz);
				row.dir="greg2lun";
				row.jd_utc=parsed.jd_utc;
				row.tz_in=parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(args.input_tz));
				row.lunar=res_lun(eph,parsed.jd_utc,lunar_day_tz_off,&cache);
				row.greg.year=row.lunar.cst_year;
				row.greg.month=row.lunar.cst_month;
				row.greg.day=row.lunar.cst_day;
				row.greg.cstday_jd=row.lunar.cstday_jd;
			}else{
				int y=0,m=0,d=0; bool leap=false;
				parse_lun_line(line.raw,y,m,d,leap);
				row.dir="lun2greg";
				row.greg=res_greg(eph,y,m,d,leap,lunar_day_tz_off,&cache);
				row.jd_utc=row.greg.cstday_jd;
				row.tz_in=args.tz;
				row.lunar=res_lun(eph,row.greg.cstday_jd,lunar_day_tz_off,&cache);
			}
			row.ok=true;
		}catch(const std::exception&ex){
			row.error=ex.what();
			++res.err_cnt;
		}
		res.rows.push_back(std::move(row));
	}
	return res;
}

void write_conv(std::ostream&os,const ConvOpt&opt,const ConvRes&res){
	const ConvArgs&args=opt.run;
	if(res.warn_jobs&&!args.quiet){
		std::cerr<<"note: --jobs is accepted; current batch executor runs seq_run for det_mode behavior.\n";
	}
	const std::string note=conv_note(args);
	int tz_disp=parse_tz(args.tz);
	if(!opt.batch){
		const FmtMap fmts={
			{"json",[&](){ JsonWriter w(os,args.pretty); w.obj_begin(); write_meta(w,args.ephem,args.tz,{note}); w.key("input"); w.obj_begin(); w.key("direction"); w.value(res.forward?"greg2lun":"lun2greg"); if(res.forward){ w.key("value_raw"); w.value(args.in_value); w.key("input_tz"); w.value(res.tz_in); w.key("display_tz"); w.value(args.tz); w.key("jd_utc"); w.value(res.jd_utc); w.key("utc_iso"); w.value(fmt_iso(res.jd_utc,0,true)); w.key("loc_iso"); w.value(fmt_iso(res.jd_utc,tz_disp,true)); }else{ w.key("lunar_year"); w.value(args.lunar_year); w.key("lun_mno"); w.value(args.lun_mno); w.key("lunar_day"); w.value(args.lunar_day); w.key("is_leap"); w.value(args.leap); } w.key("lunar_day_tz"); w.value(canonical_tz_text(args.lunar_day_tz)); w.obj_end(); w.key("data"); w.obj_begin(); if(res.forward){ w.key("lunar_date"); wr_ljson(w,res.lunar); } w.key("greg_date"); w.obj_begin(); w.key("cst_date"); w.value(ymd_str(res.greg.year,res.greg.month,res.greg.day)); w.key("cstday_jd"); w.value(res.greg.cstday_jd); w.key("cst_uiso"); w.value(fmt_iso(res.greg.cstday_jd,0,true)); w.key("cst_liso"); w.value(fmt_iso(res.greg.cstday_jd,tz_disp,true)); w.obj_end(); if(!res.forward){ w.key("lunar_date"); wr_ljson(w,res.lunar); } w.obj_end(); w.obj_end(); os<<"\n"; }},
			{"txt",[&](){ os<<"tool=lunar format=txt type=convert tz_display="<<args.tz<<"\n"; os<<"input.direction="<<(res.forward?"greg2lun":"lun2greg")<<"\n"; if(res.forward){ os<<"input.value_raw="<<args.in_value<<"\n"; os<<"input.input_tz="<<res.tz_in<<"\n"; os<<"input.jd_utc="<<format_num(res.jd_utc)<<"\n"; os<<"input.utc_iso="<<fmt_iso(res.jd_utc,0,true)<<"\n"; os<<"input.loc_iso="<<fmt_iso(res.jd_utc,tz_disp,true)<<"\n"; os<<"data.lunar_year="<<res.lunar.lunar_year<<"\n"; os<<"data.lun_mno="<<res.lunar.lun_mno<<"\n"; os<<"data.lun_leap="<<(res.lunar.is_leap?"1":"0")<<"\n"; os<<"data.lun_mlab="<<res.lunar.lun_mlab<<"\n"; os<<"data.lunar_day="<<res.lunar.lunar_day<<"\n"; os<<"data.lun_label="<<res.lunar.lun_label<<"\n"; }else{ os<<"input.lunar_year="<<args.lunar_year<<"\n"; os<<"input.lun_mno="<<args.lun_mno<<"\n"; os<<"input.lunar_day="<<args.lunar_day<<"\n"; os<<"input.lun_leap="<<(args.leap?"1":"0")<<"\n"; os<<"data.lun_label="<<res.lunar.lun_label<<"\n"; } os<<"input.lunar_day_tz="<<canonical_tz_text(args.lunar_day_tz)<<"\n"; os<<"data.gcst_date="<<ymd_str(res.greg.year,res.greg.month,res.greg.day)<<"\n"; os<<"data.gcst_jd="<<format_num(res.greg.cstday_jd)<<"\n"; if(!res.forward){ os<<"data.gcst_uiso="<<fmt_iso(res.greg.cstday_jd,0,true)<<"\n"; os<<"data.gcst_liso="<<fmt_iso(res.greg.cstday_jd,tz_disp,true)<<"\n"; } }},
		};
		run_fmt(fmts,args.format,"convert");
		return;
	}
	const FmtMap fmts={
		{"jsonl",[&](){ if(args.meta_once){ JsonWriter w(os,false); w.obj_begin(); w.key("meta"); write_meta(w,args.ephem,args.tz,{note,"batch=true"}); w.obj_end(); os<<"\n"; } for(const auto&row : res.rows){ JsonWriter w(os,false); w.obj_begin(); if(!args.meta_once){ write_meta(w,args.ephem,args.tz,{note,"batch=true"}); } w.key("line_no"); w.value(row.line_no); w.key("raw"); w.value(row.raw); if(!row.ok){ w.key("error"); w.obj_begin(); w.key("message"); w.value(row.error); w.obj_end(); }else{ w.key("input"); w.obj_begin(); w.key("direction"); w.value(row.dir); w.key("input_tz"); w.value(row.tz_in); w.key("lunar_day_tz"); w.value(canonical_tz_text(args.lunar_day_tz)); w.key("jd_utc"); w.value(row.jd_utc); w.obj_end(); w.key("data"); w.obj_begin(); w.key("lunar_date"); wr_ljson(w,row.lunar); w.key("gcst_date"); w.value(ymd_str(row.greg.year,row.greg.month,row.greg.day)); w.key("gcst_jd"); w.value(row.greg.cstday_jd); w.obj_end(); } w.obj_end(); os<<"\n"; } }},
		{"json",[&](){ JsonWriter w(os,args.pretty); w.obj_begin(); write_meta(w,args.ephem,args.tz,{note,"batch=true"}); w.key("data"); w.arr_begin(); for(const auto&row : res.rows){ w.obj_begin(); w.key("line_no"); w.value(row.line_no); w.key("raw"); w.value(row.raw); if(!row.ok){ w.key("error"); w.obj_begin(); w.key("message"); w.value(row.error); w.obj_end(); }else{ w.key("input"); w.obj_begin(); w.key("direction"); w.value(row.dir); w.key("input_tz"); w.value(row.tz_in); w.key("lunar_day_tz"); w.value(canonical_tz_text(args.lunar_day_tz)); w.key("jd_utc"); w.value(row.jd_utc); w.obj_end(); w.key("data"); w.obj_begin(); w.key("lunar_date"); wr_ljson(w,row.lunar); w.key("gcst_date"); w.value(ymd_str(row.greg.year,row.greg.month,row.greg.day)); w.key("gcst_jd"); w.value(row.greg.cstday_jd); w.obj_end(); } w.obj_end(); } w.arr_end(); w.obj_end(); os<<"\n"; }},
		{"txt",[&](){ os<<"tool=lunar format=txt type=convert-batch tz_display="<<args.tz<<"\n"; os<<"input.lunar_day_tz="<<canonical_tz_text(args.lunar_day_tz)<<"\n"; os<<"line_no\tstatus\traw\tdirection\tgregorian_cst_date\tlunar_date\tmessage\n"; for(const auto&row : res.rows){ os<<row.line_no<<"\t"; if(row.ok){ os<<"ok\t"<<row.raw<<"\t"<<row.dir<<"\t"<<ymd_str(row.greg.year,row.greg.month,row.greg.day)<<"\t"<<row.lunar.lun_label<<"\t\n"; }else{ os<<"error\t"<<row.raw<<"\t\t\t\t"<<row.error<<"\n"; } } }},
	};
	run_fmt(fmts,args.format,"convert");
}

}

int cmd_at(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_at();
		return 0;
	}
	AtOpt opt=parse_at(args);
	AtRes res=run_at(opt);
	OutTgt out=open_out(opt.run.out);
	write_at(*out.stream,opt,res);
	note_out(opt.run.out,opt.run.quiet);
	return res.err_cnt==0?0:1;
}

int cmd_conv(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_conv();
		return 0;
	}
	ConvOpt opt=parse_conv(args);
	ConvRes res=run_conv(opt);
	OutTgt out=open_out(opt.run.out);
	write_conv(*out.stream,opt,res);
	note_out(opt.run.out,opt.run.quiet);
	return res.err_cnt==0?0:1;
}
