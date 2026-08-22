#include "lunar/time_scale.hpp"

#include<algorithm>
#include<array>
#include<cmath>
#include<cstddef>
#include<iterator>
#include<limits>

namespace{

struct DeltaTSpline{
	double start_year;
	double end_year;
	double a0;
	double a1;
	double a2;
	double a3;
};

struct DeltaTSample{
	double jd;
	double seconds;
};

struct DeltaTYearSample{
	double year;
	double seconds;
};

#include "delta_t_data.inc"

constexpr int kMonthlyFirstYear=1973;
constexpr int kMonthlyFirstMonth=2;

double decimal_year(double jd){
	int year=0;
	int month=0;
	int day=0;
	int hour=0;
	int minute=0;
	double second=0.0;
	jd2greg(jd,year,month,day,hour,minute,second);
	double y0=greg2jd(year,1,1,0,0,0.0);
	double y1=greg2jd(year+1,1,1,0,0,0.0);
	return static_cast<double>(year)+(jd-y0)/(y1-y0);
}

double jd_from_decimal_year(double year){
	if(!std::isfinite(year)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	int whole=static_cast<int>(std::floor(year));
	double fraction=year-static_cast<double>(whole);
	double y0=greg2jd(whole,1,1,0,0,0.0);
	double y1=greg2jd(whole+1,1,1,0,0,0.0);
	return y0+fraction*(y1-y0);
}

double eval_smh_spline(double year){
	auto it=std::find_if(kSmh2020Splines.begin(),kSmh2020Splines.end(),
		[&](const DeltaTSpline&s){ return year<=s.end_year; });
	if(it==kSmh2020Splines.end()){
		it=std::prev(kSmh2020Splines.end());
	}
	double u=(year-it->start_year)/(it->end_year-it->start_year);
	return ((it->a3*u+it->a2)*u+it->a1)*u+it->a0;
}

void monthly_year_month(std::size_t index,int&year,int&month){
	std::size_t total=static_cast<std::size_t>(kMonthlyFirstMonth-1)+index;
	year=kMonthlyFirstYear+static_cast<int>(total/12U);
	month=static_cast<int>(total%12U)+1;
}

double monthly_jd(std::size_t index){
	int year=0;
	int month=0;
	monthly_year_month(index,year,month);
	return greg2jd(year,month,1,0,0,0.0);
}

bool eval_usno_monthly(double jd,double&seconds){
	const double first_jd=monthly_jd(0U);
	const double last_jd=monthly_jd(kUsnoMonthlyDeltaT.size()-1U);
	if(jd<first_jd||jd>last_jd){
		return false;
	}

	int year=0;
	int month=0;
	int day=0;
	int hour=0;
	int minute=0;
	double second=0.0;
	jd2greg(jd,year,month,day,hour,minute,second);
	long index=static_cast<long>(year-kMonthlyFirstYear)*12L+
				 static_cast<long>(month-kMonthlyFirstMonth);
	if(index<0){
		return false;
	}
	std::size_t i=static_cast<std::size_t>(index);
	if(i>=kUsnoMonthlyDeltaT.size()-1U){
		seconds=kUsnoMonthlyDeltaT.back();
		return true;
	}

	double left=monthly_jd(i);
	double right=monthly_jd(i+1U);
	double u=(jd-left)/(right-left);
	seconds=kUsnoMonthlyDeltaT[i]+
			u*(kUsnoMonthlyDeltaT[i+1U]-kUsnoMonthlyDeltaT[i]);
	return std::isfinite(seconds);
}

bool eval_usno_atomic(double jd,double year,double&seconds){
	const auto&first=kUsnoAtomicDeltaT.front();
	const auto&last=kUsnoAtomicDeltaT.back();
	double first_monthly_jd=monthly_jd(0U);
	if(year<1960.0||jd>=first_monthly_jd){
		return false;
	}

	if(year<first.year){
		// Join the smoothed historical reconstruction to the atomic record over
		// two years without inventing a discontinuity in Earth rotation.
		double spline=eval_smh_spline(year);
		double slope=(kUsnoAtomicDeltaT[1].seconds-first.seconds)/
					 (kUsnoAtomicDeltaT[1].year-first.year);
		double atomic=first.seconds+(year-first.year)*slope;
		double u=(year-1960.0)/(first.year-1960.0);
		u=std::max(0.0,std::min(1.0,u));
		double smooth=u*u*(3.0-2.0*u);
		seconds=spline+smooth*(atomic-spline);
		return true;
	}

	for(std::size_t i=1U;i<kUsnoAtomicDeltaT.size();++i){
		const auto&right=kUsnoAtomicDeltaT[i];
		if(year<=right.year){
			const auto&left=kUsnoAtomicDeltaT[i-1U];
			double u=(year-left.year)/(right.year-left.year);
			seconds=left.seconds+u*(right.seconds-left.seconds);
			return true;
		}
	}

	// Bridge the short 1973-01 interval to the first monthly determination.
	double first_monthly_year=decimal_year(first_monthly_jd);
	double u=(year-last.year)/(first_monthly_year-last.year);
	seconds=last.seconds+u*(kUsnoMonthlyDeltaT.front()-last.seconds);
	return true;
}

bool eval_usno_prediction(double jd,double&seconds){
	const double observed_jd=monthly_jd(kUsnoMonthlyDeltaT.size()-1U);
	if(jd<=observed_jd||jd>kUsnoPredictedDeltaT.back().jd){
		return false;
	}

	DeltaTSample left{observed_jd,kUsnoMonthlyDeltaT.back()};
	for(const auto&right:kUsnoPredictedDeltaT){
		if(right.jd<=observed_jd){
			continue;
		}
		if(jd<=right.jd){
			double u=(jd-left.jd)/(right.jd-left.jd);
			seconds=left.seconds+u*(right.seconds-left.seconds);
			return std::isfinite(seconds);
		}
		left=right;
	}
	return false;
}

double smh_long_term_past(double year){
	// Stephenson, Morrison & Hohenkerk (2016), equation 4.1. The constant
	// offset makes the extrapolation continuous with Table S15.2020 at -720.
	auto raw=[](double y){
		double u=(y-1825.0)/100.0;
		return -320.0+32.5*u*u;
	};
	static const double offset=eval_smh_spline(-720.0)-raw(-720.0);
	return raw(year)+offset;
}

double long_term_future(double year){
	// Integrated long-term LOD model from Stephenson et al. (2016) and
	// Morrison et al. (2021), including the approximately 1,400-year term.
	auto raw=[](double y){
		double t=(y-1825.0)/100.0;
		return 31.4115*t*t+
			   284.8436*std::cos(2.0*PI*(t+0.75)/14.0);
	};
	static const double last_year=decimal_year(kUsnoPredictedDeltaT.back().jd);
	static const double offset=kUsnoPredictedDeltaT.back().seconds-raw(last_year);
	return raw(year)+offset;
}

double delta_t_from_jd(double jd_tt){
	if(!std::isfinite(jd_tt)){
		return std::numeric_limits<double>::quiet_NaN();
	}

	double seconds=0.0;
	if(eval_usno_monthly(jd_tt,seconds)){
		return seconds;
	}
	if(eval_usno_prediction(jd_tt,seconds)){
		return seconds;
	}

	double year=decimal_year(jd_tt);
	if(eval_usno_atomic(jd_tt,year,seconds)){
		return seconds;
	}
	if(year>=kSmh2020Splines.front().start_year&&
	   year<=kSmh2020Splines.back().end_year){
		return eval_smh_spline(year);
	}
	if(year<kSmh2020Splines.front().start_year){
		return smh_long_term_past(year);
	}
	return long_term_future(year);
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
	for(const auto&e:table){
		if(jd_utc>=e.jd){
			leaps=e.leaps;
		}else{
			break;
		}
	}
	return leaps;
}

double TimeScale::delta_t_seconds(double jd_tt){
	return delta_t_from_jd(jd_tt);
}

double TimeScale::deltayr(double year){
	return delta_t_from_jd(jd_from_decimal_year(year));
}

double TimeScale::tdb_to_ut1(double jd_tdb){
	double jd_tt=tdb_to_tt(jd_tdb);
	return jd_tt-delta_t_seconds(jd_tt)/SEC_DAY;
}

double TimeScale::ut1_to_tdb(double jd_ut1){
	double jd_tt=jd_ut1;
	for(int i=0;i<4;++i){
		jd_tt=jd_ut1+delta_t_seconds(jd_tt)/SEC_DAY;
	}
	return tt_to_tdb(jd_tt);
}

double TimeScale::tdb_to_utc(double jd_tdb){
	double jd_tt=tdb_to_tt(jd_tdb);
	double year=decimal_year(jd_tt);
	if(year<1972.0){
		// Before the leap-second era the library's civil-time approximation is
		// UT1, matching the convention used by historical eclipse catalogs.
		return tdb_to_ut1(jd_tdb);
	}

	double jd_tai=tt_to_tai(jd_tt);
	double jd_utc=jd_tai;
	for(int i=0;i<3;++i){
		jd_utc=jd_tai-static_cast<double>(leap_sec(jd_utc))/SEC_DAY;
	}
	return jd_utc;
}

double TimeScale::utc_to_tdb(double jd_utc){
	double year=decimal_year(jd_utc);
	if(year<1972.0){
		return ut1_to_tdb(jd_utc);
	}

	int leaps=leap_sec(jd_utc);
	double jd_tt=jd_utc+(static_cast<double>(leaps)+32.184)/SEC_DAY;
	return tt_to_tdb(jd_tt);
}
