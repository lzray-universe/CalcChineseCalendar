#include<string>
#include<tuple>
#include<vector>

#include<pybind11/pybind11.h>
#include<pybind11/stl.h>

#include "lunar/c_api.h"

namespace py=pybind11;

namespace{

std::tuple<int,std::string,std::string,std::string>
run_capture(const std::vector<std::string>&args){
	std::vector<const char*> argv;
	argv.reserve(args.size());
	for(const std::string&arg : args){
		argv.push_back(arg.c_str());
	}
	const int rc=lunar_run_capture(static_cast<int>(argv.size()),argv.data());
	const char*stdout_text=lunar_last_stdout();
	const char*stderr_text=lunar_last_stderr();
	const char*error_text=lunar_last_error();
	return std::make_tuple(
		rc,
		std::string(stdout_text==nullptr?"":stdout_text),
		std::string(stderr_text==nullptr?"":stderr_text),
		std::string(error_text==nullptr?"":error_text));
}

std::string req_json_result(int rc){
	const char*stdout_text=lunar_last_stdout();
	const char*error_text=lunar_last_error();
	if(rc!=0){
		throw py::value_error(
			error_text==nullptr||error_text[0]=='\0'
				?"lunar native call failed"
				:error_text);
	}
	return stdout_text==nullptr?std::string():std::string(stdout_text);
}

std::string calc_eot_json(const std::string&ephem,double jd_utc,double lon_deg,
						  bool pretty){
	return req_json_result(
		lunar_calc_eot_json(ephem.c_str(),jd_utc,lon_deg,pretty?1:0));
}

std::string core_day_json(const std::string&ephem,const std::string&date,
						  const std::string&tz,bool pretty){
	return req_json_result(
		lunar_core_day_json(ephem.c_str(),date.c_str(),tz.c_str(),pretty?1:0));
}

std::string core_ganzhi_json(const std::string&ephem,const std::string&date,
							 const std::string&at_time,const std::string&tz,
							 int profile_code,int year_boundary,
							 int month_boundary,int leap_month_mode,
							 int day_boundary,bool pretty){
	lunar_hli_rules rules{};
	rules.profile_code=profile_code;
	rules.year_boundary=year_boundary;
	rules.month_boundary=month_boundary;
	rules.leap_month_mode=leap_month_mode;
	rules.day_boundary=day_boundary;
	return req_json_result(
		lunar_core_ganzhi_json(
			ephem.c_str(),date.c_str(),at_time.c_str(),tz.c_str(),
			&rules,pretty?1:0));
}

std::string core_ganzhi_month_json(const std::string&ephem,int year,int month,
								   const std::string&at_time,
								   const std::string&tz,int profile_code,
								   int year_boundary,int month_boundary,
								   int leap_month_mode,int day_boundary,
								   bool pretty){
	lunar_hli_rules rules{};
	rules.profile_code=profile_code;
	rules.year_boundary=year_boundary;
	rules.month_boundary=month_boundary;
	rules.leap_month_mode=leap_month_mode;
	rules.day_boundary=day_boundary;
	return req_json_result(
		lunar_core_ganzhi_month_json(
			ephem.c_str(),year,month,at_time.c_str(),tz.c_str(),
			&rules,pretty?1:0));
}

std::string tool_version(){
	const char*text=lunar_tool_ver();
	return text==nullptr?std::string():std::string(text);
}

}

PYBIND11_MODULE(_lunar_ext,m){
	m.def("run_capture",&run_capture);
	m.def("calc_eot_json",&calc_eot_json);
	m.def("core_day_json",&core_day_json,
		  py::arg("ephem"),py::arg("date"),py::arg("tz")="+08:00",
		  py::arg("pretty")=false);
	m.def("core_ganzhi_json",&core_ganzhi_json,
		  py::arg("ephem"),py::arg("date"),
		  py::arg("at_time")="12:00:00",py::arg("tz")="+08:00",
		  py::arg("profile_code")=0,py::arg("year_boundary")=1,
		  py::arg("month_boundary")=1,py::arg("leap_month_mode")=1,
		  py::arg("day_boundary")=0,py::arg("pretty")=false);
	m.def("core_ganzhi_month_json",&core_ganzhi_month_json,
		  py::arg("ephem"),py::arg("year"),py::arg("month"),
		  py::arg("at_time")="12:00:00",py::arg("tz")="+08:00",
		  py::arg("profile_code")=0,py::arg("year_boundary")=1,
		  py::arg("month_boundary")=1,py::arg("leap_month_mode")=1,
		  py::arg("day_boundary")=0,py::arg("pretty")=false);
	m.def("tool_version",&tool_version);
}
