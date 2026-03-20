#include "test_common.hpp"

#include<cmath>
#include<filesystem>
#include<string>
#include<vector>

#include<gtest/gtest.h>

#include "lunar/c_api.h"
#include "lunar/calendar.hpp"
#include "lunar/core.hpp"
#include "lunar/entry.hpp"
#include "lunar/time_scale.hpp"

namespace{

lunar::core::DayComputeOptions make_day_opt(const std::string&date_text){
	lunar::core::DayComputeOptions opt;
	opt.ephem=test_ephem();
	opt.date_text=date_text;
	opt.at_time="12:00:00";
	opt.tz="+08:00";
	opt.include_events=false;
	opt.include_astro=false;
	return opt;
}

}

TEST(CoreDay, IlluminationStaysInRange){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	DayResult day=lunar::core::compute_day(make_day_opt("2025-06-01"));
	EXPECT_GE(day.at_data.ill_pct,0.0);
	EXPECT_LE(day.at_data.ill_pct,100.0);
}

TEST(CalendarSeries, YearHasTermsAndPhases){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	EphRead eph(test_ephem());
	SolLunCal solver(eph);
	YearResult year=solver.compute_year(2025,nullptr);
	EXPECT_EQ(year.sol_terms.size(),24u);
	EXPECT_GE(year.lun_phase.size()*4u,48u);
}

TEST(CoreGanzhi, ThinApisMatchDayAndCApi){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	lunar::core::GanzhiComputeOptions gz_opt;
	gz_opt.ephem=test_ephem();
	gz_opt.date_text="2025-09-07";
	gz_opt.at_time="12:00:00";
	gz_opt.tz="+08:00";
	gz_opt.hli_rules=make_hli_rule_set(HliProfileCode::ZiPing);
	lunar::core::GanzhiSummary gz=lunar::core::compute_ganzhi(gz_opt);

	lunar::core::DayComputeOptions day_opt=make_day_opt(gz_opt.date_text);
	day_opt.at_time=gz_opt.at_time;
	day_opt.tz=gz_opt.tz;
	day_opt.hli_rules=gz_opt.hli_rules;
	DayResult day=lunar::core::compute_day(day_opt);

	lunar::core::GanzhiMonthComputeOptions month_opt;
	month_opt.ephem=test_ephem();
	month_opt.year=2025;
	month_opt.month=9;
	month_opt.at_time=gz_opt.at_time;
	month_opt.tz=gz_opt.tz;
	month_opt.hli_rules=gz_opt.hli_rules;
	lunar::core::GanzhiMonthSummary month=
		lunar::core::compute_ganzhi_month(month_opt);

	lunar_hli_rules c_rules;
	lunar_hli_rules_init(&c_rules);
	c_rules.profile_code=gz_opt.hli_rules.profile_code;
	c_rules.year_boundary=gz_opt.hli_rules.year_boundary;
	c_rules.month_boundary=gz_opt.hli_rules.month_boundary;
	c_rules.leap_month_mode=gz_opt.hli_rules.leap_month_mode;
	c_rules.day_boundary=gz_opt.hli_rules.day_boundary;

	lunar_ganzhi_summary c_sum{};
	lunar_ganzhi_month_summary c_month{};
	ASSERT_EQ(
		0,
		lunar_core_ganzhi(test_ephem().c_str(),gz_opt.date_text.c_str(),
						  gz_opt.at_time.c_str(),gz_opt.tz.c_str(),&c_rules,&c_sum));
	ASSERT_EQ(
		0,
		lunar_core_ganzhi_month(test_ephem().c_str(),month_opt.year,
								month_opt.month,month_opt.at_time.c_str(),
								month_opt.tz.c_str(),&c_rules,&c_month));

	EXPECT_EQ(gz.year.text,day.at_data.hli.y_rule.text);
	EXPECT_EQ(gz.month.text,day.at_data.hli.m_gz.text);
	EXPECT_EQ(gz.day.text,day.at_data.hli.d_gz.text);
	EXPECT_EQ(gz.year.stem,day.at_data.hli.y_rule.stem);
	EXPECT_EQ(gz.month.branch,day.at_data.hli.m_gz.branch);
	EXPECT_EQ(gz.hli_rules.profile_code,static_cast<int>(HliProfileCode::ZiPing));

	ASSERT_EQ(month.year,2025);
	ASSERT_EQ(month.month,9);
	ASSERT_EQ(month.years.size(),30u);
	ASSERT_EQ(month.months.size(),30u);
	ASSERT_EQ(month.days.size(),30u);
	EXPECT_EQ(month.years[6].text,gz.year.text);
	EXPECT_EQ(month.months[6].text,gz.month.text);
	EXPECT_EQ(month.days[6].text,gz.day.text);

	EXPECT_EQ(c_sum.year.stem,gz.year.stem);
	EXPECT_EQ(c_sum.month.branch,gz.month.branch);
	EXPECT_EQ(std::string(c_sum.year.text),gz.year.text);
	EXPECT_EQ(std::string(c_sum.month.text),gz.month.text);
	EXPECT_EQ(std::string(c_sum.day.text),gz.day.text);
	EXPECT_EQ(c_sum.rule_profile_code,gz.hli_rules.profile_code);
	EXPECT_EQ(c_sum.year_boundary_code,gz.hli_rules.year_boundary);
	EXPECT_EQ(c_sum.month_boundary_code,gz.hli_rules.month_boundary);
	EXPECT_EQ(c_sum.leap_month_mode_code,gz.hli_rules.leap_month_mode);
	EXPECT_EQ(c_sum.day_boundary_code,gz.hli_rules.day_boundary);

	EXPECT_EQ(c_month.year,month.year);
	EXPECT_EQ(c_month.month,month.month);
	EXPECT_EQ(c_month.day_count,30);
	EXPECT_EQ(std::string(c_month.years[6].text),month.years[6].text);
	EXPECT_EQ(std::string(c_month.months[6].text),month.months[6].text);
	EXPECT_EQ(std::string(c_month.days[6].text),month.days[6].text);
	EXPECT_EQ(c_month.rule_profile_code,month.hli_rules.profile_code);
	EXPECT_EQ(c_month.year_boundary_code,month.hli_rules.year_boundary);
	EXPECT_EQ(c_month.month_boundary_code,month.hli_rules.month_boundary);
	EXPECT_EQ(c_month.leap_month_mode_code,month.hli_rules.leap_month_mode);
	EXPECT_EQ(c_month.day_boundary_code,month.hli_rules.day_boundary);
}

