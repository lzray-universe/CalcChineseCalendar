#include<gtest/gtest.h>

#include<array>
#include<string>
#include<vector>

#include "lunar/calendar.hpp"
#include "lunar/core.hpp"
#include "lunar/lunar_eclipse.hpp"
#include "lunar/solar_eclipse.hpp"
#include "lunar/time_scale.hpp"

#include "test_common.hpp"

namespace{

constexpr double kSolarTermTolSec=15.0*60.0;
constexpr double kLunarPhaseTolSec=20.0*60.0;
constexpr double kLunarMonthTolSec=20.0*60.0;
constexpr double kIllPctTol=0.2;
constexpr double kAngleTolDeg=0.1;
constexpr double kEclipseTimeTolSec=20.0*60.0;
constexpr double kEclipseScalarTol=0.02;
constexpr std::array<const char*,6> kStableDates={{
	"2025-01-30",
	"2025-03-06",
	"2025-04-25",
	"2025-06-10",
	"2025-08-25",
	"2025-11-10",
}};

double cst_to_utc_jd(int year,int month,int day,int hour=0,int minute=0,
					 double second=0.0){
	return greg2jd(year,month,day,hour,minute,second)-UTC8DAY;
}

void expect_close_jd(double lhs,double rhs,double tol_sec){
	EXPECT_NEAR(lhs,rhs,tol_sec/SEC_DAY);
}

lunar::core::DayComputeOptions make_day_opt(const std::string&ephem,
											 const std::string&date_text){
	lunar::core::DayComputeOptions opt;
	opt.ephem=ephem;
	opt.date_text=date_text;
	opt.at_time="12:00:00";
	opt.tz="+08:00";
	opt.lunar_day_tz="+08:00";
	opt.include_events=false;
	opt.include_astro=false;
	return opt;
}

lunar::core::GanzhiComputeOptions make_ganzhi_opt(const std::string&ephem,
												  const std::string&date_text){
	lunar::core::GanzhiComputeOptions opt;
	opt.ephem=ephem;
	opt.date_text=date_text;
	opt.at_time="12:00:00";
	opt.tz="+08:00";
	opt.lunar_day_tz="+08:00";
	return opt;
}

void expect_same_lunar_date(const LunDate&lhs,const LunDate&rhs){
	EXPECT_EQ(lhs.lunar_year,rhs.lunar_year);
	EXPECT_EQ(lhs.lun_mno,rhs.lun_mno);
	EXPECT_EQ(lhs.is_leap,rhs.is_leap);
	EXPECT_EQ(lhs.lunar_day,rhs.lunar_day);
	EXPECT_EQ(lhs.lun_mlab,rhs.lun_mlab);
	EXPECT_EQ(lhs.lun_label,rhs.lun_label);
}

void expect_same_gz(const GzNode&lhs,const GzNode&rhs){
	EXPECT_EQ(lhs.stem,rhs.stem);
	EXPECT_EQ(lhs.branch,rhs.branch);
	EXPECT_EQ(lhs.text,rhs.text);
}

}

TEST(SeriesVsBspCalendar, SolarTermsTrackReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::string ref_bsp=reference_bsp();
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);
	SolLunCal series_solver(series_eph);
	SolLunCal bsp_solver(bsp_eph);

	for(const auto&it : SolLunCal::st_defs()){
		const std::string&code=it.first;
		const LocalDT series_dt=series_solver.find_st(code,2025);
		const LocalDT bsp_dt=bsp_solver.find_st(code,2025);
		SCOPED_TRACE(code);
		expect_close_jd(series_dt.toUtcJD(),bsp_dt.toUtcJD(),kSolarTermTolSec);
	}
#endif
}

