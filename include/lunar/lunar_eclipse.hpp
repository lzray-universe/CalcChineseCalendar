#pragma once

#include<limits>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/events.hpp"
#include "lunar/spc_ephem.hpp"

struct EclipseGeoCoord{
	double ra_deg=std::numeric_limits<double>::quiet_NaN();
	double dec_deg=std::numeric_limits<double>::quiet_NaN();
	double sd_deg=std::numeric_limits<double>::quiet_NaN();
	double ehp_deg=std::numeric_limits<double>::quiet_NaN();
};

struct EclipseLibration{
	double l_deg=std::numeric_limits<double>::quiet_NaN();
	double b_deg=std::numeric_limits<double>::quiet_NaN();
	double c_deg=std::numeric_limits<double>::quiet_NaN();
};

struct EclipsePointMeta{
	double zen_lat_deg=std::numeric_limits<double>::quiet_NaN();
	double zen_lon_deg=std::numeric_limits<double>::quiet_NaN();
	double pa_deg=std::numeric_limits<double>::quiet_NaN();
	double axis_deg=std::numeric_limits<double>::quiet_NaN();
};

struct LunarEclipse{
	bool has=false;
	std::string type="N";
	double jd_tdb_p1=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u1=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_max=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_opp=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u4=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_p4=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u2=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_u3=std::numeric_limits<double>::quiet_NaN();
	double pen_mag=std::numeric_limits<double>::quiet_NaN();
	double umb_mag=std::numeric_limits<double>::quiet_NaN();
	double rp_re=std::numeric_limits<double>::quiet_NaN();
	double ru_re=std::numeric_limits<double>::quiet_NaN();
	double opp_rp_re=std::numeric_limits<double>::quiet_NaN();
	double opp_ru_re=std::numeric_limits<double>::quiet_NaN();
	double dur_pen_sec=std::numeric_limits<double>::quiet_NaN();
	double dur_umb_sec=std::numeric_limits<double>::quiet_NaN();
	double dur_tot_sec=std::numeric_limits<double>::quiet_NaN();
	// Delta T = TT-UT1 at maximum, in seconds.
	double dt_max_sec=std::numeric_limits<double>::quiet_NaN();
	double moon_dist_km=std::numeric_limits<double>::quiet_NaN();
	double gamma=std::numeric_limits<double>::quiet_NaN();
	double eps_deg=std::numeric_limits<double>::quiet_NaN();
	EclipseGeoCoord sun_geo;
	EclipseGeoCoord moon_geo;
	EclipseLibration lib;
	EclipsePointMeta p1_meta;
	EclipsePointMeta u1_meta;
	EclipsePointMeta u2_meta;
	EclipsePointMeta max_meta;
	EclipsePointMeta u3_meta;
	EclipsePointMeta u4_meta;
	EclipsePointMeta p4_meta;
	EclipsePointMeta opp_meta;
};

enum class LunarEclipseCalcMethod{
	Modern,
	Legacy,
};

void set_lunar_eclipse_calc_method(LunarEclipseCalcMethod method);
LunarEclipseCalcMethod get_lunar_eclipse_calc_method();
bool parse_lunar_eclipse_calc_method(const std::string&value,
									 LunarEclipseCalcMethod*out);
const char*lunar_eclipse_calc_method_name(LunarEclipseCalcMethod method);

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
