#pragma once

#include "lunar/math.hpp"

struct TimeScale{
	static double tdb_to_tt(double jd_tdb);
	static double tt_to_tdb(double jd_tt);

	static double tt_to_tai(double jd_tt);

	static int leap_sec(double jd_utc);

	static double deltayr(double year);

	static double tdb_to_utc(double jd_tdb);

	static double utc_to_tdb(double jd_utc);
};
