#include "lunar/c_api.h"

#include<cstring>
#include<cstdio>
#include<exception>
#include<stdexcept>
#include<string>
#include<vector>

#include "lunar/app_long.hpp"
#include "lunar/calendar.hpp"
#include "lunar/cli/main.hpp"
#include "lunar/core.hpp"
#include "lunar/entry.hpp"

namespace{

thread_local std::string g_last_error;

void set_err(const std::string&msg){
	g_last_error=msg;
}

void clr_err(){
	g_last_error.clear();
}

template<typename Enum>
Enum req_hli_code(int code,const char*name);

template<>
HliProfileCode req_hli_code<HliProfileCode>(int code,const char*name){
	switch(static_cast<HliProfileCode>(code)){
		case HliProfileCode::Folk:
		case HliProfileCode::ZiPing:
		case HliProfileCode::PurpleStar:
		case HliProfileCode::XieJi:
		case HliProfileCode::Custom:
			return static_cast<HliProfileCode>(code);
	}
	throw std::invalid_argument(std::string("invalid ")+name);
}

template<>
HliYearBoundary req_hli_code<HliYearBoundary>(int code,const char*name){
	switch(static_cast<HliYearBoundary>(code)){
		case HliYearBoundary::LiChun:
		case HliYearBoundary::LunarNewYear:
		case HliYearBoundary::WinterSolstice:
			return static_cast<HliYearBoundary>(code);
	}
	throw std::invalid_argument(std::string("invalid ")+name);
}

template<>
HliMonthBoundary req_hli_code<HliMonthBoundary>(int code,const char*name){
	switch(static_cast<HliMonthBoundary>(code)){
		case HliMonthBoundary::SolarTerm:
		case HliMonthBoundary::LunarFirstDay:
			return static_cast<HliMonthBoundary>(code);
	}
	throw std::invalid_argument(std::string("invalid ")+name);
}

template<>
HliLeapMonthMode req_hli_code<HliLeapMonthMode>(int code,const char*name){
	switch(static_cast<HliLeapMonthMode>(code)){
		case HliLeapMonthMode::Ignore:
		case HliLeapMonthMode::InheritPrevious:
		case HliLeapMonthMode::SplitMidway:
		case HliLeapMonthMode::ShiftToNext:
			return static_cast<HliLeapMonthMode>(code);
	}
	throw std::invalid_argument(std::string("invalid ")+name);
}

template<>
HliDayBoundary req_hli_code<HliDayBoundary>(int code,const char*name){
	switch(static_cast<HliDayBoundary>(code)){
		case HliDayBoundary::Hour23:
		case HliDayBoundary::Hour0:
			return static_cast<HliDayBoundary>(code);
	}
	throw std::invalid_argument(std::string("invalid ")+name);
}

int gz_index60(int stem,int branch){
	for(int idx=0;idx<60;++idx){
		if(idx%10==stem&&idx%12==branch){
			return idx;
		}
	}
	return -1;
}

void fill_ganzhi_node(lunar_ganzhi_node*out,const GzNode&src){
	out->index=gz_index60(src.stem,src.branch);
	out->stem=src.stem;
	out->branch=src.branch;
	std::snprintf(out->text,sizeof(out->text),"%s",src.text.c_str());
}

template<typename Summary>
void fill_rule_codes(Summary*out,const HliRuleSet&rules){
	out->rule_profile_code=rules.profile_code;
	out->year_boundary_code=rules.year_boundary;
	out->month_boundary_code=rules.month_boundary;
	out->leap_month_mode_code=rules.leap_month_mode;
	out->day_boundary_code=rules.day_boundary;
}

HliRuleSet rules_from_c(const lunar_hli_rules*rules){
	if(rules==nullptr){
		return make_hli_rule_set(HliProfileCode::Folk);
	}
	HliRuleSet out;
	out.profile_code=
		static_cast<int>(req_hli_code<HliProfileCode>(
			rules->profile_code,"hli_rules.profile_code"));
	out.year_boundary=
		static_cast<int>(req_hli_code<HliYearBoundary>(
			rules->year_boundary,"hli_rules.year_boundary"));
	out.month_boundary=
		static_cast<int>(req_hli_code<HliMonthBoundary>(
			rules->month_boundary,"hli_rules.month_boundary"));
	out.leap_month_mode=
		static_cast<int>(req_hli_code<HliLeapMonthMode>(
			rules->leap_month_mode,"hli_rules.leap_month_mode"));
	out.day_boundary=
		static_cast<int>(req_hli_code<HliDayBoundary>(
			rules->day_boundary,"hli_rules.day_boundary"));
	return normalize_hli_rule_set(out);
}

std::string c_api_or_default(const char*text,const char*fallback){
	if(text==nullptr||text[0]=='\0'){
		return fallback;
	}
	return text;
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

int LUNAR_CALL lunar_calc_eot(const char*ephem,double jd_utc,double lon_deg,
							  lunar_eot_result*out){
	return guard([&](){
		if(ephem==nullptr||out==nullptr){
			throw std::invalid_argument("ephem/out must not be null");
		}
		EphRead eph(ephem);
		AppLon app(eph);
		EoTData data=app.eot_calc(jd_utc,lon_deg);
		out->jd_utc=data.jd_utc;
		out->jd_tdb=data.jd_tdb;
		out->lon_deg=data.lon_deg;
		out->lon_rad=data.lon_rad;
		out->apparent_solar_time_rad=data.apparent_solar_time_rad;
		out->mean_solar_time_rad=data.mean_solar_time_rad;
		out->eot_rad=data.eot_rad;
		out->eot_minutes=data.eot_minutes;
		out->eot_seconds=data.eot_seconds;
		return 0;
	});
}

int LUNAR_CALL lunar_core_day(const char*ephem,const char*date,const char*tz,
							  lunar_day_summary*out){
	return guard([&](){
		if(ephem==nullptr||date==nullptr||out==nullptr){
			throw std::invalid_argument("ephem/date/out must not be null");
		}
		lunar::core::DayComputeOptions opt;
		opt.ephem=ephem;
		opt.date_text=date;
		opt.tz=(tz==nullptr||std::string(tz).empty())?"+08:00":std::string(tz);
		opt.include_events=false;
		opt.include_astro=false;
		DayResult day=lunar::core::compute_day(opt);
		out->lunar_year=day.at_data.lunar_date.lunar_year;
		out->lun_mno=day.at_data.lunar_date.lun_mno;
		out->lunar_day=day.at_data.lunar_date.lunar_day;
		out->is_leap=day.at_data.lunar_date.is_leap?1:0;
		out->ill_pct=day.at_data.ill_pct;
		std::snprintf(out->phase_name,sizeof(out->phase_name),"%s",
					  day.at_data.phase_name.c_str());
		std::snprintf(out->lun_label,sizeof(out->lun_label),"%s",
					  day.at_data.lunar_date.lun_label.c_str());
		return 0;
	});
}

void LUNAR_CALL lunar_hli_rules_init(lunar_hli_rules*out){
	clr_err();
	if(out==nullptr){
		set_err("out must not be null");
		return;
	}
	HliRuleSet rules=make_hli_rule_set(HliProfileCode::Folk);
	out->profile_code=rules.profile_code;
	out->year_boundary=rules.year_boundary;
	out->month_boundary=rules.month_boundary;
	out->leap_month_mode=rules.leap_month_mode;
	out->day_boundary=rules.day_boundary;
}

int LUNAR_CALL lunar_core_ganzhi(const char*ephem,const char*date,
								 const char*at_time,const char*tz,
								 const lunar_hli_rules*rules,
								 lunar_ganzhi_summary*out){
	return guard([&](){
		if(ephem==nullptr||date==nullptr||out==nullptr){
			throw std::invalid_argument("ephem/date/out must not be null");
		}
		lunar::core::GanzhiComputeOptions opt;
		opt.ephem=ephem;
		opt.date_text=date;
		opt.at_time=c_api_or_default(at_time,"12:00:00");
		opt.tz=c_api_or_default(tz,"+08:00");
		opt.hli_rules=rules_from_c(rules);
		lunar::core::GanzhiSummary sum=lunar::core::compute_ganzhi(opt);
		std::memset(out,0,sizeof(*out));
		fill_ganzhi_node(&out->year,sum.year);
		fill_ganzhi_node(&out->month,sum.month);
		fill_ganzhi_node(&out->day,sum.day);
		fill_rule_codes(out,sum.hli_rules);
		return 0;
	});
}

int LUNAR_CALL lunar_core_ganzhi_month(
	const char*ephem,int year,int month,const char*at_time,const char*tz,
	const lunar_hli_rules*rules,lunar_ganzhi_month_summary*out){
	return guard([&](){
		if(ephem==nullptr||out==nullptr){
			throw std::invalid_argument("ephem/out must not be null");
		}
		lunar::core::GanzhiMonthComputeOptions opt;
		opt.ephem=ephem;
		opt.year=year;
		opt.month=month;
		opt.at_time=c_api_or_default(at_time,"12:00:00");
		opt.tz=c_api_or_default(tz,"+08:00");
		opt.hli_rules=rules_from_c(rules);
		lunar::core::GanzhiMonthSummary sum=lunar::core::compute_ganzhi_month(opt);
		if(sum.years.size()!=sum.months.size()||sum.months.size()!=sum.days.size()){
			throw std::runtime_error("ganzhi month result size mismatch");
		}
		if(sum.days.size()>31){
			throw std::runtime_error("ganzhi month result exceeds buffer");
		}
		std::memset(out,0,sizeof(*out));
		out->year=sum.year;
		out->month=sum.month;
		out->day_count=static_cast<int>(sum.days.size());
		fill_rule_codes(out,sum.hli_rules);
		for(std::size_t i=0;i<sum.days.size();++i){
			fill_ganzhi_node(&out->years[i],sum.years[i]);
			fill_ganzhi_node(&out->months[i],sum.months[i]);
			fill_ganzhi_node(&out->days[i],sum.days[i]);
		}
		return 0;
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

int LUNAR_CALL lunar_cmd_zodiac(int argc,const char*const*argv){
	return guard([&](){ return run_cmd(cmd_zodiac,argc,argv); });
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

int LUNAR_CALL lunar_use_zodiac(void){
	return guard([](){
		use_zodiac();
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
