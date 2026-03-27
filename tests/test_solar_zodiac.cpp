#include<gtest/gtest.h>

#include<array>

#include "lunar/solar_zodiac.hpp"
#include "test_common.hpp"

namespace{

constexpr double kZodiacAngleTolDeg=0.1;
constexpr double kZodiacTimeTolSec=15.0*60.0;

double cst_to_utc_jd(int year,int month,int day,int hour=0,int minute=0,
					 double second=0.0){
	return greg2jd(year,month,day,hour,minute,second)-UTC8DAY;
}

void expect_close_jd(double lhs,double rhs,double tol_sec){
	EXPECT_NEAR(lhs,rhs,tol_sec/SEC_DAY);
}

}

TEST(SeriesVsBspSolarZodiac, PointSamplesTrackReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::string ref_bsp=reference_bsp();
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);

	const std::array<double,6> samples={
		cst_to_utc_jd(2025,3,21,12,0,0.0),
		cst_to_utc_jd(2025,4,20,0,0,0.0),
		cst_to_utc_jd(2025,6,21,12,0,0.0),
		cst_to_utc_jd(2025,8,23,12,0,0.0),
		cst_to_utc_jd(2025,10,23,12,0,0.0),
		cst_to_utc_jd(2025,11,22,12,0,0.0),
	};
	for(double jd_utc : samples){
		const SolarZodiacPoint series_point=
			calc_solar_zodiac_at(series_eph,jd_utc);
		const SolarZodiacPoint bsp_point=calc_solar_zodiac_at(bsp_eph,jd_utc);
		SCOPED_TRACE(jd_utc);
		EXPECT_EQ(series_point.sign_code,bsp_point.sign_code);
		EXPECT_EQ(series_point.term_code,bsp_point.term_code);
		EXPECT_NEAR(series_point.sun_lam_deg,bsp_point.sun_lam_deg,
					kZodiacAngleTolDeg);
		EXPECT_NEAR(series_point.sign_offset_deg,bsp_point.sign_offset_deg,
					kZodiacAngleTolDeg);
		expect_close_jd(series_point.sign_start_jd_utc,bsp_point.sign_start_jd_utc,
					   kZodiacTimeTolSec);
		expect_close_jd(series_point.sign_end_jd_utc,bsp_point.sign_end_jd_utc,
					   kZodiacTimeTolSec);
	}
#endif
}

TEST(SeriesVsBspSolarZodiac, YearIntervalsTrackReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::string ref_bsp=reference_bsp();
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);
	const SolarZodiacYearSummary series_year=
		calc_solar_zodiac_year(series_eph,2025,8*60);
	const SolarZodiacYearSummary bsp_year=
		calc_solar_zodiac_year(bsp_eph,2025,8*60);
	ASSERT_EQ(series_year.intervals.size(),bsp_year.intervals.size());

	for(std::size_t i=0;i<series_year.intervals.size();++i){
		const SolarZodiacYearInterval&series_item=series_year.intervals[i];
		const SolarZodiacYearInterval&bsp_item=bsp_year.intervals[i];
		SCOPED_TRACE(series_item.sign_code);
		EXPECT_EQ(series_item.sign_index,bsp_item.sign_index);
		EXPECT_EQ(series_item.sign_code,bsp_item.sign_code);
		EXPECT_EQ(series_item.term_code,bsp_item.term_code);
		EXPECT_EQ(series_item.clipped_start,bsp_item.clipped_start);
		EXPECT_EQ(series_item.clipped_end,bsp_item.clipped_end);
		expect_close_jd(series_item.sign_start_jd_utc,bsp_item.sign_start_jd_utc,
					   kZodiacTimeTolSec);
		expect_close_jd(series_item.sign_end_jd_utc,bsp_item.sign_end_jd_utc,
					   kZodiacTimeTolSec);
		expect_close_jd(series_item.in_year_start_jd_utc,
					   bsp_item.in_year_start_jd_utc,kZodiacTimeTolSec);
		expect_close_jd(series_item.in_year_end_jd_utc,bsp_item.in_year_end_jd_utc,
					   kZodiacTimeTolSec);
		EXPECT_NEAR(series_item.in_year_dur_sec,bsp_item.in_year_dur_sec,
					kZodiacTimeTolSec);
	}
#endif
}
