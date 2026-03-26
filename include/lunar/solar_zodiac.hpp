#pragma once

#include<array>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"

struct SolarZodiacDef{
	int index=0;
	const char*code="";
	const char*term_code="";
	double start_lambda_rad=0.0;
	double end_lambda_rad=0.0;
};

struct SolarZodiacBoundary{
	int sign_index=0;
	std::string sign_code;
	std::string term_code;
	double jd_utc=0.0;
};

struct SolarZodiacPoint{
	double jd_utc=0.0;
	double jd_tdb=0.0;
	double sun_lam_rad=0.0;
	double sun_lam_deg=0.0;
	double sign_offset_rad=0.0;
	double sign_offset_deg=0.0;
	int sign_index=0;
	std::string sign_code;
	std::string term_code;
	double sign_start_jd_utc=0.0;
	double sign_end_jd_utc=0.0;
	double elapsed_sec=0.0;
	double remain_sec=0.0;
	double span_sec=0.0;
};

struct SolarZodiacYearInterval{
	int sign_index=0;
	std::string sign_code;
	std::string term_code;
	double sign_start_jd_utc=0.0;
	double sign_end_jd_utc=0.0;
	double in_year_start_jd_utc=0.0;
	double in_year_end_jd_utc=0.0;
	double in_year_dur_sec=0.0;
	bool clipped_start=false;
	bool clipped_end=false;
};

struct SolarZodiacYearSummary{
	int year=0;
	int tz_off=0;
	double year_start_jd_utc=0.0;
	double year_end_jd_utc=0.0;
	std::vector<SolarZodiacYearInterval> intervals;
};

const std::array<SolarZodiacDef,12>&solar_zodiac_defs();

const SolarZodiacDef&solar_zodiac_def(int index);

int solar_zodiac_index(double lambda_rad);

SolarZodiacPoint calc_solar_zodiac_at(EphRead&eph,double jd_utc);

SolarZodiacYearSummary calc_solar_zodiac_year(EphRead&eph,int year,int tz_off);
