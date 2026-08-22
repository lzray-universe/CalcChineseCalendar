#pragma once

#include<string>
#include<vector>

#include "lunar/almanac.hpp"

struct AtArgs{
	std::string ephem;
	std::string time_raw;
	std::string input_tz="+08:00";
	std::string tz="+08:00";
	std::string lunar_day_tz="+08:00";
	std::string format="txt";
	std::string out;
	bool pretty=true;
	bool quiet=false;
	bool events=true;
	bool calc_eot=false;
	double eot_lon_deg=0.0;
	std::string hli_trad="folk";
	HliRuleSet hli_rules=make_hli_rule_set(HliProfileCode::Folk);
	bool from_stdin=false;
	std::string input_file;
	int jobs=1;
	bool meta_once=false;
};

void cli_at(const AtArgs&args);

struct ConvArgs{
	std::string ephem;
	std::string in_value;
	bool has_in=false;

	bool from_lunar=false;
	bool has_lunar_input=false;
	int lunar_year=0;
	int lun_mno=0;
	int lunar_day=0;
	bool leap=false;

	std::string input_tz="+08:00";
	std::string tz="+08:00";
	std::string lunar_day_tz="+08:00";
	std::string format="txt";
	std::string out;
	bool pretty=true;
	bool quiet=false;
	bool from_stdin=false;
	std::string input_file;
	int jobs=1;
	bool meta_once=false;
};

void cli_conv(const ConvArgs&args);

int cmd_at(const std::vector<std::string>&args);
int cmd_conv(const std::vector<std::string>&args);
int cmd_sky(const std::vector<std::string>&args);
int cmd_day(const std::vector<std::string>&args);
int cmd_mview(const std::vector<std::string>&args);
int cmd_export(const std::vector<std::string>&args);
int cmd_next(const std::vector<std::string>&args);
int cmd_range(const std::vector<std::string>&args);
int cmd_search(const std::vector<std::string>&args);
int cmd_zodiac(const std::vector<std::string>&args);
int cmd_eclipse(const std::vector<std::string>&args);
int cmd_eclipse_magnitude(const std::vector<std::string>&args);
int cmd_fest(const std::vector<std::string>&args);
int cmd_alm(const std::vector<std::string>&args);
int cmd_info(const std::vector<std::string>&args);
int cmd_cfg(const std::vector<std::string>&args);
int cmd_comp(const std::vector<std::string>&args);

void use_at();
void use_conv();
void use_sky();
void use_day();
void use_mview();
void use_export();
void use_next();
void use_range();
void use_search();
void use_zodiac();
void use_eclipse();
void use_eclipse_magnitude();
void use_fest();
void use_alm();
void use_info();
void use_cfg();
void use_comp();
