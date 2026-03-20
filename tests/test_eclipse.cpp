#include<gtest/gtest.h>

#include "lunar/lunar_eclipse.hpp"
#include "lunar/solar_eclipse.hpp"
#include "lunar/time_scale.hpp"

#include "test_common.hpp"

TEST(LunarEclipseSeries, TotalEclipseRegression){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	EphRead eph(test_ephem());
	const double jd_utc=greg2jd(2025,9,7,0,0,0.0)-UTC8DAY;
	const double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	LunarEclipse ecl;
	ASSERT_TRUE(calc_lunar_eclipse(eph,jd_tdb,&ecl));
	ASSERT_TRUE(ecl.has);
	EXPECT_EQ(ecl.type,"T");
	EXPECT_LT(ecl.jd_tdb_p1,ecl.jd_tdb_max);
	EXPECT_LT(ecl.jd_tdb_max,ecl.jd_tdb_p4);
	EXPECT_LT(ecl.jd_tdb_u1,ecl.jd_tdb_u2);
	EXPECT_LT(ecl.jd_tdb_u2,ecl.jd_tdb_max);
	EXPECT_LT(ecl.jd_tdb_max,ecl.jd_tdb_u3);
	EXPECT_LT(ecl.jd_tdb_u3,ecl.jd_tdb_u4);
}

TEST(SolarEclipseSeries, TotalEclipseRegression){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	EphRead eph(test_ephem());
	const double jd_utc=greg2jd(2026,8,12,0,0,0.0)-UTC8DAY;
	const double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	SolarEclipse ecl;
	ASSERT_TRUE(calc_solar_eclipse(eph,jd_tdb,&ecl));
	ASSERT_TRUE(ecl.has);
	EXPECT_EQ(ecl.type,"T");
	EXPECT_LT(ecl.jd_tdb_c1,ecl.jd_tdb_c2);
	EXPECT_LT(ecl.jd_tdb_c2,ecl.jd_tdb_max);
	EXPECT_LT(ecl.jd_tdb_max,ecl.jd_tdb_c3);
	EXPECT_LT(ecl.jd_tdb_c3,ecl.jd_tdb_c4);
}
