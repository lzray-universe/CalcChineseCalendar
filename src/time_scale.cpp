#include "lunar/time_scale.hpp"

#include<cmath>

namespace{

static double delta53(double year){
	double t=(year-1825.0)/100.0;
	double base=-150.568+31.4115*t*t+284.8436*std::cos(2.0*PI*(t+0.75)/14.0);
	double corr=0.1056*(std::pow(year/100.0-19.55,2)-0.49);
	return base+corr;
}

}

double TimeScale::tdb_to_tt(double jd_tdb){ return jd_tdb; }

double TimeScale::tt_to_tdb(double jd_tt){ return jd_tt; }

double TimeScale::tt_to_tai(double jd_tt){ return jd_tt-32.184/SEC_DAY; }

int TimeScale::leap_sec(double jd_utc){
	struct Entry{
		double jd;
		int leaps;
	};
	static const Entry table[]={
		{2441317.5,10},
		{2441499.5,11},
		{2441683.5,12},
		{2442048.5,13},
		{2442413.5,14},
		{2442778.5,15},
		{2443144.5,16},
		{2443509.5,17},
		{2443874.5,18},
		{2444239.5,19},
		{2444786.5,20},
		{2445151.5,21},
		{2445516.5,22},
		{2446247.5,23},
		{2447161.5,24},
		{2447892.5,25},
		{2448257.5,26},
		{2448804.5,27},
		{2449169.5,28},
		{2449534.5,29},
		{2450083.5,30},
		{2450630.5,31},
		{2451179.5,32},
		{2453736.5,33},
		{2454832.5,34},
		{2456109.5,35},
		{2457204.5,36},
		{2457754.5,37},
	};
	int leaps=0;
	for(const auto&e : table){
		if(jd_utc>=e.jd){
			leaps=e.leaps;
		}else{
			break;
		}
	}
	return leaps;
}

double TimeScale::deltayr(double year){
	double t=(year-1825.0)/100.0;
	return -150.568+31.4115*t*t+284.8436*std::cos(2.0*PI*(t+0.75)/14.0);
}

double TimeScale::tdb_to_utc(double jd_tdb){
	double jd_tt=tdb_to_tt(jd_tdb);
	double year=2000.0+(jd_tt-2451544.5)/365.2425;

	if(year<1970.0||year>2026.0){
		double delta_t=delta53(year);
		double jd_ut1=jd_tt-delta_t/SEC_DAY;
		return jd_ut1;
	}

	if(year<1972.0){
		double delta_t=deltayr(year);
		double jd_ut1=jd_tt-delta_t/SEC_DAY;
		return jd_ut1;
	}

	double jd_tai=jd_tt-32.184/SEC_DAY;
	double jd_utc=jd_tai;
	for(int i=0;i<2;++i){
		int leaps=leap_sec(jd_utc);
		jd_utc=jd_tai-static_cast<double>(leaps)/SEC_DAY;
	}
	return jd_utc;
}

double TimeScale::utc_to_tdb(double jd_utc){
	double year=2000.0+(jd_utc-2451544.5)/365.2425;

	if(year<1970.0||year>2026.0){
		double delta_t=delta53(year);
		double jd_tdb=jd_utc+delta_t/SEC_DAY;
		return jd_tdb;
	}

	if(year<1972.0){
		double delta_t=deltayr(year);
		double jd_tdb=jd_utc+delta_t/SEC_DAY;
		return jd_tdb;
	}

	int leaps=leap_sec(jd_utc);
	double jd_tt=jd_utc+(static_cast<double>(leaps)+32.184)/SEC_DAY;
	double jd_tdb=tt_to_tdb(jd_tt);
	return jd_tdb;
}
