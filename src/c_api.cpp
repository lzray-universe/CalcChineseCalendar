#include "lunar/c_api.h"

#include<exception>
#include<stdexcept>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/cli.hpp"
#include "lunar/entry.hpp"

namespace{

thread_local std::string g_last_error;

void set_err(const std::string&msg){
	g_last_error=msg;
}

void clr_err(){
	g_last_error.clear();
}

std::vector<std::string> mk_args(int argc,const char*const*argv){
	if(argc<0){
		throw std::invalid_argument("argc must be >= 0");
	}
	if(argc>0&&argv==nullptr){
		throw std::invalid_argument("argv must not be null when argc > 0");
	}

	std::vector<std::string> out;
	out.reserve(static_cast<std::size_t>(argc));
	for(int i=0;i<argc;++i){
		if(argv[i]==nullptr){
			throw std::invalid_argument("argv contains null entry");
		}
		out.emplace_back(argv[i]);
	}
	return out;
}

int map_ex(const std::invalid_argument&ex){
	set_err(ex.what());
	return 2;
}

int map_ex(const std::exception&ex){
	set_err(ex.what());
	return 1;
}

int map_ukn_ex(){
	set_err("unknown error");
	return 1;
}

template<typename Fn>
int guard(Fn&&fn){
	clr_err();
	try{
		return fn();
	}catch(const std::invalid_argument&ex){
		return map_ex(ex);
	}catch(const std::exception&ex){
		return map_ex(ex);
	}catch(...){
		return map_ukn_ex();
	}
}

using CmdFn=int(*)(const std::vector<std::string>&args);

int run_cmd(CmdFn cmd,int argc,const char*const*argv){
	std::vector<std::string> args=mk_args(argc,argv);
	return cmd(args);
}

}

extern "C"{

const char*LUNAR_CALL lunar_tool_ver(void){
	static thread_local std::string ver;
	ver=tool_ver();
	return ver.c_str();
}

const char*LUNAR_CALL lunar_last_error(void){
	return g_last_error.empty()?nullptr:g_last_error.c_str();
}

void LUNAR_CALL lunar_clear_error(void){
	clr_err();
}

int LUNAR_CALL lunar_run(int argc,const char*const*argv){
	return guard([&](){
		std::vector<std::string> args=mk_args(argc,argv);
		return run_cli_args(args);
	});
}

int LUNAR_CALL lunar_root_batch(const char*ephem,const char*input_path,
								const char*out_path){
	return guard([&](){
		if(ephem==nullptr||input_path==nullptr||out_path==nullptr){
			throw std::invalid_argument(
				"ephem/input_path/out_path must not be null");
		}
		return run_rootw(ephem,input_path,out_path);
	});
}

int LUNAR_CALL lunar_cmd_month(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_month,argc,argv); });
}

int LUNAR_CALL lunar_cmd_cal(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_cal,argc,argv); });
}

int LUNAR_CALL lunar_cmd_year(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_year,argc,argv); });
}

int LUNAR_CALL lunar_cmd_event(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_event,argc,argv); });
}

int LUNAR_CALL lunar_cmd_dl(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_dl,argc,argv); });
}

int LUNAR_CALL lunar_cmd_at(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_at,argc,argv); });
}

int LUNAR_CALL lunar_cmd_conv(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_conv,argc,argv); });
}

int LUNAR_CALL lunar_cmd_day(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_day,argc,argv); });
}

int LUNAR_CALL lunar_cmd_mview(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_mview,argc,argv); });
}

int LUNAR_CALL lunar_cmd_next(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_next,argc,argv); });
}

int LUNAR_CALL lunar_cmd_range(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_range,argc,argv); });
}

int LUNAR_CALL lunar_cmd_search(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_search,argc,argv); });
}

int LUNAR_CALL lunar_cmd_fest(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_fest,argc,argv); });
}

int LUNAR_CALL lunar_cmd_alm(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_alm,argc,argv); });
}

int LUNAR_CALL lunar_cmd_info(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_info,argc,argv); });
}

int LUNAR_CALL lunar_cmd_test(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_test,argc,argv); });
}

int LUNAR_CALL lunar_cmd_cfg(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_cfg,argc,argv); });
}

int LUNAR_CALL lunar_cmd_comp(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_comp,argc,argv); });
}

int LUNAR_CALL lunar_use_main(void){
	return guard([](){
		use_main();
		return 0;
	});
}

int LUNAR_CALL lunar_use_month(void){
	return guard([](){
		use_month();
		return 0;
	});
}

int LUNAR_CALL lunar_use_cal(void){
	return guard([](){
		use_cal();
		return 0;
	});
}

int LUNAR_CALL lunar_use_year(void){
	return guard([](){
		use_year();
		return 0;
	});
}

int LUNAR_CALL lunar_use_event(void){
	return guard([](){
		use_event();
		return 0;
	});
}

int LUNAR_CALL lunar_use_dl(void){
	return guard([](){
		use_dl();
		return 0;
	});
}

int LUNAR_CALL lunar_use_at(void){
	return guard([](){
		use_at();
		return 0;
	});
}

int LUNAR_CALL lunar_use_conv(void){
	return guard([](){
		use_conv();
		return 0;
	});
}

int LUNAR_CALL lunar_use_day(void){
	return guard([](){
		use_day();
		return 0;
	});
}

int LUNAR_CALL lunar_use_mview(void){
	return guard([](){
		use_mview();
		return 0;
	});
}

int LUNAR_CALL lunar_use_next(void){
	return guard([](){
		use_next();
		return 0;
	});
}

int LUNAR_CALL lunar_use_range(void){
	return guard([](){
		use_range();
		return 0;
	});
}

int LUNAR_CALL lunar_use_search(void){
	return guard([](){
		use_search();
		return 0;
	});
}

int LUNAR_CALL lunar_use_fest(void){
	return guard([](){
		use_fest();
		return 0;
	});
}

int LUNAR_CALL lunar_use_alm(void){
	return guard([](){
		use_alm();
		return 0;
	});
}

int LUNAR_CALL lunar_use_info(void){
	return guard([](){
		use_info();
		return 0;
	});
}

int LUNAR_CALL lunar_use_test(void){
	return guard([](){
		use_test();
		return 0;
	});
}

int LUNAR_CALL lunar_use_cfg(void){
	return guard([](){
		use_cfg();
		return 0;
	});
}

int LUNAR_CALL lunar_use_comp(void){
	return guard([](){
		use_comp();
		return 0;
	});
}

}
