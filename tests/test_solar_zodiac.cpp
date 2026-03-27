#include<gtest/gtest.h>

#include<cstdlib>
#include<filesystem>

#include "lunar/solar_zodiac.hpp"

namespace{

std::string zodiac_ref_bsp(){
	const char*raw=std::getenv("LUNAR_TEST_BSP");
	if(raw!=nullptr&&*raw!='\0'){
		std::error_code ec;
		if(std::filesystem::exists(raw,ec)&&!ec){
			return raw;
		}
	}

	const std::filesystem::path repo_bsp="de442.bsp";
	if(std::filesystem::exists(repo_bsp)){
		return repo_bsp.string();
	}
	return "";
}

}

TEST(SolarZodiacSeries, TracksBspReference){
	const std::string ref_bsp=zodiac_ref_bsp();
	if(ref_bsp.empty()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or repo-local de442.bsp";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);

	const double jd_utc=greg2jd(2025,4,20,0,0,0.0)-UTC8DAY;
	const SolarZodiacPoint series_point=calc_solar_zodiac_at(series_eph,jd_utc);
	const SolarZodiacPoint bsp_point=calc_solar_zodiac_at(bsp_eph,jd_utc);
	EXPECT_NEAR(series_point.sun_lam_deg,bsp_point.sun_lam_deg,0.1);
	EXPECT_NEAR(series_point.sign_end_jd_utc,bsp_point.sign_end_jd_utc,0.1);

	const SolarZodiacYearSummary series_year=
		calc_solar_zodiac_year(series_eph,2025,8*60);
	const SolarZodiacYearSummary bsp_year=
		calc_solar_zodiac_year(bsp_eph,2025,8*60);
	ASSERT_GE(series_year.intervals.size(),4U);
	ASSERT_GE(bsp_year.intervals.size(),4U);
	EXPECT_EQ(series_year.intervals[3].sign_code,"aries");
	EXPECT_EQ(bsp_year.intervals[3].sign_code,"aries");
	EXPECT_NEAR(series_year.intervals[3].sign_end_jd_utc,
				bsp_year.intervals[3].sign_end_jd_utc,0.1);
#endif
}
