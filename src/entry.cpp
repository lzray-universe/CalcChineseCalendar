#include "lunar/entry.hpp"

#include<iostream>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/cli.hpp"
#include "lunar/interact.hpp"

int run_cli_args(const std::vector<std::string>&args){
	if(args.empty()){
		int_mode();
		return 0;
	}

	const std::string&first=args[0];

	if(first=="__root_batch"){
		if(args.size()!=4){
			return 2;
		}
		return run_rootw(args[1],args[2],args[3]);
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
		return cmd_month(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="calendar"){
		return cmd_cal(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="year"){
		return cmd_year(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="event"){
		return cmd_event(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="download"){
		return cmd_dl(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="at"){
		return cmd_at(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="convert"){
		return cmd_conv(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="day"){
		return cmd_day(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="monthview"){
		return cmd_mview(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="next"){
		return cmd_next(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="range"){
		return cmd_range(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="search"){
		return cmd_search(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="festival"){
		return cmd_fest(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="almanac"){
		return cmd_alm(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="info"){
		return cmd_info(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="selftest"){
		return cmd_test(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="config"){
		return cmd_cfg(std::vector<std::string>(args.begin()+1,args.end()));
	}
	if(first=="completion"){
		return cmd_comp(std::vector<std::string>(args.begin()+1,args.end()));
	}

	return cmd_month(args);
}
