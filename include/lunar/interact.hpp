#pragma once

#include<filesystem>
#include<string>
#include<vector>

#include "lunar/almanac.hpp"
#include "lunar/cli/main.hpp"
#include "lunar/download.hpp"

struct InterCfg{
	std::string bsp_dir;
	std::string def_bsp;
	std::vector<std::string> bsp_list;
	std::string default_tz="+08:00";
	std::string default_lang="zh";
	std::string default_lunar_day_tz;
	std::string def_fmt="txt";
	std::string hli_trad="folk";
	std::string hli_year_boundary;
	std::string hli_month_boundary;
	std::string hli_leap_month_mode;
	std::string hli_day_boundary;
	bool def_prety=true;
};

extern const std::string CFG_FILE;

std::string trim(const std::string&s);

bool load_cfg(InterCfg&cfg);

bool save_cfg(const InterCfg&cfg);

std::string default_lunar_day_tz_for_lang(const std::string&lang_code);

std::string resolve_lunar_day_tz(const InterCfg&cfg);

HliRuleSet hli_rules_from_cfg(const InterCfg&cfg);

bool file_ok(const std::string&path);

std::vector<std::filesystem::path>
find_bsps(const std::vector<std::filesystem::path>&dirs);

std::string ask_line(const std::string&msg);

bool ask_yes_no(const std::string&msg,bool yes_def=true);

std::string pick_bsp(InterCfg&cfg);

std::string init_bsp(InterCfg&cfg);

void int_month(const std::string&ephem);

void int_cal(const std::string&ephem);

void int_at(const std::string&ephem);

void int_conv(const std::string&ephem);

std::string init_bspq(InterCfg&cfg);

void int_mode();
