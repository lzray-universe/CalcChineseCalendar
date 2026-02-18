#include "lunar/json.hpp"

SerMonth ser_month(const LunarMonth&m){
	SerMonth s;
	s.label=m.label;
	s.month_no=m.month_no;
	s.is_leap=m.is_leap;
	s.start=fmt_local(m.start_dt);
	s.end=fmt_local(m.end_dt);
	return s;
}

std::vector<YearBundle> get_years(LunCal6&calc,const std::vector<int>&years,
								  const std::string&mode){
	std::vector<YearBundle> results;
	for(int y : years){
		std::vector<LunarMonth> months=
			(mode=="lunar")?enum_lyr(calc,y):enum_gyr(calc,y);
		YearBundle bundle;
		bundle.year=y;
		bundle.mode=mode;
		for(const auto&m : months){
			bundle.months.push_back(ser_month(m));
		}
		results.push_back(bundle);
	}
	return results;
}
