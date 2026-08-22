#pragma once

#include<array>
#include<limits>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/events.hpp"
#include "lunar/spc_ephem.hpp"

struct SolarBesselianElements{
	bool has=false;
	double jd_tdb_epoch=std::numeric_limits<double>::quiet_NaN();
	double x=std::numeric_limits<double>::quiet_NaN();
	double y=std::numeric_limits<double>::quiet_NaN();
	double d_deg=std::numeric_limits<double>::quiet_NaN();
	double mu_deg=std::numeric_limits<double>::quiet_NaN();
	double l1=std::numeric_limits<double>::quiet_NaN();
	double l2=std::numeric_limits<double>::quiet_NaN();
	double tan_f1=std::numeric_limits<double>::quiet_NaN();
	double tan_f2=std::numeric_limits<double>::quiet_NaN();
	double x_dot=std::numeric_limits<double>::quiet_NaN();
	double y_dot=std::numeric_limits<double>::quiet_NaN();
	double d_dot_deg=std::numeric_limits<double>::quiet_NaN();
	double mu_dot_deg=std::numeric_limits<double>::quiet_NaN();
	double l1_dot=std::numeric_limits<double>::quiet_NaN();
	double l2_dot=std::numeric_limits<double>::quiet_NaN();
	std::array<double,4> x_coeff{};
	std::array<double,4> y_coeff{};
	std::array<double,4> d_coeff_deg{};
	std::array<double,4> mu_coeff_deg{};
	std::array<double,4> l1_coeff{};
	std::array<double,4> l2_coeff{};
};

struct SolarEclipse{
	bool has=false;
	std::string type="N";
	double jd_tdb_c1=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_c2=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_max=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_c3=std::numeric_limits<double>::quiet_NaN();
	double jd_tdb_c4=std::numeric_limits<double>::quiet_NaN();
	double mag=std::numeric_limits<double>::quiet_NaN();
	double obscuration=std::numeric_limits<double>::quiet_NaN();
	double gamma=std::numeric_limits<double>::quiet_NaN();
	double sep_max_deg=std::numeric_limits<double>::quiet_NaN();
	double sun_sd_max_deg=std::numeric_limits<double>::quiet_NaN();
	double moon_sd_max_deg=std::numeric_limits<double>::quiet_NaN();
	double moon_dist_km=std::numeric_limits<double>::quiet_NaN();
	double sun_dist_km=std::numeric_limits<double>::quiet_NaN();
	double dt_max_sec=std::numeric_limits<double>::quiet_NaN();
	double rp_re=std::numeric_limits<double>::quiet_NaN();
	double ru_re=std::numeric_limits<double>::quiet_NaN();
	SolarBesselianElements besselian;
};

bool calc_solar_eclipse(EphRead&eph,double jd_tdb_near_new_moon,SolarEclipse*out);

bool calc_solar_eclipse_from_max(EphRead&eph,double jd_tdb_max,SolarEclipse*out);

// Recompute only the catalog magnitude and obscuration at an already known
// greatest-eclipse instant. This avoids repeating contact searches and
// Besselian polynomial generation when refreshing an eclipse catalog.
bool calc_solar_eclipse_magnitude_from_max(EphRead&eph,double jd_tdb_max,
										   const std::string&type,double*mag,
										   double*obscuration);

std::vector<EventRec> bld_solar_eclipse_events(EphRead&eph,
												const YearResult&yr,
												int tz_off);

bool solar_eclipse_window_tdb(const SolarEclipse&ecl,
							  const std::string&stage_window,
							  double*jd_tdb_start,double*jd_tdb_end);

struct SolarEclipsePointVis{
	std::string stage_window="any";
	double lat_deg=0.0;
	double lon_deg=0.0;
	double height_m=0.0;
	bool has_eclipse=false;
	bool visible=false;
	bool central=false;
	double max_mag=std::numeric_limits<double>::quiet_NaN();
	double max_obscuration=std::numeric_limits<double>::quiet_NaN();
	double max_sun_alt_deg=std::numeric_limits<double>::quiet_NaN();
	double first_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double last_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double c1_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double c2_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double max_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double c3_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double c4_jd_utc=std::numeric_limits<double>::quiet_NaN();
	int sample_count=0;
};

struct SolarEclipseGlobalPoint{
	double lat_deg=0.0;
	double lon_deg=0.0;
	double max_mag=std::numeric_limits<double>::quiet_NaN();
	double max_sun_alt_deg=std::numeric_limits<double>::quiet_NaN();
	double first_jd_utc=std::numeric_limits<double>::quiet_NaN();
	double last_jd_utc=std::numeric_limits<double>::quiet_NaN();
};

struct SolarEclipseGlobalVis{
	std::string stage_window="any";
	double jd_start_utc=std::numeric_limits<double>::quiet_NaN();
	double jd_end_utc=std::numeric_limits<double>::quiet_NaN();
	double lat_step_deg=0.0;
	double lon_step_deg=0.0;
	int sample_count=0;
	std::vector<SolarEclipseGlobalPoint> points;
};

bool solar_eclipse_point_visibility(EphRead&eph,const SolarEclipse&ecl,
									const std::string&stage_window,
									double lat_deg,double lon_deg,double height_m,
									double sample_minutes,bool refine_edge,
									SolarEclipsePointVis*out);

bool solar_eclipse_global_visibility(EphRead&eph,const SolarEclipse&ecl,
									 const std::string&stage_window,
									 double lat_step_deg,double lon_step_deg,
									 double sample_minutes,
									 SolarEclipseGlobalVis*out);
