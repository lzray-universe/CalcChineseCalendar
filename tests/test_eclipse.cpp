#include<gtest/gtest.h>

#include "lunar/lunar_eclipse.hpp"
#include "lunar/solar_eclipse.hpp"
#include "lunar/time_scale.hpp"

#include "test_common.hpp"

#include<cmath>

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
	if(is_series_ephem(test_ephem())){
		EXPECT_NEAR(ecl.lib.l_deg,-4.0686082308,1e-5);
		EXPECT_NEAR(ecl.lib.b_deg,0.3789342709,1e-5);
		EXPECT_NEAR(ecl.lib.c_deg,-21.2439672118,1e-5);
	}
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
	ASSERT_TRUE(ecl.besselian.has);
	EXPECT_NEAR(ecl.besselian.jd_tdb_epoch,ecl.jd_tdb_max,1e-12);
	EXPECT_NEAR(ecl.besselian.l1,ecl.rp_re,1e-12);
	EXPECT_NEAR(ecl.besselian.l2,ecl.ru_re,1e-12);
	EXPECT_NEAR(ecl.besselian.x_coeff[0],ecl.besselian.x,1e-12);
	EXPECT_NEAR(ecl.besselian.y_coeff[0],ecl.besselian.y,1e-12);
	auto axis_distance2=[&](double hour){
		auto eval=[&](const std::array<double,4>&coeff){
			return ((coeff[3]*hour+coeff[2])*hour+coeff[1])*hour+coeff[0];
		};
		double x=eval(ecl.besselian.x_coeff);
		double y=eval(ecl.besselian.y_coeff);
		return x*x+y*y;
	};
	constexpr double kTenSecondsInHours=10.0/3600.0;
	EXPECT_LT(axis_distance2(0.0),axis_distance2(-kTenSecondsInHours));
	EXPECT_LT(axis_distance2(0.0),axis_distance2(kTenSecondsInHours));
	EXPECT_NEAR(std::hypot(ecl.besselian.x,ecl.besselian.y),
				std::fabs(ecl.gamma),1e-10);
	EXPECT_NEAR(ecl.jd_tdb_max,2461265.241032,10.0/86400.0);
	EXPECT_NEAR(ecl.mag,1.0386,2e-4);
	EXPECT_DOUBLE_EQ(ecl.obscuration,1.0);
	double fast_mag=0.0;
	double fast_obscuration=0.0;
	ASSERT_TRUE(calc_solar_eclipse_magnitude_from_max(
		eph,ecl.jd_tdb_max,ecl.type,&fast_mag,&fast_obscuration));
	EXPECT_DOUBLE_EQ(fast_mag,ecl.mag);
	EXPECT_DOUBLE_EQ(fast_obscuration,ecl.obscuration);
	EXPECT_TRUE(std::isfinite(ecl.besselian.tan_f1));
	EXPECT_TRUE(std::isfinite(ecl.besselian.tan_f2));

	SolarEclipse detail;
	ASSERT_TRUE(calc_solar_eclipse_from_max(eph,ecl.jd_tdb_max,&detail));
	EXPECT_DOUBLE_EQ(detail.jd_tdb_max,ecl.jd_tdb_max);
	EXPECT_DOUBLE_EQ(detail.gamma,ecl.gamma);
	EXPECT_DOUBLE_EQ(detail.besselian.jd_tdb_epoch,ecl.jd_tdb_max);

	const double annular_jd_utc=greg2jd(2026,2,17,0,0,0.0)-UTC8DAY;
	SolarEclipse annular;
	ASSERT_TRUE(calc_solar_eclipse(
		eph,TimeScale::utc_to_tdb(annular_jd_utc),&annular));
	EXPECT_EQ(annular.type,"A");
	EXPECT_NEAR(annular.mag,0.9630,1e-3);
	EXPECT_GT(annular.obscuration,0.0);
	EXPECT_LT(annular.obscuration,1.0);

	const double partial_jd_utc=greg2jd(2025,3,29,0,0,0.0)-UTC8DAY;
	SolarEclipse partial;
	ASSERT_TRUE(calc_solar_eclipse(
		eph,TimeScale::utc_to_tdb(partial_jd_utc),&partial));
	EXPECT_EQ(partial.type,"P");
	EXPECT_NEAR(partial.mag,0.9376,1.5e-3);
	EXPECT_GT(partial.obscuration,0.0);
	EXPECT_LT(partial.obscuration,1.0);
}
