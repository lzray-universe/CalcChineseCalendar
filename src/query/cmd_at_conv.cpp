namespace{

int run_abcli(const AtArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"jsonl","json","txt"},"at");
	if(args.from_stdin&&!args.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	std::vector<BatchLine> lines=read_bat(args.from_stdin,args.input_file);
	if(lines.empty()){
		throw std::invalid_argument("batch input is empty");
	}

	if(args.jobs>1&&!args.quiet){
		std::cerr<<"note: --jobs is accepted; current batch executor runs "
				   "seq_run for det_mode behavior.\n";
	}

	const int tz_disp=parse_tz(args.tz);
	const int lunar_day_tz_off=parse_tz(args.lunar_day_tz);
	EphRead eph(args.ephem);
	QueryCache cache(eph);

	struct Row{
		int line_no=0;
		std::string raw;
		bool ok=false;
		std::string error;
		AtData data;
	};
	std::vector<Row> rows;
	rows.reserve(lines.size());
	int err_cnt=0;
	for(const auto&line : lines){
		Row row;
		row.line_no=line.line_no;
		row.raw=line.raw;
		try{
			double hli_lon=args.calc_eot
							   ?args.eot_lon_deg
							   :static_cast<double>(tz_disp)/60.0*15.0;
			row.data=at_ftxt(eph,line.raw,args.input_tz,tz_disp,args.tz,
							 lunar_day_tz_off,args.events,args.calc_eot,args.eot_lon_deg,
							 hli_lon,&args.hli_rules,&cache);
			row.ok=true;
		}catch(const std::exception&ex){
			row.ok=false;
			row.error=ex.what();
			++err_cnt;
		}
		rows.push_back(std::move(row));
	}

	OutTgt out=open_out(args.out);
	const FmtMap fmt_handlers={
		{"jsonl",[&](){
			 if(args.meta_once){
				 JsonWriter wm(*out.stream,false);
				 wm.obj_begin();
				 wm.key("meta");
				 write_meta(wm,args.ephem,args.tz,
							{"batch=true","schema=lunar.v1"});
				 wm.obj_end();
				 *out.stream<<"\n";
			 }
			 for(const auto&row : rows){
				 JsonWriter w(*out.stream,false);
				 w.obj_begin();
				 if(!args.meta_once){
					 write_meta(w,args.ephem,args.tz,
								{"batch=true","schema=lunar.v1"});
				 }
				 w.key("line_no");
				 w.value(row.line_no);
				 w.key("raw");
				 w.value(row.raw);
				 if(row.ok){
					 w.key("input");
					 wr_aijs(w,row.data);
					 w.key("data");
					 wr_adjs(w,row.data,eph);
				 }else{
					 w.key("error");
					 w.obj_begin();
					 w.key("message");
					 w.value(row.error);
					 w.obj_end();
				 }
				 w.obj_end();
				 *out.stream<<"\n";
			 }
		 }},
		{"json",[&](){
			 JsonWriter w(*out.stream,args.pretty);
			 w.obj_begin();
			 write_meta(w,args.ephem,args.tz,{"batch=true","schema=lunar.v1"});
			 w.key("data");
			 w.arr_begin();
			 for(const auto&row : rows){
				 w.obj_begin();
				 w.key("line_no");
				 w.value(row.line_no);
				 w.key("raw");
				 w.value(row.raw);
				 if(row.ok){
					 w.key("input");
					 wr_aijs(w,row.data);
					 w.key("data");
					 wr_adjs(w,row.data,eph);
				 }else{
					 w.key("error");
					 w.obj_begin();
					 w.key("message");
					 w.value(row.error);
					 w.obj_end();
				 }
				 w.obj_end();
			 }
			 w.arr_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=at-batch tz_display="<<args.tz
			   <<"\n";
			 os<<"line_no\tstatus\traw\till_pct\tphase_name\tlunar_date\t"
			   "y_lun_gz\ty_lchun_gz\ty_rule_gz\tm_gz\td_gz\th_gz\th_true_gz\t"
			   "rule_profile\tyear_boundary\tmonth_boundary\tleap_month_mode\t"
			   "day_boundary\tjianchu\tchong_sha\tyi\tji";
			 if(args.calc_eot){
				 os<<"\teot_minutes";
			 }
			 os<<"\tmessage\n";
			 for(const auto&row : rows){
				 os<<row.line_no<<"\t";
				 if(row.ok){
					 os<<"ok\t"<<row.raw<<"\t"<<format_num(row.data.ill_pct)
					   <<"\t"<<row.data.phase_name<<"\t"
					   <<row.data.lunar_date.lun_label<<"\t"
					   <<row.data.hli.y_lun.text<<"\t"
					   <<row.data.hli.y_lchun.text<<"\t"
					   <<row.data.hli.y_rule.text<<"\t"
					   <<row.data.hli.m_gz.text<<"\t"
					   <<row.data.hli.d_gz.text<<"\t"
					   <<row.data.hli.h_gz.text<<"\t"
					   <<row.data.hli.h_gz_true.text<<"\t"
					   <<row.data.hli.rule_profile<<"\t"
					   <<row.data.hli.year_boundary_text<<"\t"
					   <<row.data.hli.month_boundary_text<<"\t"
					   <<row.data.hli.leap_month_mode_text<<"\t"
					   <<row.data.hli.day_boundary_text<<"\t"
					   <<row.data.hli.jianchu<<"\t"
					   <<row.data.hli.chong_sha<<"\t"
					   <<join_pipe(row.data.hli.yi)<<"\t"
					   <<join_pipe(row.data.hli.ji);
					 if(args.calc_eot){
						 os<<"\t"<<format_num(row.data.eot.eot_minutes);
					 }
					 os<<"\t\n";
				 }else{
					 os<<"error\t"<<row.raw
					   <<"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
					 if(args.calc_eot){
						 os<<"\t";
					 }
					 os<<"\t"<<row.error<<"\n";
				 }
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"at");
	note_out(args.out,args.quiet);
	return (err_cnt==0)?0:1;
}

int run_cbcli(const ConvArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"jsonl","json","txt"},"convert");
	if(args.from_stdin&&!args.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	std::vector<BatchLine> lines=read_bat(args.from_stdin,args.input_file);
	if(lines.empty()){
		throw std::invalid_argument("batch input is empty");
	}
	if(args.jobs>1&&!args.quiet){
		std::cerr<<"note: --jobs is accepted; current batch executor runs "
				   "seq_run for det_mode behavior.\n";
	}

	EphRead eph(args.ephem);
	const int lunar_day_tz_off=parse_tz(args.lunar_day_tz);
	QueryCache cache(eph);

	struct Row{
		int line_no=0;
		std::string raw;
		bool ok=false;
		std::string error;
		std::string direction;
		double jd_utc=std::numeric_limits<double>::quiet_NaN();
		std::string tz_in;
		LunDate lunar_date;
		GregDate greg_date;
	};

	auto parse_lun=[](const std::string&raw,int&y,int&m,int&d,bool&leap){
		std::istringstream iss(raw);
		std::string a,b,c,d4;
		if(!(iss>>a>>b>>c)){
			throw std::invalid_argument(
				"expected: <lunar_year> <month_no> <day> [leap]");
		}
		y=parse_int(a,"lunar_year");
		m=parse_int(b,"lun_mno");
		d=parse_int(c,"lunar_day");
		leap=false;
		if(iss>>d4){
			if(d4=="1"||to_low(d4)=="true"||to_low(d4)=="leap"){
				leap=true;
			}else if(d4=="0"||to_low(d4)=="false"||to_low(d4)=="normal"){
				leap=false;
			}else{
				throw std::invalid_argument(
					"invalid leap flag, expected 0/1/true/false");
			}
		}
	};

	std::vector<Row> rows;
	rows.reserve(lines.size());
	int err_cnt=0;
	for(const auto&line : lines){
		Row row;
		row.line_no=line.line_no;
		row.raw=line.raw;
		try{
			if(!args.from_lunar){
				IsoTime parsed=parse_iso(line.raw,args.input_tz);
				row.direction="greg2lun";
				row.jd_utc=parsed.jd_utc;
				row.tz_in=parsed.has_tz?fmt_tz(parsed.tz_off)
									   :fmt_tz(parse_tz(args.input_tz));
				row.lunar_date=res_lun(eph,parsed.jd_utc,lunar_day_tz_off,&cache);
				row.greg_date.year=row.lunar_date.cst_year;
				row.greg_date.month=row.lunar_date.cst_month;
				row.greg_date.day=row.lunar_date.cst_day;
				row.greg_date.cstday_jd=row.lunar_date.cstday_jd;
			}else{
				int y=0;
				int m=0;
				int d=0;
				bool leap=false;
				parse_lun(line.raw,y,m,d,leap);
				row.direction="lun2greg";
				row.greg_date=res_greg(eph,y,m,d,leap,lunar_day_tz_off,&cache);
				row.jd_utc=row.greg_date.cstday_jd;
				row.tz_in=args.tz;
				row.lunar_date=
					res_lun(eph,row.greg_date.cstday_jd,lunar_day_tz_off,&cache);
			}
			row.ok=true;
		}catch(const std::exception&ex){
			row.ok=false;
			row.error=ex.what();
			++err_cnt;
		}
		rows.push_back(std::move(row));
	}

	OutTgt out=open_out(args.out);
	const std::string note=
		lunar_day_rule_note(args.lunar_day_tz)+"; "+
		lunar::i18n::pick("--tz仅影响显示","--tz affects display only.",
						  "--tz は表示のみ影響します。",
						  "--tz 는 표시만 바꿉니다.");
	const FmtMap fmt_handlers={
		{"jsonl",[&](){
			 if(args.meta_once){
				 JsonWriter wm(*out.stream,false);
				 wm.obj_begin();
				 wm.key("meta");
				 write_meta(wm,args.ephem,args.tz,{note,"batch=true"});
				 wm.obj_end();
				 *out.stream<<"\n";
			 }
			 for(const auto&row : rows){
				 JsonWriter w(*out.stream,false);
				 w.obj_begin();
				 if(!args.meta_once){
					 write_meta(w,args.ephem,args.tz,{note,"batch=true"});
				 }
				 w.key("line_no");
				 w.value(row.line_no);
				 w.key("raw");
				 w.value(row.raw);
				 if(!row.ok){
					 w.key("error");
					 w.obj_begin();
					 w.key("message");
					 w.value(row.error);
					 w.obj_end();
				 }else{
					 w.key("input");
					 w.obj_begin();
					 w.key("direction");
					 w.value(row.direction);
					 w.key("input_tz");
					 w.value(row.tz_in);
					 w.key("lunar_day_tz");
					 w.value(canonical_tz_text(args.lunar_day_tz));
					 w.key("jd_utc");
					 w.value(row.jd_utc);
					 w.obj_end();
					 w.key("data");
					 w.obj_begin();
					 w.key("lunar_date");
					 wr_ljson(w,row.lunar_date);
					 w.key("gcst_date");
					 w.value(ymd_str(row.greg_date.year,row.greg_date.month,
									 row.greg_date.day));
					 w.key("gcst_jd");
					 w.value(row.greg_date.cstday_jd);
					 w.obj_end();
				 }
				 w.obj_end();
				 *out.stream<<"\n";
			 }
		 }},
		{"json",[&](){
			 JsonWriter w(*out.stream,args.pretty);
			 w.obj_begin();
			 write_meta(w,args.ephem,args.tz,{note,"batch=true"});
			 w.key("data");
			 w.arr_begin();
			 for(const auto&row : rows){
				 w.obj_begin();
				 w.key("line_no");
				 w.value(row.line_no);
				 w.key("raw");
				 w.value(row.raw);
				 if(!row.ok){
					 w.key("error");
					 w.obj_begin();
					 w.key("message");
					 w.value(row.error);
					 w.obj_end();
				 }else{
					 w.key("input");
					 w.obj_begin();
					 w.key("direction");
					 w.value(row.direction);
					 w.key("input_tz");
					 w.value(row.tz_in);
					 w.key("lunar_day_tz");
					 w.value(canonical_tz_text(args.lunar_day_tz));
					 w.key("jd_utc");
					 w.value(row.jd_utc);
					 w.obj_end();
					 w.key("data");
					 w.obj_begin();
					 w.key("lunar_date");
					 wr_ljson(w,row.lunar_date);
					 w.key("gcst_date");
					 w.value(ymd_str(row.greg_date.year,row.greg_date.month,
									 row.greg_date.day));
					 w.key("gcst_jd");
					 w.value(row.greg_date.cstday_jd);
					 w.obj_end();
				 }
				 w.obj_end();
			 }
			 w.arr_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=convert-batch tz_display="
			   <<args.tz<<"\n";
			 os<<"input.lunar_day_tz="<<canonical_tz_text(args.lunar_day_tz)
			   <<"\n";
			 os<<"line_no\tstatus\traw\tdirection\tgregorian_cst_date\tlunar_"
				 "date\tmessage\n";
			 for(const auto&row : rows){
				 os<<row.line_no<<"\t";
				 if(row.ok){
					 os<<"ok\t"<<row.raw<<"\t"<<row.direction<<"\t"
					   <<ymd_str(row.greg_date.year,row.greg_date.month,
								 row.greg_date.day)
					   <<"\t"<<row.lunar_date.lun_label<<"\t\n";
				 }else{
					 os<<"error\t"<<row.raw<<"\t\t\t\t"<<row.error<<"\n";
				 }
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"convert");

	note_out(args.out,args.quiet);
	return (err_cnt==0)?0:1;
}

}

int cmd_at(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_at();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"at requires: <bsp> <time> or --time <time>");
	}

	InterCfg cfg=load_def();
	AtArgs a;
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
	a.hli_trad=
		hli_profile_key(static_cast<HliProfileCode>(a.hli_rules.profile_code));

	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		a.time_raw=args[i];
		++i;
	}
	const OptMap handlers={
		{"--time",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 a.time_raw=req_val(src,idx,opt);
		 }},
		{"--stdin",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ a.from_stdin=true; }},
		{"--file",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 a.input_file=req_val(src,idx,opt);
		 }},
		{"--input-tz",[&](const std::vector<std::string>&src,std::size_t&idx,
						  const std::string&opt){
			 a.input_tz=req_val(src,idx,opt);
		 }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ a.tz=req_val(src,idx,opt); }},
		{"--lunar-day-tz",[&](const std::vector<std::string>&src,
							  std::size_t&idx,const std::string&opt){
			 a.lunar_day_tz=req_val(src,idx,opt);
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 a.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ a.out=req_val(src,idx,opt); }},
		{"--jobs",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 a.jobs=parse_int(req_val(src,idx,opt),"--jobs");
			 if(a.jobs<1){
				 throw std::invalid_argument("--jobs must be >= 1");
			 }
		 }},
		{"--meta-once",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 a.meta_once=parse_bool01(req_val(src,idx,opt),"--meta-once");
		 }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 a.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ a.quiet=true; }},
		{"--events",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 a.events=parse_bool01(req_val(src,idx,opt),"--events");
		 }},
		{"--eot-lon",[&](const std::vector<std::string>&src,std::size_t&idx,
						 const std::string&opt){
			 a.eot_lon_deg=parse_double(req_val(src,idx,opt),"--eot-lon");
			 a.calc_eot=true;
		 }},
		{"--trad",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 std::string value=req_val(src,idx,opt);
			 HliProfileCode parsed=HliProfileCode::Folk;
			 if(!parse_hli_profile(value,&parsed)){
				 throw std::invalid_argument(
					 "invalid --trad: "+value+
					 " (expected folk|ziping|purple|xieji)");
			 }
			 a.hli_trad=hli_profile_key(parsed);
			 a.hli_rules=make_hli_rule_set(parsed);
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
			 a.hli_rules.year_boundary=static_cast<int>(parsed);
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
			 a.hli_rules.month_boundary=static_cast<int>(parsed);
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
			 a.hli_rules.leap_month_mode=static_cast<int>(parsed);
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
			 a.hli_rules.day_boundary=static_cast<int>(parsed);
		 }},
	};

	for(;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="-h"||opt=="--help"){
			use_at();
			return 0;
		}
		apply_opt(handlers,args,i,opt,"at");
	}

	if(a.from_stdin&&!a.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	bool batch_mode=a.from_stdin||!a.input_file.empty();
	if(batch_mode&&!a.time_raw.empty()){
		if(!a.quiet){
			std::cerr<<"note: positional <time> is ignored in batch mode\n";
		}
		a.time_raw.clear();
	}

	if(a.time_raw.empty()){
		if(!batch_mode){
			throw std::invalid_argument(
				"at requires a <time> or --time <time>");
		}
	}
	if(!batch_mode&&to_low(a.format)=="jsonl"){
		a.format="json";
		a.pretty=false;
	}
	a.lunar_day_tz=canonical_tz_text(a.lunar_day_tz);
	a.hli_rules=normalize_hli_rule_set(a.hli_rules);
	a.hli_trad=
		hli_profile_key(static_cast<HliProfileCode>(a.hli_rules.profile_code));

	if(batch_mode){
		return run_abcli(a);
	}

	cli_at(a);
	return 0;
}

