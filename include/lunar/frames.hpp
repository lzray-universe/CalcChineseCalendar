#pragma once

#include<utility>

#include "lunar/math.hpp"

enum class PrecModel{ AUTO,IAU2006,VONDRAK };

extern const PrecModel PREC_MODEL;
extern const double LONG_THR;

struct CoordTf{
	static Mat3 R1(double angle);

	static Mat3 R3(double angle);

	static Mat3 bias_mat();
};

struct PrecNut{
	static Mat3 prec_mat(double jd_tdb);

	static double mean_obl(double jd_tdb);

	static std::pair<double,double> nut_ang(double jd_tdb);

	static Mat3 nut_mat(double jd_tdb);

	// True equator/equinox of date to ITRS, including celestial-pole offsets
	// and polar motion whenever the bundled EOP snapshot covers the epoch.
	static Mat3 earth_rot(double jd_tdb);
};
