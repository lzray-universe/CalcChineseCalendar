#pragma once

#include<utility>

#include "lunar/frames.hpp"
#include "lunar/spc_ephem.hpp"

struct AberState{
	Pos3 X;
	Vel3 V;
	double tr;
	double tr_rate=1.0;
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
	static double lightday(const Pos3&vec);

	// Same-epoch geometric state: target(t)-Earth(t).
	static AberState geo_geom_state(EphRead&eph,int target,double jd_tdb);

	// Converged one-way light-time state: target(t_emit)-Earth(t_receive),
	// without gravitational deflection or observer aberration.
	static AberState geo_lt_state(EphRead&eph,int target,double jd_tdb,
								 int max_iter=6);

	// Apparent state: light time, solar gravitational deflection and
	// reception-epoch observer aberration, in that order.
	static AberState geo_app_state(EphRead&eph,int target,double jd_tdb,
								  int max_iter=6);

	static Pos3 geo_app(EphRead&eph,int target,double jd_tdb,double*tr_out,
						int max_iter=3);

	static Pos3 geo_app(EphRead&eph,int target,double jd_tdb,int max_iter=3);
};

struct AppLon{
	EphRead&eph;

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

	double body_lam_app(int target,double jd_tdb);

	std::pair<double,double> sun_calc(double jd_tdb);

	std::pair<double,double> moon_calc(double jd_tdb);

	EoTData eot_calc(double jd_utc,double lon_deg);
};
