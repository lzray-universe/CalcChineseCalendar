#include<gtest/gtest.h>

#include "lunar/math.hpp"
#include "lunar/time_scale.hpp"

#include<cmath>

TEST(TimeScaleDeltaT, UsesPublishedSplineAndUsnoData){
	EXPECT_NEAR(TimeScale::delta_t_seconds(greg2jd(-720,1,1)),20371.848,1e-9);
	EXPECT_NEAR(TimeScale::delta_t_seconds(greg2jd(1600,1,1)),109.127,1e-9);
	EXPECT_NEAR(TimeScale::delta_t_seconds(greg2jd(1973,2,1)),43.4724,1e-9);
	EXPECT_NEAR(TimeScale::delta_t_seconds(greg2jd(2025,4,1)),69.1471,1e-9);
	EXPECT_NEAR(TimeScale::delta_t_seconds(greg2jd(2026,4,1)),69.1330,1e-9);
	EXPECT_NEAR(TimeScale::delta_t_seconds(2462319.5),69.83,1e-9);
}

TEST(TimeScaleDeltaT, InterpolatesMonthlyObservations){
	double left=greg2jd(2025,4,1);
	double right=greg2jd(2025,5,1);
	double middle=0.5*(left+right);
	EXPECT_NEAR(TimeScale::delta_t_seconds(middle),
				0.5*(69.1471+69.1542),1e-12);
}

TEST(TimeScaleDeltaT, ModelBoundariesAreContinuous){
	constexpr double one_second=1.0/SEC_DAY;
	const double boundaries[]={
		greg2jd(-720,1,1),
		greg2jd(1960,1,1),
		greg2jd(1962,1,1),
		greg2jd(1973,2,1),
		greg2jd(2026,4,1),
		2463871.5,
	};
	for(double jd:boundaries){
		double before=TimeScale::delta_t_seconds(jd-one_second);
		double after=TimeScale::delta_t_seconds(jd+one_second);
		EXPECT_LT(std::fabs(after-before),0.01)<<"JD "<<jd;
	}
}

TEST(TimeScaleDeltaT, Ut1AndUtcAreDistinctAndRoundTrip){
	double jd_utc=greg2jd(2025,4,1);
	double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	double jd_ut1=TimeScale::tdb_to_ut1(jd_tdb);
	double delta_t=TimeScale::delta_t_seconds(TimeScale::tdb_to_tt(jd_tdb));

	EXPECT_NEAR((jd_tdb-jd_ut1)*SEC_DAY,delta_t,5e-5);
	EXPECT_NEAR((jd_ut1-jd_utc)*SEC_DAY,69.184-delta_t,5e-5);
	EXPECT_NEAR(TimeScale::tdb_to_utc(jd_tdb),jd_utc,1e-12);
	EXPECT_NEAR(TimeScale::ut1_to_tdb(jd_ut1),jd_tdb,1e-12);
}

TEST(TimeScaleEop, UsesRecentDailyEarthOrientation){
	double jd_utc=greg2jd(2026,9,4);
	EarthOrientation eop=TimeScale::earth_orientation(jd_utc);
	ASSERT_TRUE(eop.available);
	EXPECT_TRUE(eop.predicted);
	EXPECT_NEAR(eop.ut1_utc_seconds,0.0008643,1e-12);
	EXPECT_NEAR(eop.xp_arcsec,0.207634,1e-12);
	EXPECT_NEAR(eop.yp_arcsec,0.337963,1e-12);
	EXPECT_NEAR(TimeScale::delta_t_seconds(jd_utc+69.184/SEC_DAY),
				69.1831357,1e-9);
}

TEST(TimeScaleEop, InterpolatesAndReportsFiniteCoverage){
	double jd0=greg2jd(2026,9,4);
	EarthOrientation mid=TimeScale::earth_orientation(jd0+0.5);
	ASSERT_TRUE(mid.available);
	EXPECT_NEAR(mid.ut1_utc_seconds,0.5*(0.0008643+0.0006443),1e-12);
	EXPECT_FALSE(TimeScale::earth_orientation(greg2jd(2025,9,4)).available);
}
