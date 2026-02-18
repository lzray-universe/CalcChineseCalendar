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
};
