#include "lunar/entry.hpp"

#include<iostream>
#include<stdexcept>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/cli/main.hpp"
#include "lunar/global_context.hpp"
#include "lunar/i18n.hpp"
#include "lunar/interact.hpp"
#include "lunar/lunar_eclipse.hpp"

namespace{

std::string req_global_opt_value(const std::vector<std::string>&args,
								 std::size_t&i,const std::string&opt){
	if(i+1>=args.size()){
		throw std::invalid_argument("missing value for option: "+opt);
	}
	++i;
	if(args[i]=="-h"||args[i]=="--help"||args[i].rfind("--",0)==0){
		throw std::invalid_argument("missing value for option: "+opt);
	}
	return args[i];
}

std::vector<std::string> parse_global_opts(const std::vector<std::string>&args,
										   const std::string&default_lang){
	LunarEclipseCalcMethod method=LunarEclipseCalcMethod::Modern;
	lunar::i18n::Lang lang=lunar::i18n::Lang::Zh;
	(void)lunar::i18n::try_parse_lang(default_lang,&lang);
	std::vector<std::string> out;
	out.reserve(args.size());

	const std::string prefix="--eclipse-method=";
	const std::string lang_prefix="--lang=";
	for(std::size_t i=0;i<args.size();++i){
		const std::string&arg=args[i];
		if(arg=="--eclipse-method"){
			LunarEclipseCalcMethod parsed=LunarEclipseCalcMethod::Modern;
			const std::string&value=req_global_opt_value(args,i,arg);
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
			if(value.empty()){
				throw std::invalid_argument(
					"missing value for option: --eclipse-method");
			}
			if(!parse_lunar_eclipse_calc_method(value,&parsed)){
				throw std::invalid_argument(
					"invalid --eclipse-method: "+value+
					" (expected modern|legacy)");
			}
			method=parsed;
			continue;
		}
		if(arg=="--lang"){
			const std::string&value=req_global_opt_value(args,i,arg);
			if(!lunar::i18n::try_parse_lang(value,&lang)){
				throw std::invalid_argument(
					"invalid --lang: "+value+" (expected zh|zht|en|ja|ko)");
			}
			continue;
		}
		if(arg.rfind(lang_prefix,0)==0){
			std::string value=arg.substr(lang_prefix.size());
			if(value.empty()){
				throw std::invalid_argument("missing value for option: --lang");
			}
			if(!lunar::i18n::try_parse_lang(value,&lang)){
				throw std::invalid_argument(
					"invalid --lang: "+value+" (expected zh|zht|en|ja|ko)");
			}
			continue;
		}
		out.push_back(arg);
	}

	set_lunar_eclipse_calc_method(method);
	lunar::i18n::set_lang(lang);
	return out;
}

}

int run_cli_args(const std::vector<std::string>&args){
	InterCfg cfg;
	load_cfg(cfg);
	std::string default_lang=cfg.default_lang.empty()?"zh":cfg.default_lang;
	std::vector<std::string> parsed=parse_global_opts(args,default_lang);
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

	GlobalContext gctx=load_global_ctx();
	auto tail_for=[&](const std::string&cmd){
		return prep_cmd_args(
			cmd,std::vector<std::string>(parsed.begin()+1,parsed.end()),gctx);
	};

	if(first=="months"){
		return cmd_month(tail_for(first));
	}
	if(first=="calendar"){
		return cmd_cal(tail_for(first));
	}
	if(first=="year"){
		return cmd_year(tail_for(first));
	}
	if(first=="event"){
		return cmd_event(tail_for(first));
	}
	if(first=="download"){
		return cmd_dl(tail_for(first));
	}
	if(first=="at"){
		return cmd_at(tail_for(first));
	}
	if(first=="convert"){
		return cmd_conv(tail_for(first));
	}
	if(first=="zodiac"){
		return cmd_zodiac(tail_for(first));
	}
	if(first=="sky"){
		return cmd_sky(tail_for(first));
	}
	if(first=="day"){
		return cmd_day(tail_for(first));
	}
	if(first=="monthview"){
		return cmd_mview(tail_for(first));
	}
	if(first=="export"){
		return cmd_export(tail_for(first));
	}
	if(first=="next"){
		return cmd_next(tail_for(first));
	}
	if(first=="range"){
		return cmd_range(tail_for(first));
	}
	if(first=="search"){
		return cmd_search(tail_for(first));
	}
	if(first=="eclipse"){
		return cmd_eclipse(tail_for(first));
	}
	if(first=="eclipse-magnitude"){
		return cmd_eclipse_magnitude(
			std::vector<std::string>(parsed.begin()+1,parsed.end()));
	}
	if(first=="festival"){
		return cmd_fest(tail_for(first));
	}
	if(first=="almanac"){
		return cmd_alm(tail_for(first));
	}
	if(first=="info"){
		return cmd_info(tail_for(first));
	}
	if(first=="config"){
		return cmd_cfg(tail_for(first));
	}
	if(first=="completion"){
		return cmd_comp(tail_for(first));
	}

	return cmd_month(prep_cmd_args("months",parsed,gctx));
}
