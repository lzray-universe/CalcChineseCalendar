#pragma once

#include<limits>
#include<string>

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
