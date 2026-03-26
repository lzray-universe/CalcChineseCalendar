#include "lunar/solar_zodiac.hpp"

#include<algorithm>
#include<cmath>
#include<stdexcept>
#include<utility>

#include "lunar/time_scale.hpp"

namespace{

constexpr double kSignWidth=TWO_PI/12.0;

double norm2pi(double angle){
	double v=std::fmod(angle,TWO_PI);
	if(v<0.0){
		v+=TWO_PI;
	}
	return v;
}

double year_start_jd_utc(int year,int tz_off){
	return greg2jd(year,1,1,0,0,0.0)-static_cast<double>(tz_off)/1440.0;
}

std::vector<SolarZodiacBoundary> build_boundaries(EphRead&eph,int year_from,
												  int year_to){
	SolLunCal solver(eph);
	std::vector<SolarZodiacBoundary> out;
	out.reserve(static_cast<std::size_t>(year_to-year_from+1)*
				solar_zodiac_defs().size());
	for(int year=year_from;year<=year_to;++year){
		for(const auto&def : solar_zodiac_defs()){
			LocalDT dt=solver.find_st(def.term_code,year);
			out.push_back({def.index,def.code,def.term_code,dt.toUtcJD()});
		}
	}
	std::sort(out.begin(),out.end(),[](const SolarZodiacBoundary&a,
									   const SolarZodiacBoundary&b){
		return a.jd_utc<b.jd_utc;
	});
	out.erase(std::unique(out.begin(),out.end(),
						  [](const SolarZodiacBoundary&a,
							 const SolarZodiacBoundary&b){
			return a.sign_index==b.sign_index&&
				   std::fabs(a.jd_utc-b.jd_utc)<=JD_EPSILON;
		}),
			  out.end());
	return out;
}

std::pair<std::size_t,std::size_t>
locate_interval(const std::vector<SolarZodiacBoundary>&boundaries,
				double jd_utc){
	auto it=std::upper_bound(boundaries.begin(),boundaries.end(),jd_utc,
							 [](double value,const SolarZodiacBoundary&item){
		return value<item.jd_utc;
	});
	if(it==boundaries.begin()||it==boundaries.end()){
		throw std::runtime_error(
			"failed to locate solar zodiac interval around target time");
	}
	std::size_t next_idx=static_cast<std::size_t>(it-boundaries.begin());
	return {next_idx-1,next_idx};
}

}

const std::array<SolarZodiacDef,12>&solar_zodiac_defs(){
	static const std::array<SolarZodiacDef,12> kDefs={{
		{0,"aries","Z2",0.0,PI/6.0},
		{1,"taurus","Z3",PI/6.0,2.0*PI/6.0},
		{2,"gemini","Z4",2.0*PI/6.0,3.0*PI/6.0},
		{3,"cancer","Z5",3.0*PI/6.0,4.0*PI/6.0},
		{4,"leo","Z6",4.0*PI/6.0,5.0*PI/6.0},
		{5,"virgo","Z7",5.0*PI/6.0,PI},
		{6,"libra","Z8",PI,7.0*PI/6.0},
		{7,"scorpio","Z9",7.0*PI/6.0,8.0*PI/6.0},
		{8,"sagittarius","Z10",8.0*PI/6.0,9.0*PI/6.0},
		{9,"capricorn","Z11",9.0*PI/6.0,10.0*PI/6.0},
		{10,"aquarius","Z12",10.0*PI/6.0,11.0*PI/6.0},
		{11,"pisces","Z1",11.0*PI/6.0,TWO_PI},
	}};
	return kDefs;
}

const SolarZodiacDef&solar_zodiac_def(int index){
	const auto&defs=solar_zodiac_defs();
	if(index<0||index>=static_cast<int>(defs.size())){
		throw std::invalid_argument("solar zodiac index out of range");
	}
	return defs[static_cast<std::size_t>(index)];
}

int solar_zodiac_index(double lambda_rad){
	double lam=norm2pi(lambda_rad);
	int idx=static_cast<int>(std::floor(lam/kSignWidth));
	if(idx>=12){
		idx=0;
	}
	return idx;
}

SolarZodiacPoint calc_solar_zodiac_at(EphRead&eph,double jd_utc){
	SolarZodiacPoint out;
	out.jd_utc=jd_utc;
	out.jd_tdb=TimeScale::utc_to_tdb(jd_utc);

	AppLon app(eph);
	auto sun=app.sun_calc(out.jd_tdb);
	out.sun_lam_rad=sun.first;
	out.sun_lam_deg=sun.first*180.0/PI;
	out.sign_index=solar_zodiac_index(out.sun_lam_rad);

	const auto&def=solar_zodiac_def(out.sign_index);
	out.sign_code=def.code;
	out.term_code=def.term_code;

	int year=0;
	int month=0;
	int day=0;
	int hour=0;
	int minute=0;
	double second=0.0;
	jd2greg(jd_utc,year,month,day,hour,minute,second);

	std::vector<SolarZodiacBoundary> boundaries=
		build_boundaries(eph,year-1,year+1);
	auto interval=locate_interval(boundaries,jd_utc);
	const SolarZodiacBoundary&prev=boundaries[interval.first];
	const SolarZodiacBoundary&next=boundaries[interval.second];

	out.sign_start_jd_utc=prev.jd_utc;
	out.sign_end_jd_utc=next.jd_utc;
	out.span_sec=(out.sign_end_jd_utc-out.sign_start_jd_utc)*SEC_DAY;
	out.elapsed_sec=(jd_utc-out.sign_start_jd_utc)*SEC_DAY;
	out.remain_sec=(out.sign_end_jd_utc-jd_utc)*SEC_DAY;

	double offset=norm2pi(out.sun_lam_rad-def.start_lambda_rad);
	if(offset>kSignWidth&&offset-kSignWidth<=1e-12){
		offset=kSignWidth;
	}
	out.sign_offset_rad=offset;
		}

		SolarZodiacYearInterval item;
		item.sign_index=cur.sign_index;
		item.sign_code=cur.sign_code;
		item.term_code=cur.term_code;
		item.sign_start_jd_utc=cur.jd_utc;
		item.sign_end_jd_utc=next.jd_utc;
		item.in_year_start_jd_utc=in_year_start;
		item.in_year_end_jd_utc=in_year_end;
		item.in_year_dur_sec=(in_year_end-in_year_start)*SEC_DAY;
		item.clipped_start=cur.jd_utc+JD_EPSILON<out.year_start_jd_utc;
		item.clipped_end=next.jd_utc-JD_EPSILON>out.year_end_jd_utc;
		out.intervals.push_back(std::move(item));
	}

	if(out.intervals.empty()){
		throw std::runtime_error("failed to build solar zodiac year summary");
	}
	return out;
}
