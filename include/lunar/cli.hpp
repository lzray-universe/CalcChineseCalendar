#pragma once

#include<string>
#include<vector>

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

struct AtArgs{
	std::string ephem;
	std::string time_raw;
	std::string input_tz="+08:00";
	std::string tz="+08:00";
	std::string format="txt";
	std::string out;
	bool pretty=true;
	bool quiet=false;
	bool events=true;
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
	int lunar_year=0;
	int lun_mno=0;
	int lunar_day=0;
	bool leap=false;

	std::string input_tz="+08:00";
	std::string tz="+08:00";
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

int cmd_month(const std::vector<std::string>&args);
int cmd_cal(const std::vector<std::string>&args);
int cmd_year(const std::vector<std::string>&args);
int cmd_event(const std::vector<std::string>&args);
int cmd_dl(const std::vector<std::string>&args);
int cmd_at(const std::vector<std::string>&args);
int cmd_conv(const std::vector<std::string>&args);
int cmd_day(const std::vector<std::string>&args);
int cmd_mview(const std::vector<std::string>&args);
int cmd_next(const std::vector<std::string>&args);
int cmd_range(const std::vector<std::string>&args);
int cmd_search(const std::vector<std::string>&args);
int cmd_fest(const std::vector<std::string>&args);
int cmd_alm(const std::vector<std::string>&args);
int cmd_info(const std::vector<std::string>&args);
int cmd_test(const std::vector<std::string>&args);
int cmd_cfg(const std::vector<std::string>&args);
int cmd_comp(const std::vector<std::string>&args);

std::string tool_ver();

void use_main();
void use_month();
void use_cal();
void use_year();
void use_event();
void use_dl();
void use_at();
void use_conv();
void use_day();
void use_mview();
void use_next();
void use_range();
void use_search();
void use_fest();
void use_alm();
void use_info();
void use_test();
void use_cfg();
void use_comp();
