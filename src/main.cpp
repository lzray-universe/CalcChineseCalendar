#include "lunar/calendar.hpp"
#include "lunar/interact.hpp"

#include<exception>
#include<iostream>
#include<string>
#include<vector>

#ifdef _WIN32
#include<windows.h>
#endif

namespace{

std::vector<std::string> col_args(int argc,char**argv){
	std::vector<std::string> out;
	out.reserve(argc>1?static_cast<std::size_t>(argc-1):0);
	for(int i=1;i<argc;++i){
		out.emplace_back(argv[i]);
	}
	return out;
}

}

int main(int argc,char**argv){
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	try{
		if(argc<=1){
			int_mode();
			return 0;
		}

		std::vector<std::string> args=col_args(argc,argv);
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
			return cmd_month(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="calendar"){
			return cmd_cal(std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="year"){
			return cmd_year(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="event"){
			return cmd_event(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="download"){
			return cmd_dl(std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="at"){
			return cmd_at(std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="convert"){
			return cmd_conv(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="day"){
			return cmd_day(std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="monthview"){
			return cmd_mview(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="next"){
			return cmd_next(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="range"){
			return cmd_range(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="search"){
			return cmd_search(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="festival"){
			return cmd_fest(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="almanac"){
			return cmd_alm(std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="info"){
			return cmd_info(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="selftest"){
			return cmd_test(
				std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="config"){
			return cmd_cfg(std::vector<std::string>(args.begin()+1,args.end()));
		}
		if(first=="completion"){
			return cmd_comp(
				std::vector<std::string>(args.begin()+1,args.end()));
		}

		return cmd_month(args);

	}catch(const std::invalid_argument&ex){
		std::cerr<<"argument error: "<<ex.what()<<std::endl;
		return 2;
	}catch(const std::exception&ex){
		std::cerr<<"error: "<<ex.what()<<std::endl;
		return 1;
	}
}
