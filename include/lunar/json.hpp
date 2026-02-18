#pragma once

#include<string>
#include<vector>

#include "lunar/calendar.hpp"

struct SerMonth{
	std::string label;
	int month_no;
	bool is_leap;
	std::string start;
	std::string end;
};

SerMonth ser_month(const LunarMonth&m);

struct YearBundle{
	int year;
	std::string mode;
	std::vector<SerMonth> months;
};

std::vector<YearBundle> get_years(LunCal6&calc,const std::vector<int>&years,
								  const std::string&mode);
