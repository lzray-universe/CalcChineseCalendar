int cmd_month(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_month();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("months requires: <bsp> <years>");
	}

	MonthsArgs margs;
	margs.ephem=args[0];
	margs.years=args[1];
	bool inc_eclipse=false;
	const OptMap handlers={
		{"--mode",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 margs.mode=to_low(req_val(src,idx,opt));
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 margs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ margs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ margs.tz=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 margs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ margs.quiet=true; }},
		{"--include-eclipses",[&](const std::vector<std::string>&src,
								  std::size_t&idx,const std::string&opt){
			 inc_eclipse=parse_bool01(req_val(src,idx,opt),opt);
		 }},
		{"--output",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 margs.out_json=req_val(src,idx,opt);
		 }},
		{"--output-txt",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 margs.out_txt=req_val(src,idx,opt);
		 }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_month();
			return 0;
		}
		apply_opt(handlers,args,i,a,"months");
	}

	if((!margs.out_json.empty()||!margs.out_txt.empty())&&!margs.out.empty()){
		throw std::invalid_argument(
			"--out cannot be combined with deprecated --output/--output-txt");
	}
	if(inc_eclipse&&margs.out_json.empty()&&margs.out_txt.empty()&&
	   to_low(margs.format)=="csv"){
		throw std::invalid_argument(
			"--include-eclipses requires json or txt format for months");
	}

	cli_month_impl(margs,inc_eclipse);
	return 0;
}

int cmd_cal(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_cal();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument("calendar requires: <bsp> [<years>]");
	}

	CalArgs cargs;
	cargs.ephem=args[0];
	bool inc_eclipse=false;
	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		cargs.years_arg=args[i];
		cargs.has_years=true;
		++i;
	}
	const OptMap handlers={
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 cargs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ cargs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ cargs.tz=req_val(src,idx,opt); }},
		{"--include-months",[&](const std::vector<std::string>&src,
								std::size_t&idx,const std::string&opt){
			 cargs.inc_month=parse_bool01(req_val(src,idx,opt),"--include-months");
		 }},
		{"--include-eclipses",[&](const std::vector<std::string>&src,
								   std::size_t&idx,const std::string&opt){
			 inc_eclipse=parse_bool01(req_val(src,idx,opt),opt);
		 }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 cargs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ cargs.quiet=true; }},
	};
	for(;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_cal();
			return 0;
		}
		apply_opt(handlers,args,i,a,"calendar");
	}

	cli_cal_impl(cargs,inc_eclipse);
	return 0;
}

int cmd_year(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_year();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("year requires: <bsp> <year>");
	}

	YearArgs yargs;
	yargs.ephem=args[0];
	yargs.year=parse_int(args[1],"year");
	const OptMap handlers={
		{"--mode",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 yargs.mode=to_low(req_val(src,idx,opt));
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 yargs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ yargs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ yargs.tz=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 yargs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ yargs.quiet=true; }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_year();
			return 0;
		}
		apply_opt(handlers,args,i,a,"year");
	}

	cli_year(yargs);
	return 0;
}

int cmd_event(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_event();
		return 0;
	}
	if(args.size()>=2){
		std::string cat=to_low(args[1]);
		if(cat=="lunar-eclipse"||cat=="lunar_eclipse"||
		   cat=="solar-eclipse"||cat=="solar_eclipse"){
			bool solar=(cat=="solar-eclipse"||cat=="solar_eclipse");
			std::vector<std::string> eargs;
			eargs.push_back(args[0]);
			eargs.push_back("--kind");
			eargs.push_back(solar?"solar":"lunar");
			for(std::size_t i=2;i<args.size();++i){
				if(args[i]=="--eclipse"){
					bool on=parse_bool01(req_val(args,i,"--eclipse"),"--eclipse");
					if(!on){
						throw std::invalid_argument(
							(solar?"solar-eclipse":"lunar-eclipse")+
							std::string(" category does not support --eclipse 0"));
					}
					continue;
				}
				eargs.push_back(args[i]);
			}
			return cmd_eclipse(eargs);
		}
	}
	if(args.size()<3){
		throw std::invalid_argument(
			"event requires: <bsp> <solar-term|lunar-phase> ...");
	}

	EventArgs eargs;
	eargs.ephem=args[0];
	eargs.category=to_low(args[1]);
	eargs.code=args[2];
	bool calc_eclipse=false;

	std::size_t i=3;
	if(eargs.category=="solar-term"){
		if(i>=args.size()){
			throw std::invalid_argument("solar-term requires: <code> <year>");
		}
		eargs.year=parse_int(args[i],"year");
		eargs.has_year=true;
		++i;
	}else if(eargs.category!="lunar-phase"){
		throw std::invalid_argument(
			"event category must be solar-term or lunar-phase");
	}
	const OptMap handlers={
		{"--near",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 eargs.near_date=req_val(src,idx,opt);
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 eargs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ eargs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ eargs.tz=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 eargs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ eargs.quiet=true; }},
		{"--eclipse",[&](const std::vector<std::string>&src,std::size_t&idx,
						 const std::string&opt){
			 calc_eclipse=parse_bool01(req_val(src,idx,opt),opt);
		 }},
	};

	for(;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_event();
			return 0;
		}
		apply_opt(handlers,args,i,a,"event");
	}

	if(eargs.category=="lunar-phase"&&eargs.near_date.empty()){
		throw std::invalid_argument("lunar-phase requires --near YYYY-MM-DD");
	}

	cli_event_impl(eargs,calc_eclipse);
	return 0;
}

int cmd_dl(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_dl();
		return 0;
	}

	DlArgs dargs;
	dargs.action=to_low(args[0]);

	std::size_t i=1;
	if(dargs.action=="get"){
		if(i>=args.size()){
			throw std::invalid_argument("download get requires <id>");
		}
		dargs.id=args[i];
		++i;
	}else if(dargs.action!="list"){
		throw std::invalid_argument("download action must be list or get");
	}
	const OptMap handlers={
		{"--dir",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ dargs.dir=req_val(src,idx,opt); }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ dargs.quiet=true; }},
	};

	for(;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_dl();
			return 0;
		}
		apply_opt(handlers,args,i,a,"download");
	}

	cli_dl(dargs);
	return 0;
}

