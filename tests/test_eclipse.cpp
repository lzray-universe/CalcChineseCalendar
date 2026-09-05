#include<gtest/gtest.h>

#include "lunar/lunar_eclipse.hpp"
#include "lunar/solar_eclipse.hpp"
#include "lunar/time_scale.hpp"

#include "test_common.hpp"

#include<algorithm>
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
		EXPECT_NEAR(ecl.lib.l_deg,-4.0681838374,1e-5);
		EXPECT_NEAR(ecl.lib.b_deg,0.3782181667,1e-5);
		EXPECT_NEAR(ecl.lib.c_deg,-21.2444894900,1e-5);
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
	EXPECT_NEAR(ecl.besselian.l2,-ecl.ru_re,1e-12);
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
	EXPECT_NEAR(ecl.catalog_mag,1.0386,2e-4);
	EXPECT_DOUBLE_EQ(ecl.catalog_obscuration,1.0);
	double fast_mag=0.0;
	double fast_obscuration=0.0;
	ASSERT_TRUE(calc_solar_eclipse_magnitude_from_max(
		eph,ecl.jd_tdb_max,ecl.type,&fast_mag,&fast_obscuration));
	EXPECT_DOUBLE_EQ(fast_mag,ecl.catalog_mag);
	EXPECT_DOUBLE_EQ(fast_obscuration,ecl.catalog_obscuration);
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
	EXPECT_NEAR(annular.catalog_mag,0.9630,1e-3);
	EXPECT_GT(annular.catalog_obscuration,0.0);
	EXPECT_LT(annular.catalog_obscuration,1.0);

	const double partial_jd_utc=greg2jd(2025,3,29,0,0,0.0)-UTC8DAY;
	SolarEclipse partial;
	ASSERT_TRUE(calc_solar_eclipse(
		eph,TimeScale::utc_to_tdb(partial_jd_utc),&partial));
	EXPECT_EQ(partial.type,"P");
	EXPECT_NEAR(partial.catalog_mag,0.9376,1.5e-3);
	EXPECT_GT(partial.catalog_obscuration,0.0);
	EXPECT_LT(partial.catalog_obscuration,1.0);
}

TEST(SolarEclipseCatalog, HistoricalBoundaryMagnitudes){
	EphRead eph("@series");
	struct Case{
		double jd_tdb;
		const char*input_type;
		const char*expected_type;
		double expected_mag;
	};
	const Case cases[]={
		{2433359.1472370834,"A","A",0.9620},
		{2435958.5037939330,"A","A",0.9799},
		{2436134.7041950226,"T","T",1.0013},
		{2439265.9021016695,"H","A",0.9991},
		{2439796.7353726323,"T","T",1.0126},
	};
	for(const auto&item : cases){
		double mag=0.0;
		double obscuration=0.0;
		std::string corrected_type;
		ASSERT_TRUE(calc_solar_eclipse_magnitude_from_max(
			eph,item.jd_tdb,item.input_type,&mag,&obscuration,&corrected_type));
		EXPECT_EQ(corrected_type,item.expected_type);
		EXPECT_NEAR(mag,item.expected_mag,2e-4);
		EXPECT_GE(obscuration,0.0);
		EXPECT_LE(obscuration,1.0);
	}
}

TEST(TimeScale, HistoricalDeltaT1950){
	const double jd_tdb=2433359.1472370834;
	double jd_tt=TimeScale::tdb_to_tt(jd_tdb);
	double delta_t=(jd_tdb-TimeScale::tdb_to_ut1(jd_tdb))*SEC_DAY;
	EXPECT_NEAR(delta_t,TimeScale::delta_t_seconds(jd_tt),5e-5);
}

TEST(SolarEclipseSeries, GreatestEclipseMatchesCatalogAndIsSeedInvariant){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	EphRead eph(test_ephem());
	struct CatalogCase{
		int year;
		int month;
		int day;
		int hour_td;
		int minute_td;
		double second_td;
		const char*type;
		double gamma;
		double catalog_mag;
	};
	// Authoritative catalog values: greatest-eclipse TD, type and gamma.
	// The one-limit events exercise both annular and total non-central geometry;
	// 2014 also remains sensitive to the old limb-based MAX objective.
	const std::array<CatalogCase,10> cases{{
		{1900,5,28,14,53,56.0,"T",0.3943,1.0249},
		{1900,11,22,7,19,43.0,"A",-0.2245,0.9421},
		{1950,3,18,15,32,1.0,"A",-0.9988,0.9620},
		{1957,4,30,0,5,28.0,"A",0.9992,0.9799},
		{1957,10,23,4,54,2.0,"T",-1.0022,1.0013},
		{1967,11,2,5,38,56.0,"T",-1.0007,1.0126},
		{2014,4,29,6,4,33.0,"A",-1.0000,0.9868},
		{2023,4,20,4,17,56.0,"H",-0.3952,1.0132},
		{2025,3,29,10,48,36.0,"P",1.0405,0.93759},
		{2029,7,11,15,37,19.0,"P",-1.4191,0.2303},
	}};

	for(const auto&item:cases){
		SCOPED_TRACE(std::to_string(item.year)+"-"+
					 std::to_string(item.month)+"-"+
					 std::to_string(item.day));
		const double catalog_td=greg2jd(item.year,item.month,item.day,
									   item.hour_td,item.minute_td,item.second_td);
		SolarEclipse ecl;
		ASSERT_TRUE(calc_solar_eclipse(eph,catalog_td,&ecl));
		ASSERT_TRUE(ecl.has);
		EXPECT_EQ(ecl.type,item.type);
		EXPECT_NEAR(ecl.jd_tdb_max,catalog_td,5.0/86400.0);
		EXPECT_NEAR(ecl.gamma,item.gamma,0.0002);
		EXPECT_NEAR(ecl.catalog_mag,item.catalog_mag,0.0025);

		SolarEclipse detail;
		ASSERT_TRUE(calc_solar_eclipse_from_max(eph,ecl.jd_tdb_max,&detail));
		EXPECT_DOUBLE_EQ(detail.jd_tdb_max,ecl.jd_tdb_max);
		EXPECT_DOUBLE_EQ(detail.catalog_mag,ecl.catalog_mag);

		if(item.year==2014){
			SolarEclipse early;
			SolarEclipse late;
			ASSERT_TRUE(calc_solar_eclipse(eph,catalog_td-6.0/24.0,&early));
			ASSERT_TRUE(calc_solar_eclipse(eph,catalog_td+6.0/24.0,&late));
			EXPECT_NEAR(early.jd_tdb_max,ecl.jd_tdb_max,0.01/86400.0);
			EXPECT_NEAR(late.jd_tdb_max,ecl.jd_tdb_max,0.01/86400.0);
			EXPECT_EQ(early.type,ecl.type);
			EXPECT_EQ(late.type,ecl.type);
		}
	}
}

TEST(EclipseEventMetadata, UsesDisplayCivilYearAcrossCalculationBoundary){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	EphRead eph(test_ephem());
	SolLunCal solver(eph);
	YearResult source_year=solver.compute_year(1897,nullptr);
	std::vector<EventRec> events=bld_lunar_eclipse_events(eph,source_year,480);
	auto it=std::find_if(events.begin(),events.end(),[](const EventRec&ev){
		return ev.utc_iso.rfind("1898-01-08",0)==0;
	});
	ASSERT_NE(it,events.end());
	EXPECT_EQ(it->year,1898);
	EXPECT_EQ(it->loc_iso.substr(0,4),"1898");
}
