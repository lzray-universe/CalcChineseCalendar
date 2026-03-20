#pragma once

#include<cstddef>
#include<cstdint>

namespace vsop87a{

constexpr double kJ2000=2451545.0;
constexpr double kDaysPerMillennium=365250.0;
constexpr int kCoordinateCount=3;
constexpr int kOrderCount=6;

struct Term{
	int8_t a[12];
	double A;
	double B;
	double C;
};

struct Series{
	const Term*terms;
	std::uint32_t count;
};

struct BodyData{
	const char*name;
	Series coord[kCoordinateCount][kOrderCount];
};

enum class Body{
	Mercury,
	Venus,
	Earth,
	Mars,
	Jupiter,
	Saturn,
	Uranus,
	Neptune,
	EarthMoonBarycenter
};

const BodyData&GetBodyData(Body body);

void EvaluateXYZ(Body body,double jd_tdb,double out_xyz_au[3],
				 double out_vxyz_au_per_day[3]);

}
