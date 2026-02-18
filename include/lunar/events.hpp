#pragma once

#include<limits>
#include<string>

struct EventRec{
	std::string kind;
	std::string code;
	std::string name;
	int year=0;
	double jd_tdb=std::numeric_limits<double>::quiet_NaN();
	double jd_utc=std::numeric_limits<double>::quiet_NaN();
	std::string utc_iso;
	std::string loc_iso;
};

struct MonthRec{
	std::string label;
	int month_no=0;
	bool is_leap=false;
	double st_jdutc=std::numeric_limits<double>::quiet_NaN();
	double ed_jdutc=std::numeric_limits<double>::quiet_NaN();
	std::string st_utc;
	std::string st_loc;
	std::string ed_utc;
	std::string ed_loc;
};
