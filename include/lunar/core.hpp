#pragma once

#include<string>

#include "lunar/models.hpp"

namespace lunar::core{

struct DayComputeOptions{
	std::string ephem;
	std::string date_text;
	std::string at_time="12:00:00";
	std::string tz="+08:00";
	bool quiet=false;
	bool include_events=true;
	bool include_astro=false;
	std::string astro_mode_text="less";
	std::string astro_pick_csv;
	double astro_lat_deg=0.0;
	double astro_lon_deg=0.0;
	double astro_height_m=0.0;
	bool has_astro_site=false;
	double hli_lon_deg=120.0;
};

DayResult compute_day(const DayComputeOptions&opt);

}
