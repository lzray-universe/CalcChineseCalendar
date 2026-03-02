#include "lunar/entry.hpp"

#include<iostream>
#include<stdexcept>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/cli.hpp"
#include "lunar/interact.hpp"
#include "lunar/lunar_eclipse.hpp"

namespace{

std::vector<std::string> parse_global_eclipse_method_args(
	const std::vector<std::string>&args){
	LunarEclipseCalcMethod method=LunarEclipseCalcMethod::Modern;
	std::vector<std::string> out;
	out.reserve(args.size());

	const std::string prefix="--eclipse-method=";
	for(std::size_t i=0;i<args.size();++i){
		const std::string&arg=args[i];
		if(arg=="--eclipse-method"){
			if(i+1>=args.size()){
				throw std::invalid_argument(
					"--eclipse-method requires: modern|legacy");
			}
			LunarEclipseCalcMethod parsed=LunarEclipseCalcMethod::Modern;
			const std::string&value=args[++i];
			if(!parse_lunar_eclipse_calc_method(value,&parsed)){
				throw std::invalid_argument(
					"invalid --eclipse-method: "+value+
					" (expected modern|legacy)");
			}
			method=parsed;
			continue;
		}
		if(arg.rfind(prefix,0)==0){
			LunarEclipseCalcMethod parsed=LunarEclipseCalcMethod::Modern;
			std::string value=arg.substr(prefix.size());
			if(!parse_lunar_eclipse_calc_method(value,&parsed)){
				throw std::invalid_argument(
					"invalid --eclipse-method: "+value+
					" (expected modern|legacy)");
			}
			method=parsed;
			continue;
		}
		out.push_back(arg);
	}

	set_lunar_eclipse_calc_method(method);
	return out;
}

}

int run_cli_args(const std::vector<std::string>&args){
	std::vector<std::string> parsed=parse_global_eclipse_method_args(args);
	if(parsed.empty()){
		int_mode();
		return 0;
	}

	const std::string&first=parsed[0];

	if(first=="__root_batch"){
		if(parsed.size()!=4){
			return 2;
		}
		return run_rootw(parsed[1],parsed[2],parsed[3]);
	}

	if(first=="-h"||first=="--help"){
		use_main();
		return 0;
	}
	if(first=="--version"){
		std::cout<<tool_ver()<<std::endl;
		return 0;
	}

	if(first=="months"){
		return cmd_month(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="calendar"){
		return cmd_cal(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="year"){
		return cmd_year(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="event"){
		return cmd_event(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="download"){
		return cmd_dl(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="at"){
		return cmd_at(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="convert"){
		return cmd_conv(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="day"){
		return cmd_day(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="monthview"){
		return cmd_mview(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="next"){
		return cmd_next(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="range"){
		return cmd_range(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="search"){
		return cmd_search(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="eclipse"){
		return cmd_eclipse(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="festival"){
		return cmd_fest(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="almanac"){
		return cmd_alm(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="info"){
		return cmd_info(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="selftest"){
		return cmd_test(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="config"){
		return cmd_cfg(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="completion"){
		return cmd_comp(std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}

	return cmd_month(parsed);
}

