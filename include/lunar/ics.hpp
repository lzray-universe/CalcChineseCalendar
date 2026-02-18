#pragma once

#include<iosfwd>
#include<string>
#include<vector>

struct IcsEvent{
	std::string uid;
	std::string summary;
	std::string desc;
	double jd_utc=0.0;
};

std::string ics_time(double jd_utc);

void write_ics(std::ostream&os,const std::string&prodid,
			   const std::string&cal_name,const std::vector<IcsEvent>&events,
			   const std::vector<std::string>&x_notes={});
