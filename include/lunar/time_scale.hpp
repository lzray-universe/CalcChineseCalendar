#pragma once

#include "lunar/math.hpp"

struct EarthOrientation{
	bool available=false;
	bool predicted=false;
	double ut1_utc_seconds=0.0;
	double xp_arcsec=0.0;
	double yp_arcsec=0.0;
	double dx_mas=0.0;
	double dy_mas=0.0;
};

struct TimeScale{
	static double tdb_to_tt(double jd_tdb);
	static double tt_to_tdb(double jd_tt);

	static double tt_to_tai(double jd_tt);

	static int leap_sec(double jd_utc);

	// Delta T = TT - UT1 in SI seconds. Observations are preferred over
	// historical splines, official predictions and long-term extrapolation.
	static double delta_t_seconds(double jd_tt);

	// Compatibility entry point accepting a decimal Gregorian year.
	static double deltayr(double year);

	// Interpolated rapid/final Earth-orientation parameters when the bundled
	// daily snapshot covers jd_utc. Values outside that finite window are
	// explicitly reported unavailable rather than silently treated as zero.
	static EarthOrientation earth_orientation(double jd_utc);

	static double tdb_to_ut1(double jd_tdb);
	static double ut1_to_tdb(double jd_ut1);

	static double tdb_to_utc(double jd_tdb);

	static double utc_to_tdb(double jd_utc);
};