int cmd_conv(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_conv();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"convert requires: <bsp> <dt_or_tm> or --from-lunar ...");
	}

	InterCfg cfg=load_def();
	ConvArgs c;
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
		c.in_value=args[i];
		c.has_in=true;
		++i;
	}
	const OptMap handlers={
		{"--from-lunar",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&){
			 if(idx+3>=src.size()){
				 throw std::invalid_argument(
					 "--from-lunar requires: <year> <month_no> <day>");
			 }
			 c.from_lunar=true;
			 c.lunar_year=parse_int(src[++idx],"lunar_year");
			 c.lun_mno=parse_int(src[++idx],"lun_mno");
			 c.lunar_day=parse_int(src[++idx],"lunar_day");
		 }},
		{"--stdin",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ c.from_stdin=true; }},
		{"--file",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 c.input_file=req_val(src,idx,opt);
		 }},
		{"--leap",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 c.leap=parse_bool01(req_val(src,idx,opt),"--leap");
		 }},
		{"--input-tz",[&](const std::vector<std::string>&src,std::size_t&idx,
						  const std::string&opt){
			 c.input_tz=req_val(src,idx,opt);
		 }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ c.tz=req_val(src,idx,opt); }},
		{"--lunar-day-tz",[&](const std::vector<std::string>&src,
							  std::size_t&idx,const std::string&opt){
			 c.lunar_day_tz=req_val(src,idx,opt);
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 c.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ c.out=req_val(src,idx,opt); }},
		{"--jobs",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 c.jobs=parse_int(req_val(src,idx,opt),"--jobs");
			 if(c.jobs<1){
				 throw std::invalid_argument("--jobs must be >= 1");
			 }
		 }},
		{"--meta-once",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 c.meta_once=parse_bool01(req_val(src,idx,opt),"--meta-once");
		 }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 c.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ c.quiet=true; }},
	};

	for(;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="-h"||opt=="--help"){
			use_conv();
			return 0;
		}
		apply_opt(handlers,args,i,opt,"convert");
	}

	if(c.from_lunar&&c.has_in){
		throw std::invalid_argument(
			"do not pass positional <dt_or_tm> when using --from-lunar");
	}
	if(c.from_stdin&&!c.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	bool batch_mode=c.from_stdin||!c.input_file.empty();
	if(!c.from_lunar&&!c.has_in&&!batch_mode){
		throw std::invalid_argument(
			"convert requires positional <dt_or_tm> when not using "
			"--from-lunar");
	}
	if(c.from_lunar&&!batch_mode){
		if(c.lun_mno<1||c.lun_mno>12){
			throw std::invalid_argument("lunar month must be 1..12");
		}
	}
	if(!batch_mode&&to_low(c.format)=="jsonl"){
		c.format="json";
		c.pretty=false;
	}
	c.lunar_day_tz=canonical_tz_text(c.lunar_day_tz);

	if(batch_mode){
		return run_cbcli(c);
	}

	cli_conv(c);
	return 0;
}

