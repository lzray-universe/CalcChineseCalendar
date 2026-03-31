namespace{

struct MonthCmd{
	MonthsArgs args;
	bool inc_ecl=false;
	bool help=false;
};

struct CalCmd{
	CalArgs args;
	bool inc_ecl=false;
	bool help=false;
};

struct YearCmd{
	YearArgs args;
	bool help=false;
};

struct EventCmd{
	EventArgs args;
	bool calc_ecl=false;
	bool help=false;
	bool to_ecl=false;
	std::vector<std::string> ecl_args;
};

struct DlCmd{
	DlArgs args;
	bool help=false;
};

MonthCmd parse_month(const std::vector<std::string>&src){
	if(src.size()<2){
		throw std::invalid_argument("months requires: <bsp> <years>");
	}
	MonthCmd cmd;
	cmd.args.ephem=src[0];
	cmd.args.years=src[1];
	lunar::ArgParser parser;
	parser.add_flag("-h",[&](){ cmd.help=true; })
		.add_flag("--help",[&](){ cmd.help=true; })
		.add_value("--mode",[&](const std::string&v){ cmd.args.mode=to_low(v); })
		.add_value("--format",[&](const std::string&v){
			cmd.args.format=to_low(v);
		})
		.add_value("--out",[&](const std::string&v){ cmd.args.out=v; })
		.add_value("--tz",[&](const std::string&v){ cmd.args.tz=v; })
		.add_value("--pretty",[&](const std::string&v){
			cmd.args.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ cmd.args.quiet=true; })
		.add_value("--include-eclipses",[&](const std::string&v){
			cmd.inc_ecl=parse_bool01(v,"--include-eclipses");
		})
		.add_value("--output",[&](const std::string&v){ cmd.args.out_json=v; })
		.add_value("--output-txt",[&](const std::string&v){ cmd.args.out_txt=v; });
	for(std::size_t i=2;i<src.size();++i){
		if(!parser.parse_one(src,i,"months")){
			throw std::invalid_argument("unknown option for months: "+src[i]);
		}
		if(cmd.help){
			return cmd;
		}
	}
	if((!cmd.args.out_json.empty()||!cmd.args.out_txt.empty())&&
	   !cmd.args.out.empty()){
		throw std::invalid_argument(
			"--out cannot be combined with deprecated --output/--output-txt");
	}
	if(cmd.inc_ecl&&cmd.args.out_json.empty()&&cmd.args.out_txt.empty()&&
	   to_low(cmd.args.format)=="csv"){
		throw std::invalid_argument(
			"--include-eclipses requires json or txt format for months");
	}
	return cmd;
}

void run_month(const MonthCmd&cmd){
	cli_month_impl(cmd.args,cmd.inc_ecl);
}

CalCmd parse_cal(const std::vector<std::string>&src){
	if(src.empty()){
		throw std::invalid_argument("calendar requires: <bsp> [<years>]");
	}
	CalCmd cmd;
	cmd.args.ephem=src[0];
	std::size_t i=1;
	if(i<src.size()&&!is_opt(src[i])){
		cmd.args.years_arg=src[i];
		cmd.args.has_years=true;
		++i;
	}
	lunar::ArgParser parser;
	parser.add_flag("-h",[&](){ cmd.help=true; })
		.add_flag("--help",[&](){ cmd.help=true; })
		.add_value("--format",[&](const std::string&v){
			cmd.args.format=to_low(v);
		})
		.add_value("--out",[&](const std::string&v){ cmd.args.out=v; })
		.add_value("--tz",[&](const std::string&v){ cmd.args.tz=v; })
		.add_value("--include-months",[&](const std::string&v){
			cmd.args.inc_month=parse_bool01(v,"--include-months");
		})
		.add_value("--include-eclipses",[&](const std::string&v){
			cmd.inc_ecl=parse_bool01(v,"--include-eclipses");
		})
		.add_value("--pretty",[&](const std::string&v){
			cmd.args.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ cmd.args.quiet=true; });
	for(;i<src.size();++i){
		if(!parser.parse_one(src,i,"calendar")){
			throw std::invalid_argument("unknown option for calendar: "+src[i]);
		}
		if(cmd.help){
			return cmd;
		}
	}
	return cmd;
}

void run_cal(const CalCmd&cmd){
	cli_cal_impl(cmd.args,cmd.inc_ecl);
}

YearCmd parse_year_cmd(const std::vector<std::string>&src){
	if(src.size()<2){
		throw std::invalid_argument("year requires: <bsp> <year>");
	}
	YearCmd cmd;
	cmd.args.ephem=src[0];
	cmd.args.year=parse_int(src[1],"year");
	lunar::ArgParser parser;
	parser.add_flag("-h",[&](){ cmd.help=true; })
		.add_flag("--help",[&](){ cmd.help=true; })
		.add_value("--mode",[&](const std::string&v){ cmd.args.mode=to_low(v); })
		.add_value("--format",[&](const std::string&v){
			cmd.args.format=to_low(v);
		})
		.add_value("--out",[&](const std::string&v){ cmd.args.out=v; })
		.add_value("--tz",[&](const std::string&v){ cmd.args.tz=v; })
		.add_value("--pretty",[&](const std::string&v){
			cmd.args.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ cmd.args.quiet=true; });
	for(std::size_t i=2;i<src.size();++i){
		if(!parser.parse_one(src,i,"year")){
			throw std::invalid_argument("unknown option for year: "+src[i]);
		}
		if(cmd.help){
			return cmd;
		}
	}
	return cmd;
}

void run_year_cmd(const YearCmd&cmd){
	cli_year(cmd.args);
}

EventCmd parse_event(const std::vector<std::string>&src){
	if(src.size()<3){
		throw std::invalid_argument(
			"event requires: <bsp> <solar-term|lunar-phase> ...");
	}
	EventCmd cmd;
	if(src.size()>=2){
		std::string cat=to_low(src[1]);
		if(cat=="lunar-eclipse"||cat=="lunar_eclipse"||
		   cat=="solar-eclipse"||cat=="solar_eclipse"){
			bool solar=(cat=="solar-eclipse"||cat=="solar_eclipse");
			cmd.to_ecl=true;
			cmd.ecl_args.push_back(src[0]);
			cmd.ecl_args.push_back("--kind");
			cmd.ecl_args.push_back(solar?"solar":"lunar");
			for(std::size_t i=2;i<src.size();++i){
				if(src[i]=="--eclipse"){
					bool on=parse_bool01(req_val(src,i,"--eclipse"),"--eclipse");
					if(!on){
						throw std::invalid_argument(
							(solar?"solar-eclipse":"lunar-eclipse")+
							std::string(" category does not support --eclipse 0"));
					}
					continue;
				}
				cmd.ecl_args.push_back(src[i]);
			}
			return cmd;
		}
	}

	cmd.args.ephem=src[0];
	cmd.args.category=to_low(src[1]);
	cmd.args.code=src[2];
	std::size_t i=3;
	if(cmd.args.category=="solar-term"){
		if(i>=src.size()){
			throw std::invalid_argument("solar-term requires: <code> <year>");
		}
		cmd.args.year=parse_int(src[i],"year");
		cmd.args.has_year=true;
		++i;
	}else if(cmd.args.category!="lunar-phase"){
		throw std::invalid_argument(
			"event category must be solar-term or lunar-phase");
	}
	lunar::ArgParser parser;
	parser.add_flag("-h",[&](){ cmd.help=true; })
		.add_flag("--help",[&](){ cmd.help=true; })
		.add_value("--near",[&](const std::string&v){ cmd.args.near_date=v; })
		.add_value("--format",[&](const std::string&v){
			cmd.args.format=to_low(v);
		})
		.add_value("--out",[&](const std::string&v){ cmd.args.out=v; })
		.add_value("--tz",[&](const std::string&v){ cmd.args.tz=v; })
		.add_value("--pretty",[&](const std::string&v){
			cmd.args.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ cmd.args.quiet=true; })
		.add_value("--eclipse",[&](const std::string&v){
			cmd.calc_ecl=parse_bool01(v,"--eclipse");
		});
	for(;i<src.size();++i){
		if(!parser.parse_one(src,i,"event")){
			throw std::invalid_argument("unknown option for event: "+src[i]);
		}
		if(cmd.help){
			return cmd;
		}
	}
	if(cmd.args.category=="lunar-phase"&&cmd.args.near_date.empty()){
		throw std::invalid_argument("lunar-phase requires --near YYYY-MM-DD");
	}
	return cmd;
}

int run_event_cmd(const EventCmd&cmd){
	if(cmd.to_ecl){
		return cmd_eclipse(cmd.ecl_args);
	}
	cli_event_impl(cmd.args,cmd.calc_ecl);
	return 0;
}

DlCmd parse_dl(const std::vector<std::string>&src){
	if(src.empty()){
		throw std::invalid_argument("download action must be list or get");
	}
	DlCmd cmd;
	cmd.args.action=to_low(src[0]);
	std::size_t i=1;
	if(cmd.args.action=="get"){
		if(i>=src.size()){
			throw std::invalid_argument("download get requires <id>");
		}
		cmd.args.id=src[i];
		++i;
	}else if(cmd.args.action!="list"){
		throw std::invalid_argument("download action must be list or get");
	}
	lunar::ArgParser parser;
	parser.add_flag("-h",[&](){ cmd.help=true; })
		.add_flag("--help",[&](){ cmd.help=true; })
		.add_value("--dir",[&](const std::string&v){ cmd.args.dir=v; })
		.add_flag("--quiet",[&](){ cmd.args.quiet=true; });
	for(;i<src.size();++i){
		if(!parser.parse_one(src,i,"download")){
			throw std::invalid_argument("unknown option for download: "+src[i]);
		}
		if(cmd.help){
			return cmd;
		}
	}
	return cmd;
}

void run_dl(const DlCmd&cmd){
	cli_dl(cmd.args);
}

}

int cmd_month(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_month();
		return 0;
	}
	MonthCmd cmd=parse_month(args);
	if(cmd.help){
		use_month();
		return 0;
	}
	run_month(cmd);
	return 0;
}

int cmd_cal(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_cal();
		return 0;
	}
	CalCmd cmd=parse_cal(args);
	if(cmd.help){
		use_cal();
		return 0;
	}
	run_cal(cmd);
	return 0;
}

int cmd_year(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_year();
		return 0;
	}
	YearCmd cmd=parse_year_cmd(args);
	if(cmd.help){
		use_year();
		return 0;
	}
	run_year_cmd(cmd);
	return 0;
}

int cmd_event(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_event();
		return 0;
	}
	EventCmd cmd=parse_event(args);
	if(cmd.help){
		use_event();
		return 0;
	}
	return run_event_cmd(cmd);
}

int cmd_dl(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_dl();
		return 0;
	}
	DlCmd cmd=parse_dl(args);
	if(cmd.help){
		use_dl();
		return 0;
	}
	run_dl(cmd);
	return 0;
}
