#pragma once

#include<string>

#include "lunar/models.hpp"

namespace lunar::core{

struct GanzhiComputeOptions{
	std::string ephem;
	std::string date_text;
	std::string at_time="12:00:00";
	std::string tz="+08:00";
	HliRuleSet hli_rules=make_hli_rule_set(HliProfileCode::Folk);
};

struct GanzhiSummary{
	GzNode year;
	GzNode month;
	GzNode day;
	HliRuleSet hli_rules=make_hli_rule_set(HliProfileCode::Folk);
};

struct GanzhiMonthComputeOptions{
	std::string ephem;
	int year=0;
	int month=0;
	std::string at_time="12:00:00";
	std::string tz="+08:00";
	HliRuleSet hli_rules=make_hli_rule_set(HliProfileCode::Folk);
};

struct GanzhiMonthSummary{
	int year=0;
	int month=0;
	std::string at_time="12:00:00";
	std::string tz="+08:00";
	HliRuleSet hli_rules=make_hli_rule_set(HliProfileCode::Folk);
	std::vector<GzNode> years;
	std::vector<GzNode> months;
	std::vector<GzNode> days;
};

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
	HliRuleSet hli_rules=make_hli_rule_set(HliProfileCode::Folk);
};

GanzhiSummary compute_ganzhi(const GanzhiComputeOptions&opt);

GanzhiMonthSummary compute_ganzhi_month(const GanzhiMonthComputeOptions&opt);

DayResult compute_day(const DayComputeOptions&opt);

}
