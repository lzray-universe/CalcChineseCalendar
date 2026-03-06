#pragma once

#include<string>
#include<vector>

#include "lunar/almanac.hpp"
#include "lunar/app_long.hpp"
#include "lunar/events.hpp"
#include "lunar/star.hpp"

struct LunDate{
	int lunar_year=0;
	int lun_mno=0;
	bool is_leap=false;
	std::string lun_mlab;
	int lunar_day=0;
	std::string lun_label;

	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	double cstday_jd=0.0;
};

struct GregDate{
	int year=0;
	int month=0;
	int day=0;
	double cstday_jd=0.0;
};

struct NearEvt{
	bool has=false;
	EventRec event;
};

struct NearEvents{
	NearEvt solar_prev;
	NearEvt solar_next;
	NearEvt phase_prev;
	NearEvt phase_next;
};

struct AtData{
	std::string time_raw;
	std::string tz_in;
	std::string display_tz;
	double jd_utc=0.0;
	double jd_tdb=0.0;
	std::string utc_iso;
	std::string local_iso;

	double lam_s=0.0;
	double lam_s_dot=0.0;
	double lam_m=0.0;
	double lam_m_dot=0.0;
	double elong=0.0;
	double elong_deg=0.0;
	double ill_frac=0.0;
	double ill_pct=0.0;
	bool waxing=false;
	std::string phase_name;

	LunDate lunar_date;
	bool inc_ev=false;
	NearEvents near_ev;
	bool has_eot=false;
	EoTData eot;
	lunar::MoonXg moon_xg;
	HliData hli;
};

struct DayResult{
	std::string ephem;
	std::string date_text;
	std::string at_time;
	std::string tz;
	double hli_lon_deg=120.0;
	bool inc_astro=false;
	std::string astro_mode_text="less";
	std::string astro_pick_csv;
	lunar::AstroObs astro_obs;
	AtData at_data;
	std::vector<EventRec> day_events;
	std::vector<EventRec> astro_events;
};
