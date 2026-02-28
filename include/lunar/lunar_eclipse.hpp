#pragma once

#include<limits>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/events.hpp"
#include "lunar/spc_ephem.hpp"

struct LunarEclipse{
	bool has=false;
	std::string type="N";
	double jd_tdb_p1=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u1=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_max=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u4=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_p4=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u2=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u3=std::numeric_limits<double>::quiet_NaN();
	double pen_mag=std::numeric_limits<double>::quiet_NaN();
	double umb_mag=std::numeric_limits<double>::quiet_NaN();
};

bool calc_lunar_eclipse(EphRead&eph,double jd_tdb_near_full_moon,
						LunarEclipse*out);

std::vector<EventRec> bld_lunar_eclipse_events(EphRead&eph,
												const YearResult&yr,
												int tz_off);

bool lunar_eclipse_window_tdb(const LunarEclipse&ecl,
							  const std::string&stage_window,
							  double*jd_tdb_start,double*jd_tdb_end);

struct LunarEclipsePointVis{
	std::string stage_window="any";
	double lat_deg=0.0;
	double lon_deg=0.0;
	double height_m=0.0;
	bool visible=false;
	double max_alt_deg=std::numeric_limits<double>::quiet_NaN();
	double first_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double last_jd_utc=std::numeric_limits<double>::quiet_NaN();
	int sample_count=0;
};

struct LunarEclipseGlobalPoint{
	double lat_deg=0.0;
	double lon_deg=0.0;
	double max_alt_deg=std::numeric_limits<double>::quiet_NaN();
	double first_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double last_jd_utc=std::numeric_limits<double>::quiet_NaN();
};

struct LunarEclipseGlobalVis{
	std::string stage_window="any";
	double jd_start_utc=std::numeric_limits<double>::quiet_NaN();
	double jd_end_utc=std::numeric_limits<double>::quiet_NaN();
	double lat_step_deg=0.0;
	double lon_step_deg=0.0;
	int sample_count=0;
	std::vector<LunarEclipseGlobalPoint> points;
};

bool lunar_eclipse_point_visibility(EphRead&eph,const LunarEclipse&ecl,
									const std::string&stage_window,
									double lat_deg,double lon_deg,double height_m,
									double sample_minutes,bool refine_edge,
									LunarEclipsePointVis*out);

bool lunar_eclipse_global_visibility(EphRead&eph,const LunarEclipse&ecl,
									 const std::string&stage_window,
									 double lat_step_deg,double lon_step_deg,
									 double sample_minutes,
									 LunarEclipseGlobalVis*out);