void use_at(){
	std::cout
		<<"Usage:\n"
		<<"  lunar at <bsp> <time>\n"
		<<"  lunar at <bsp> --time <time>\n"
		<<"  lunar at <bsp> --stdin\n"
		<<"  lunar at <bsp> --file <path>\n"
		<<"    [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]\n"
		<<"    [--lunar-day-tz Z|+08:00|-05:00]\n"
		<<"    [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] "
		  "[--quiet] [--events 0|1] [--eot-lon <deg>] [--trad "
		  "folk|ziping|purple|xieji]\n"
		<<"    [--year-boundary lichun|lunar_new_year|dongzhi] "
		  "[--month-boundary solar_term|lunar_first_day]\n"
		<<"    [--leap-month-mode ignore|inherit_previous|split_midway|"
		  "shift_to_next] [--day-boundary hour23|hour0]\n"
		<<"    [--jobs N] [--meta-once 0|1]\n"
		<<"Time formats:\n"
		<<"  YYYY-MM-DD\n"
		<<"  YYYY-MM-DDTHH:MM\n"
		<<"  YYYY-MM-DDTHH:MM:SS[.sss]\n"
		<<"  optional timezone suffix: Z or +HH:MM/-HH:MM\n"
		<<"Examples:\n"
		<<"  lunar at D:\\de442.bsp 2025-06-01T00:00:00+08:00 --format json\n"
		<<"  lunar at D:\\de442.bsp --time 2025-06-01T00:00 --input-tz +08:00 "
		  "--tz Z --lunar-day-tz +09:00\n"
		<<"  lunar at D:\\de442.bsp 2025-01-31T12:00:00+08:00 --trad ziping\n"
		<<"  lunar at D:\\de442.bsp 2025-06-01T00:00:00+08:00 --eot-lon "
		  "116.391\n"
		<<"  lunar at D:\\de442.bsp --file times.txt --format jsonl "
		  "--meta-once 1\n"
		<<"Notes:\n"
		<<"  --input-tz only parses input without timezone suffix; --tz only "
		  "affects display.\n"
		<<"  --lunar-day-tz controls which civil-day boundary is used for lunar "
		  "date mapping.\n"
		<<"  --eot-lon uses east-positive degrees; output is apparent - mean "
		  "solar time.\n";
}

void use_conv(){
	std::cout<<"Usage:\n"
			 <<"  lunar convert <bsp> <dt_or_tm>\n"
			 <<"    [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]\n"
			 <<"    [--lunar-day-tz Z|+08:00|-05:00]\n"
			 <<"    [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] "
			   "[--quiet]\n"
			 <<"    [--stdin|--file <path>] [--jobs N] [--meta-once 0|1]\n"
			 <<"  lunar convert <bsp> --from-lunar <lunar_year> <month_no> "
			   "<day> [--leap 0|1]\n"
			 <<"    [--tz ...] [--lunar-day-tz ...] [--format json|txt|jsonl] [--out <path>] "
			   "[--pretty 0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar convert D:\\de442.bsp 2026-02-18 --format txt\n"
			 <<"  lunar convert D:\\de442.bsp 2025-06-01T00:00 --input-tz "
			   "+08:00 --lunar-day-tz +09:00 --format json\n"
			 <<"  lunar convert D:\\de442.bsp --file dates.txt --format jsonl\n"
			 <<"Notes:\n"
			 <<"  --lunar-day-tz selects the civil-day boundary used for lunar "
			   "date mapping; --tz only affects display.\n";
}