TEST(SeriesVsBspCalendar, LunarPhasesTrackReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::string ref_bsp=reference_bsp();
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);
	SolLunCal series_solver(series_eph);
	SolLunCal bsp_solver(bsp_eph);

	const YearResult series_year=series_solver.compute_year(2025,nullptr);
	const YearResult bsp_year=bsp_solver.compute_year(2025,nullptr);
	ASSERT_EQ(series_year.lun_phase.size(),bsp_year.lun_phase.size());

	for(std::size_t i=0;i<series_year.lun_phase.size();++i){
		const MoonPhMon&series_item=series_year.lun_phase[i];
		const MoonPhMon&bsp_item=bsp_year.lun_phase[i];
		SCOPED_TRACE(i);
		expect_close_jd(series_item.new_moon.toUtcJD(),bsp_item.new_moon.toUtcJD(),
					   kLunarPhaseTolSec);
		expect_close_jd(series_item.fst_qtr.toUtcJD(),bsp_item.fst_qtr.toUtcJD(),
					   kLunarPhaseTolSec);
		expect_close_jd(series_item.full_moon.toUtcJD(),bsp_item.full_moon.toUtcJD(),
					   kLunarPhaseTolSec);
		expect_close_jd(series_item.lst_qtr.toUtcJD(),bsp_item.lst_qtr.toUtcJD(),
					   kLunarPhaseTolSec);
	}
#endif
}

TEST(SeriesVsBspCalendar, LunarMonthsTrackReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::string ref_bsp=reference_bsp();
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);
	LunCal6 series_calc(series_eph);
	LunCal6 bsp_calc(bsp_eph);

	const std::vector<LunarMonth>&series_months=series_calc.get_months(2025);
	const std::vector<LunarMonth>&bsp_months=bsp_calc.get_months(2025);
	ASSERT_EQ(series_months.size(),bsp_months.size());

	for(std::size_t i=0;i<series_months.size();++i){
		const LunarMonth&series_item=series_months[i];
		const LunarMonth&bsp_item=bsp_months[i];
		SCOPED_TRACE(series_item.label);
		EXPECT_EQ(series_item.month_no,bsp_item.month_no);
		EXPECT_EQ(series_item.is_leap,bsp_item.is_leap);
		EXPECT_EQ(series_item.label,bsp_item.label);
		expect_close_jd(series_item.start_dt.toUtcJD(),bsp_item.start_dt.toUtcJD(),
					   kLunarMonthTolSec);
		expect_close_jd(series_item.end_dt.toUtcJD(),bsp_item.end_dt.toUtcJD(),
					   kLunarMonthTolSec);
	}
#endif
}

TEST(SeriesVsBspCoreDay, StableDatesTrackReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::string ref_bsp=reference_bsp();
	for(const char*date_text : kStableDates){
		const DayResult series_day=
			lunar::core::compute_day(make_day_opt("series",date_text));
		const DayResult bsp_day=
			lunar::core::compute_day(make_day_opt(ref_bsp,date_text));
		SCOPED_TRACE(date_text);
		expect_same_lunar_date(series_day.at_data.lunar_date,bsp_day.at_data.lunar_date);
		EXPECT_EQ(series_day.at_data.phase_name,bsp_day.at_data.phase_name);
		EXPECT_EQ(series_day.at_data.waxing,bsp_day.at_data.waxing);
		EXPECT_NEAR(series_day.at_data.ill_pct,bsp_day.at_data.ill_pct,kIllPctTol);
		EXPECT_NEAR(series_day.at_data.elong_deg,bsp_day.at_data.elong_deg,
					kAngleTolDeg);
	}
#endif
}

TEST(SeriesVsBspGanzhi, StableDatesTrackReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::string ref_bsp=reference_bsp();
	for(const char*date_text : kStableDates){
		const lunar::core::GanzhiSummary series_sum=
			lunar::core::compute_ganzhi(make_ganzhi_opt("series",date_text));
		const lunar::core::GanzhiSummary bsp_sum=
			lunar::core::compute_ganzhi(make_ganzhi_opt(ref_bsp,date_text));
		SCOPED_TRACE(date_text);
		expect_same_gz(series_sum.year,bsp_sum.year);
		expect_same_gz(series_sum.month,bsp_sum.month);
		expect_same_gz(series_sum.day,bsp_sum.day);
	}
#endif
}

