#pragma once

#include<array>
#include<string>
#include<vector>

#include "lunar/app_long.hpp"
#include "lunar/calendar.hpp"
#include "lunar/star.hpp"

struct GzNode{
	int stem=0;
	int branch=0;
	std::string text;
};

struct HliHour{
	std::string slot;
	std::string gz;
	std::string luck;
};

struct HliData{
	GzNode y_lun;
	GzNode y_lchun;
	GzNode m_gz;
	GzNode d_gz;
	GzNode h_gz;
	GzNode h_gz_true;
	std::string bazi_clock;
	std::string bazi_true;

	std::string jianchu;
	std::string duty_god;
	std::string duty_tag;
	std::string clash;
	std::string chong_sha;
	std::string six_he;
	std::string three_he;
	std::string zodiac_day;
	std::string pengzu;
	std::string nayin;
	std::string wx_day;
	std::string fetal_god;
	std::string meridian;
	std::string lucky_dir;
	std::string wealth_dir;
	std::string mascot_dir;
	std::string sun_noble_dir;
	std::string moon_noble_dir;
	std::string xiu28;
	std::string xiu_id;

	std::vector<std::string> good_gods;
	std::vector<std::string> bad_gods;
	std::vector<std::string> yi;
	std::vector<std::string> ji;

	int yi_ji_level=0;
	std::string yi_ji_rule;

	double lon_deg=120.0;
	double eot_min=0.0;
	double tst_min=0.0;

	std::vector<HliHour> hour_jx;
};

struct HliInput{
	double jd_utc=0.0;
	int gy=0;
	int gm=0;
	int gd=0;
	int hh=0;
	int mm=0;
	double ss=0.0;
	int tz_off=480;
	double lon_deg=120.0;

	int lun_year=0;
	int lun_month=0;
	int lun_day=0;
	bool lun_leap=false;

	std::string phase_name;
	lunar::MoonXg moon_xg;
};

HliData calc_hli(EphRead&eph,LunCal6&lc,SolLunCal&solver,AppLon&app,
				 const HliInput&in);
