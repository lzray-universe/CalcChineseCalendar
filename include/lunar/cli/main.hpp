#pragma once

#include<string>
#include<vector>

#include "lunar/cli/query.hpp"

std::vector<int> parse_year(const std::string&arg);

struct MonthsArgs{
	std::string ephem;
	std::string years;
	std::string mode="lunar";
	std::string format="txt";
	std::string out;
	std::string tz="+08:00";
	bool pretty=true;
	bool quiet=false;
	std::string out_json;
	std::string out_txt;
};

void cli_month(const MonthsArgs&args);

struct CalArgs{
	std::string ephem;
	std::string years_arg;
	bool has_years=false;
	std::string format="txt";
	std::string out;
	std::string tz="+08:00";
	bool inc_month=false;
	bool pretty=true;
	bool quiet=false;
};

void cli_cal(const CalArgs&args);

struct YearArgs{
	std::string ephem;
	int year=0;
	std::string mode="lunar";
	std::string format="txt";
	std::string out;
	std::string tz="+08:00";
	bool pretty=true;
	bool quiet=false;
};

void cli_year(const YearArgs&args);

struct EventArgs{
	std::string ephem;
	std::string category;
	std::string code;
	int year=0;
	bool has_year=false;
	std::string near_date;
	std::string format="txt";
	std::string out;
	std::string tz="+08:00";
	bool pretty=true;
	bool quiet=false;
};

void cli_event(const EventArgs&args);

struct DlArgs{
	std::string action;
	std::string id;
	std::string dir;
	bool quiet=false;
};

void cli_dl(const DlArgs&args);

int cmd_month(const std::vector<std::string>&args);
int cmd_cal(const std::vector<std::string>&args);
int cmd_year(const std::vector<std::string>&args);
int cmd_event(const std::vector<std::string>&args);
int cmd_dl(const std::vector<std::string>&args);

std::string tool_ver();

void use_main();
void use_month();
void use_cal();
void use_year();
void use_event();
void use_dl();