TEST(SeriesVsBspLunarEclipse, TotalEclipseTracksReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const double jd_tdb=TimeScale::utc_to_tdb(cst_to_utc_jd(2025,9,7));
	const std::string ref_bsp=reference_bsp();
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);
	LunarEclipse series_ecl;
	LunarEclipse bsp_ecl;
	ASSERT_TRUE(calc_lunar_eclipse(series_eph,jd_tdb,&series_ecl));
	ASSERT_TRUE(calc_lunar_eclipse(bsp_eph,jd_tdb,&bsp_ecl));
	ASSERT_TRUE(series_ecl.has);
	ASSERT_TRUE(bsp_ecl.has);
	EXPECT_EQ(series_ecl.type,bsp_ecl.type);
	expect_close_jd(series_ecl.jd_tdb_p1,bsp_ecl.jd_tdb_p1,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_u1,bsp_ecl.jd_tdb_u1,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_max,bsp_ecl.jd_tdb_max,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_u4,bsp_ecl.jd_tdb_u4,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_p4,bsp_ecl.jd_tdb_p4,kEclipseTimeTolSec);
	EXPECT_NEAR(series_ecl.pen_mag,bsp_ecl.pen_mag,kEclipseScalarTol);
	EXPECT_NEAR(series_ecl.umb_mag,bsp_ecl.umb_mag,kEclipseScalarTol);
	EXPECT_NEAR(series_ecl.gamma,bsp_ecl.gamma,kEclipseScalarTol);
	EXPECT_NEAR(series_ecl.lib.l_deg,bsp_ecl.lib.l_deg,kAngleTolDeg);
	EXPECT_NEAR(series_ecl.lib.b_deg,bsp_ecl.lib.b_deg,kAngleTolDeg);
	EXPECT_NEAR(series_ecl.lib.c_deg,bsp_ecl.lib.c_deg,kAngleTolDeg);
#endif
}

TEST(SeriesVsBspSolarEclipse, TotalEclipseTracksReference){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const double jd_tdb=TimeScale::utc_to_tdb(cst_to_utc_jd(2026,8,12));
	const std::string ref_bsp=reference_bsp();
	EphRead series_eph("series");
	EphRead bsp_eph(ref_bsp);
	SolarEclipse series_ecl;
	SolarEclipse bsp_ecl;
	ASSERT_TRUE(calc_solar_eclipse(series_eph,jd_tdb,&series_ecl));
	ASSERT_TRUE(calc_solar_eclipse(bsp_eph,jd_tdb,&bsp_ecl));
	ASSERT_TRUE(series_ecl.has);
	ASSERT_TRUE(bsp_ecl.has);
	EXPECT_EQ(series_ecl.type,bsp_ecl.type);
	expect_close_jd(series_ecl.jd_tdb_c1,bsp_ecl.jd_tdb_c1,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_c2,bsp_ecl.jd_tdb_c2,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_max,bsp_ecl.jd_tdb_max,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_c3,bsp_ecl.jd_tdb_c3,kEclipseTimeTolSec);
	expect_close_jd(series_ecl.jd_tdb_c4,bsp_ecl.jd_tdb_c4,kEclipseTimeTolSec);
	EXPECT_NEAR(series_ecl.mag,bsp_ecl.mag,kEclipseScalarTol);
	EXPECT_NEAR(series_ecl.catalog_mag,bsp_ecl.catalog_mag,kEclipseScalarTol);
	EXPECT_NEAR(series_ecl.obscuration,bsp_ecl.obscuration,kEclipseScalarTol);
	EXPECT_NEAR(series_ecl.gamma,bsp_ecl.gamma,kEclipseScalarTol);
	EXPECT_NEAR(series_ecl.sep_max_deg,bsp_ecl.sep_max_deg,kAngleTolDeg);
#endif
}
