#pragma once

#include<utility>

#include "lunar/frames.hpp"
#include "lunar/spc_ephem.hpp"

struct RetProp{
	Vec3 X;
	Vec3 V;
	double tr;
};

struct EoTData{
	double jd_utc=0.0;
	double jd_tdb=0.0;
	double lon_deg=0.0;
	double lon_rad=0.0;
	double apparent_solar_time_rad=0.0;
	double mean_solar_time_rad=0.0;
	double eot_rad=0.0;
	double eot_minutes=0.0;
	double eot_seconds=0.0;
};

struct AberCorr{
	static double lightday(const Vec3&vec);

	static RetProp geo_prop(EphRead&eph,int target,double jd_tdb,
							int max_iter=3);

	static Vec3 geo_app(EphRead&eph,int target,double jd_tdb,double*tr_out,
						int max_iter=3);

	static Vec3 geo_app(EphRead&eph,int target,double jd_tdb,int max_iter=3);
};

struct AppLon{
	EphRead&eph;
	Mat3 frame_bias;

	bool prec_ok;
	double prec_jd;
	Mat3 prec_cache;

	bool r1n_ok;
	double r1n_jd;
	Mat3 r1n_cache;

	bool rot_ok;
	double rot_jd;
	Mat3 rot_cache;

	explicit AppLon(EphRead&reader);

	static double epsA(double jd_tdb);

	Mat3 R1_eps_N(double jd_tdb);

	Mat3 prec_mat(double jd_tdb);

	Mat3 rot_mat(double jd_tdb);

	std::pair<double,double> sun_calc(double jd_tdb);

	std::pair<double,double> moon_calc(double jd_tdb);

	EoTData eot_calc(double jd_utc,double lon_deg);
};
