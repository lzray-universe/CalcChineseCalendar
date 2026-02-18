#pragma once

#include<filesystem>
#include<string>
#include<vector>

#include "lunar/cli.hpp"
#include "lunar/download.hpp"

struct InterCfg{
	std::string bsp_dir;
	std::string def_bsp;
	std::string default_tz="+08:00";
	std::string def_fmt="txt";
	bool def_prety=true;
};

extern const std::string CFG_FILE;

std::string trim(const std::string&s);

bool load_cfg(InterCfg&cfg);

bool save_cfg(const InterCfg&cfg);

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
