#pragma once

#include<cstddef>
#include<fstream>
#include<set>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/events.hpp"

namespace cli_util{

struct OutTgt{
	std::ofstream file;
	std::ostream*stream=nullptr;
};

bool is_opt(const std::string&s);

std::string to_low(std::string s);

int parse_int(const std::string&text,const std::string&label);

bool parse_bool01(const std::string&text,const std::string&label);

std::string req_val(const std::vector<std::string>&args,std::size_t&idx,
					const std::string&opt);

OutTgt open_out(const std::string&path);

void note_out(const std::string&path,bool quiet);

void chk_fmt(const std::string&format,const std::set<std::string>&allowed,
			 const std::string&ctx);

EventRec mk_erec(const std::string&kind,const std::string&code,
				 const std::string&name,int year,double jd_tdb,double jd_utc,
				 int tz_off);

EventRec mk_erec(const std::string&kind,const std::string&code,
				 const std::string&name,int year,double jd_utc,int tz_off);

std::vector<EventRec> bld_stev(const YearResult&yr,int tz_off);

std::vector<EventRec> bld_lpev(const YearResult&yr,int tz_off);

} // namespace cli_util
