void cli_month_impl(const MonthsArgs&args,bool inc_eclipse){
	std::vector<int> years=parse_year(args.years);
	const std::string mode=to_low(args.mode);
	chk_mode(mode);

	int tz_off=parse_tz(args.tz);

	EphRead eph(args.ephem);
	LunCal6 calc(eph);
	SolLunCal solver(eph);

	std::vector<MonYrData> data;
	data.reserve(years.size());
	for(int y : years){
		if(!args.quiet){
			std::cerr<<"computing months for year "<<y<<" ..."<<std::endl;
		}
		std::vector<LunarMonth> months=
			(mode=="lunar")?enum_lyr(calc,y):enum_gyr(calc,y);
		MonYrData row;
		row.year=y;
		row.mode=mode;
		row.months=bld_mrec(months,tz_off);
		row.inc_eclipse=inc_eclipse;
		if(inc_eclipse){
			YearResult yr=solver.compute_year(y,args.quiet?nullptr:&std::cerr);
			row.eclipses=bld_eclipses(eph,yr);
		}
		data.push_back(std::move(row));
	}

	if(!args.out_json.empty()||!args.out_txt.empty()){
		if(!args.out_json.empty()){
			OutTgt out=open_out(args.out_json);
			wr_mjs(*out.stream,data,args.ephem,args.tz,true);
			note_out(args.out_json,args.quiet);
		}
		if(!args.out_txt.empty()){
			OutTgt out=open_out(args.out_txt);
			wr_mtxt(*out.stream,data,args.tz);
			note_out(args.out_txt,args.quiet);
		}
		return;
	}

	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","csv"},"months");
	run_mout(args,data,format,args.out);
}

void cli_month(const MonthsArgs&args){ cli_month_impl(args,false); }

void cli_cal_impl(const CalArgs&args,bool inc_eclipse){
	int tz_off=parse_tz(args.tz);
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","ics"},"calendar");

	std::vector<int> years;
	if(args.has_years){
		years=parse_year(args.years_arg);
	}else{
		years.push_back(2025);
	}

	EphRead eph(args.ephem);
	SolLunCal solver(eph);
	LunCal6 calc(eph);

	std::vector<CalYrData> out_data;
	out_data.reserve(years.size());
	for(int y : years){
		YearResult yr=solver.compute_year(y,args.quiet?nullptr:&std::cerr);
		CalYrData item;
		item.year=y;
		item.mode="lunar";
		item.sol_terms=bld_stev(yr,tz_off);
		item.lun_phase=bld_lpev(yr,tz_off);
		item.inc_month=args.inc_month;
		item.inc_eclipse=inc_eclipse;
		if(args.inc_month){
			std::vector<LunarMonth> months=enum_lyr(calc,y);
			item.months=bld_mrec(months,tz_off);
		}
		if(inc_eclipse){
			item.eclipses=bld_eclipses(eph,yr);
		}
		out_data.push_back(std::move(item));
	}

	OutTgt out=open_out(args.out);
	const FmtMap handlers={
		{"json",[&](){
			 wr_caljs(*out.stream,out_data,args.ephem,args.tz,args.pretty,eph);
		 }},
		{"ics",[&](){
			 std::vector<EventRec> merged;
			 for(const auto&item : out_data){
				 merged.insert(merged.end(),item.sol_terms.begin(),
							   item.sol_terms.end());
				 merged.insert(merged.end(),item.lun_phase.begin(),
							   item.lun_phase.end());
			 }
			 std::sort(merged.begin(),merged.end(),[](const EventRec&a,
													 const EventRec&b){
				 return a.jd_utc<b.jd_utc;
			 });
			 std::ostringstream name;
			 name<<"lunar-calendar";
			 if(!years.empty()){
				 name<<"-"<<years.front();
				 if(years.size()>1){
					 name<<"-to-"<<years.back();
				 }
			 }
			 wr_eics(*out.stream,args.ephem,name.str(),merged);
		 }},
		{"txt",[&](){ wr_caltx(*out.stream,out_data,args.tz); }},
	};
	run_fmt(handlers,format,"calendar");
	note_out(args.out,args.quiet);
}

void cli_cal(const CalArgs&args){ cli_cal_impl(args,false); }

void cli_year(const YearArgs&args){
	int tz_off=parse_tz(args.tz);
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","ics"},"year");
	const std::string mode=to_low(args.mode);
	chk_mode(mode);

	EphRead eph(args.ephem);
	SolLunCal solver(eph);
	LunCal6 calc(eph);

	YearResult yr=solver.compute_year(args.year,args.quiet?nullptr:&std::cerr);
	CalYrData data;
	data.year=args.year;
	data.mode=mode;
	data.sol_terms=bld_stev(yr,tz_off);
	data.lun_phase=bld_lpev(yr,tz_off);
	data.inc_month=true;
	std::vector<LunarMonth> months=
		(mode=="lunar")?enum_lyr(calc,args.year):enum_gyr(calc,args.year);
	data.months=bld_mrec(months,tz_off);

	OutTgt out=open_out(args.out);
	const FmtMap handlers={
		{"json",[&](){
			 wr_yjs(*out.stream,data,args.ephem,args.tz,args.pretty,eph);
		 }},
		{"ics",[&](){
			 std::vector<EventRec> merged=data.sol_terms;
			 merged.insert(merged.end(),data.lun_phase.begin(),
						   data.lun_phase.end());
			 std::sort(merged.begin(),merged.end(),[](const EventRec&a,
													 const EventRec&b){
				 return a.jd_utc<b.jd_utc;
			 });
			 wr_eics(*out.stream,args.ephem,
					 "lunar-year-"+std::to_string(args.year),merged);
		 }},
		{"txt",[&](){ wr_ytxt(*out.stream,data,args.tz); }},
	};
	run_fmt(handlers,format,"year");
	note_out(args.out,args.quiet);
}

