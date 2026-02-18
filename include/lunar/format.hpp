#pragma once

#include<string>

int parse_tz(const std::string&tz);

std::string fmt_tz(int off_min);

struct IsoTime{
	double jd_utc=0.0;
	int tz_off=0;
	bool has_tz=false;
};

IsoTime parse_iso(const std::string&text,const std::string&default_tz);

std::string fmt_iso(double jd_utc,int off_min,bool with_ms=true);