TEST(CApi, DayAndEotAreCallable){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	lunar_day_summary day{};
	ASSERT_EQ(0,lunar_core_day(test_ephem().c_str(),"2025-06-01","+08:00",&day));
	EXPECT_EQ(day.is_leap,0);
	EXPECT_GE(day.ill_pct,0.0);
	EXPECT_LE(day.ill_pct,100.0);
	EXPECT_NE(std::string(day.phase_name).size(),0u);

	lunar_eot_result eot{};
	const double jd_utc=greg2jd(2025,3,20,12,0,0.0)-UTC8DAY;
	ASSERT_EQ(0,lunar_calc_eot(test_ephem().c_str(),jd_utc,120.0,&eot));
	EXPECT_TRUE(std::isfinite(eot.eot_minutes));
	EXPECT_TRUE(std::isfinite(eot.apparent_solar_time_rad));
}

TEST(CliConvert, RoundTripStaysOnSameCivilDate){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path greg_out=make_temp_path("convert_greg2lun",".txt");
	const std::filesystem::path lun_out=make_temp_path("convert_lun2greg",".txt");

	std::vector<std::string> greg_args={
		"convert",test_ephem(),"2026-02-18",
		"--format","txt",
		"--out",greg_out.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(greg_args));
	const std::string greg_txt=read_file_text(greg_out);
	const std::string lunar_year=txt_value(greg_txt,"data.lunar_year");
	const std::string lun_mno=txt_value(greg_txt,"data.lun_mno");
	const std::string lunar_day=txt_value(greg_txt,"data.lunar_day");
	const std::string lun_leap=txt_value(greg_txt,"data.lun_leap");
	ASSERT_FALSE(lunar_year.empty());
	ASSERT_FALSE(lun_mno.empty());
	ASSERT_FALSE(lunar_day.empty());
	ASSERT_FALSE(lun_leap.empty());

	std::vector<std::string> lunar_args={
		"convert",test_ephem(),
		"--from-lunar",lunar_year,lun_mno,lunar_day,
		"--leap",lun_leap,
		"--format","txt",
		"--out",lun_out.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(lunar_args));
	const std::string lun_txt=read_file_text(lun_out);
	EXPECT_EQ(txt_value(lun_txt,"data.gcst_date"),"2026-02-18");

	std::error_code ec;
	std::filesystem::remove(greg_out,ec);
	std::filesystem::remove(lun_out,ec);
}

TEST(CliInfo, SeriesEphemerisInfoWritesText){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path out_path=make_temp_path("info",".txt");
	std::vector<std::string> args={
		"info",test_ephem(),
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_EQ(txt_value(txt,"ephem.path"),test_ephem());
	EXPECT_EQ(txt_value(txt,"spk.coverage"),"not_avail");

	std::error_code ec;
	std::filesystem::remove(out_path,ec);
}