void cli_event_impl(const EventArgs&args,bool calc_eclipse){
	int tz_off=parse_tz(args.tz);
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","ics"},"event");

	EphRead eph(args.ephem);
	SolLunCal solver(eph);

	const std::string category=to_low(args.category);
	EventRec ev;
	if(category=="solar-term"){
		if(!args.has_year){
			throw std::invalid_argument("solar-term requires a <year>");
		}
		const auto&defs=SolLunCal::st_defs();
		auto it=defs.find(args.code);
		if(it==defs.end()){
			throw std::invalid_argument("unknown solar-term code: "+args.code);
		}
		LocalDT dt=solver.find_st(args.code,args.year);
		ev=mk_erec("solar_term",args.code,it->second.name,args.year,
				   std::numeric_limits<double>::quiet_NaN(),dt.toUtcJD(),
				   tz_off);
	}else if(category=="lunar-phase"){
		const auto&defs=SolLunCal::lp_defs();
		auto it=defs.find(args.code);
		if(it==defs.end()){
			throw std::invalid_argument("unknown lunar-phase code: "+args.code);
		}
		if(args.near_date.empty()){
			throw std::invalid_argument(
				"lunar-phase requires --near YYYY-MM-DD");
		}
		int y=0;
		int m=0;
		int d=0;
		std::tie(y,m,d)=parse_ld(args.near_date);
		LocalDT loc_mid=SolLunCal::mk_local(y,m,d,0,0,0.0);
		double jd_utc=SolLunCal::loc2utc(loc_mid);
		double jd_guess=TimeScale::utc_to_tdb(jd_utc);
		LocalDT dt=solver.find_lp(args.code,jd_guess);
		ev=mk_erec("lunar_phase",args.code,it->second.name,dt.year,
				   std::numeric_limits<double>::quiet_NaN(),dt.toUtcJD(),
				   tz_off);
	}else{
		throw std::invalid_argument(
			"event category must be solar-term or lunar-phase");
	}
	LunarEclipse eclipse_data;
	const LunarEclipse*ecl_ptr=nullptr;
	if(calc_eclipse&&is_full_moon_ev(ev)){
		double jd_tdb=TimeScale::utc_to_tdb(ev.jd_utc);
		calc_lunar_eclipse(eph,jd_tdb,&eclipse_data);
		ecl_ptr=&eclipse_data;
	}

	OutTgt out=open_out(args.out);
	const FmtMap handlers={
		{"json",[&](){
			 wr_ejdoc(*out.stream,ev,args.ephem,args.tz,args.pretty,eph,ecl_ptr,
					  calc_eclipse);
		 }},
		{"ics",[&](){
			 std::vector<EventRec> one{ev};
			 wr_eics(*out.stream,args.ephem,"lunar-event",one);
		 }},
		{"txt",[&](){ wr_setxt(*out.stream,ev,args.tz,ecl_ptr,calc_eclipse); }},
	};
	run_fmt(handlers,format,"event");
	note_out(args.out,args.quiet);
}

void cli_event(const EventArgs&args){ cli_event_impl(args,false); }

void cli_dl(const DlArgs&args){
	const std::string action=to_low(args.action);
	if(action=="list"){
		auto opts=bsp_opts();
		std::cout<<"id\tsize\trange\turl\n";
		for(const auto&opt : opts){
			std::cout<<opt.id<<"\t"<<opt.size<<"\t"<<opt.range<<"\t"<<opt.url
					 <<"\n";
		}
		return;
	}

	if(action!="get"){
		throw std::invalid_argument("download action must be list or get");
	}

	auto opts=bsp_opts();
	const BspOption*found=nullptr;
	std::string id_l=to_low(args.id);
	for(const auto&opt : opts){
		if(to_low(opt.id)==id_l){
			found=&opt;
			break;
		}
	}
	if(!found){
		throw std::invalid_argument("unknown bsp id: "+args.id);
	}

	std::filesystem::path dir=args.dir.empty()?std::filesystem::current_path()
											  :std::filesystem::path(args.dir);
	std::error_code ec;
	std::filesystem::create_directories(dir,ec);
	if(ec){
		throw std::runtime_error("failed to create output directory: "+
								 dir.string());
	}
	std::filesystem::path filename=std::filesystem::path(found->url).filename();
	std::filesystem::path target=dir/filename;
	if(!dl_file(found->url,target.string())){
		throw std::runtime_error("download failed");
	}
	std::cout<<target.string()<<std::endl;
}

