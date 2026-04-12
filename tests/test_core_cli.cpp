#include "test_common.hpp"

#include<cmath>
#include<ctime>
#include<filesystem>
#include<fstream>
#include<iomanip>
#include<stdexcept>
#include<sstream>
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

void write_bin_text(const std::filesystem::path&path,const std::string&text){
	std::ofstream ofs(path,std::ios::binary);
	if(!ofs){
		throw std::runtime_error("failed to open file: "+path.string());
	}
	ofs.write(text.data(),static_cast<std::streamsize>(text.size()));
	if(!ofs){
		throw std::runtime_error("failed to write file: "+path.string());
	}
}

class ScopedCwd{
public:
	explicit ScopedCwd(const std::filesystem::path&path):
		old_(std::filesystem::current_path()){
		std::filesystem::current_path(path);
	}

	~ScopedCwd(){ std::filesystem::current_path(old_); }

private:
	std::filesystem::path old_;
};

std::filesystem::path make_temp_dir(const char*stem){
	const std::filesystem::path dir=make_temp_path(stem,"");
	std::filesystem::create_directories(dir);
	return dir;
}

std::string current_utc_iso_text(){
	const std::time_t now=std::time(nullptr);
	std::tm utc_tm{};
#if defined(_WIN32)
	gmtime_s(&utc_tm,&now);
#else
	gmtime_r(&now,&utc_tm);
#endif
	std::ostringstream oss;
	oss<<std::setfill('0')<<std::setw(4)<<utc_tm.tm_year+1900<<"-"
	   <<std::setw(2)<<utc_tm.tm_mon+1<<"-"<<std::setw(2)<<utc_tm.tm_mday
	   <<"T"<<std::setw(2)<<utc_tm.tm_hour<<":"<<std::setw(2)<<utc_tm.tm_min
	   <<":"<<std::setw(2)<<utc_tm.tm_sec<<"Z";
	return oss.str();
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

TEST(CoreDay, RejectsTrailingGarbageInAtTime){
	lunar::core::DayComputeOptions opt=make_day_opt("2025-06-01");
	opt.at_time="12:34oops";
	EXPECT_THROW(lunar::core::compute_day(opt),std::invalid_argument);

	opt.at_time="12:34:56xyz";
	EXPECT_THROW(lunar::core::compute_day(opt),std::invalid_argument);
}

TEST(CoreDay, RejectsInvalidCalendarDate){
	lunar::core::DayComputeOptions opt=make_day_opt("2025-02-31");
	EXPECT_THROW(lunar::core::compute_day(opt),std::invalid_argument);

	opt.date_text="2024-02-30";
	EXPECT_THROW(lunar::core::compute_day(opt),std::invalid_argument);
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

TEST(CApi, NativeJsonApisPopulateCaptureBuffer){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	ASSERT_EQ(0,lunar_core_day_json(test_ephem().c_str(),"2025-06-01","+08:00",0));
	ASSERT_NE(lunar_last_stdout(),nullptr);
	EXPECT_NE(std::string(lunar_last_stdout()).find("\"lunar_year\""),
			  std::string::npos);

	lunar_hli_rules rules{};
	lunar_hli_rules_init(&rules);
	ASSERT_EQ(
		0,
		lunar_core_ganzhi_json(
			test_ephem().c_str(),"2025-09-07","12:00:00","+08:00",&rules,0));
	ASSERT_NE(lunar_last_stdout(),nullptr);
	EXPECT_NE(std::string(lunar_last_stdout()).find("\"day\""),
			  std::string::npos);

	ASSERT_EQ(
		0,
		lunar_core_ganzhi_month_json(
			test_ephem().c_str(),2025,9,"12:00:00","+08:00",&rules,0));
	ASSERT_NE(lunar_last_stdout(),nullptr);
	EXPECT_NE(std::string(lunar_last_stdout()).find("\"day_ganzhi\""),
			  std::string::npos);

	const double jd_utc=greg2jd(2025,3,20,12,0,0.0)-UTC8DAY;
	ASSERT_EQ(0,lunar_calc_eot_json(test_ephem().c_str(),jd_utc,120.0,0));
	ASSERT_NE(lunar_last_stdout(),nullptr);
	EXPECT_NE(std::string(lunar_last_stdout()).find("\"eot_minutes\""),
			  std::string::npos);
	EXPECT_EQ(std::string(lunar_last_stderr()==nullptr?"":lunar_last_stderr()),"");
}

TEST(CApi, RunCaptureCollectsVersionOutput){
	const char*argv[]={"--version"};
	ASSERT_EQ(0,lunar_run_capture(1,argv));
	ASSERT_NE(lunar_last_stdout(),nullptr);
	EXPECT_EQ(std::string(lunar_last_stderr()==nullptr?"":lunar_last_stderr()),"");
	EXPECT_NE(std::string(lunar_last_stdout()).find(tool_ver()),std::string::npos);
}

TEST(CApi, RunCaptureCollectsJsonOutput){
	const char*argv[]={
		"config","show",
		"--format","json",
		"--pretty","0",
		"--quiet"
	};
	ASSERT_EQ(0,lunar_run_capture(6,argv));
	ASSERT_NE(lunar_last_stdout(),nullptr);
	const std::string text=lunar_last_stdout();
	EXPECT_FALSE(text.empty());
	EXPECT_EQ(text.front(),'{');
	EXPECT_NE(text.find("\"def_bsp\""),std::string::npos);
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

TEST(CliConvert, LunarDayTzChangesCivilDateMapping){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path out_8=make_temp_path("convert_ldtz_8",".txt");
	const std::filesystem::path out_9=make_temp_path("convert_ldtz_9",".txt");

	std::vector<std::string> args_8={
		"convert",test_ephem(),"2026-02-17T15:30:00Z",
		"--lunar-day-tz","+08:00",
		"--format","txt",
		"--out",out_8.string(),
		"--quiet"
	};
	std::vector<std::string> args_9={
		"convert",test_ephem(),"2026-02-17T15:30:00Z",
		"--lunar-day-tz","+09:00",
		"--format","txt",
		"--out",out_9.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args_8));
	ASSERT_EQ(0,run_cli_args(args_9));

	const std::string txt_8=read_file_text(out_8);
	const std::string txt_9=read_file_text(out_9);
	EXPECT_EQ(txt_value(txt_8,"input.lunar_day_tz"),"+08:00");
	EXPECT_EQ(txt_value(txt_9,"input.lunar_day_tz"),"+09:00");
	EXPECT_EQ(txt_value(txt_8,"data.gcst_date"),"2026-02-17");
	EXPECT_EQ(txt_value(txt_9,"data.gcst_date"),"2026-02-18");
	EXPECT_NE(txt_value(txt_8,"data.lun_label"),txt_value(txt_9,"data.lun_label"));

	std::error_code ec;
	std::filesystem::remove(out_8,ec);
	std::filesystem::remove(out_9,ec);
}

TEST(CliConvert, RejectsInvalidIsoCalendarDate){
	EXPECT_THROW(
		run_cli_args({"convert",test_ephem(),"2025-02-31T12:00:00+08:00","--quiet"}),
		std::invalid_argument);
	EXPECT_THROW(
		run_cli_args({"at",test_ephem(),"2024-02-30T12:00:00+08:00","--quiet"}),
		std::invalid_argument);
}

TEST(CliConvert, BatchFileAcceptsUtf8Bom){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path in_path=make_temp_path("convert_bom_in",".txt");
	const std::filesystem::path out_path=make_temp_path("convert_bom_out",".txt");
	write_bin_text(in_path,
				   "\xEF\xBB\xBF""2025-06-01T12:00:00+08:00\r\n");

	std::vector<std::string> args={
		"convert",test_ephem(),
		"--file",in_path.string(),
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_NE(txt.find("1\tok\t2025-06-01T12:00:00+08:00\tgreg2lun\t"),
			  std::string::npos);
	EXPECT_EQ(txt.find("invalid datetime"),std::string::npos);

	std::error_code ec;
	std::filesystem::remove(in_path,ec);
	std::filesystem::remove(out_path,ec);
}

TEST(CliConvert, BatchFromLunarFileWorksWithoutInlineDate){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path in_path=
		make_temp_path("convert_lunar_batch_in",".txt");
	const std::filesystem::path out_path=
		make_temp_path("convert_lunar_batch_out",".txt");
	write_bin_text(in_path,"2026 1 1\r\n");

	std::vector<std::string> args={
		"convert",test_ephem(),
		"--from-lunar",
		"--file",in_path.string(),
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_NE(txt.find("1\tok\t2026 1 1\tlun2greg\t"),std::string::npos);

	std::error_code ec;
	std::filesystem::remove(in_path,ec);
	std::filesystem::remove(out_path,ec);
}

TEST(CliConvert, BatchFromLunarRejectsExtraFields){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path in_path=
		make_temp_path("convert_lunar_extra_in",".txt");
	const std::filesystem::path out_path=
		make_temp_path("convert_lunar_extra_out",".txt");
	write_bin_text(in_path,"2026 1 1 leap extra\r\n");

	std::vector<std::string> args={
		"convert",test_ephem(),
		"--from-lunar",
		"--file",in_path.string(),
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(1,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_NE(txt.find("too many fields, expected: <lunar_year> <month_no> <day> [leap]"),
			  std::string::npos);

	std::error_code ec;
	std::filesystem::remove(in_path,ec);
	std::filesystem::remove(out_path,ec);
}

TEST(CliConvert, MissingFileValueIsRejectedBeforeOpen){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args({"convert",test_ephem(),"--file","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --file");
	}
}

TEST(CliDay, MissingExplicitBspPathDoesNotSilentlyFallback){
	EXPECT_THROW(
		run_cli_args({"day","definitely_missing_12345.bsp","2025-06-01","--quiet"}),
		std::runtime_error);
}

TEST(CliDay, SeriesAliasWorksAsExplicitEphem){
#if !LUNAR_ENABLE_SERIES_FALLBACK
	GTEST_SKIP()<<"requires series fallback";
#else
	const std::filesystem::path out_path=make_temp_path("day_series_alias",".txt");
	std::vector<std::string> args={
		"day","series","2025-06-01",
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_EQ(txt_value(txt,"input.date"),"2025-06-01");
	EXPECT_FALSE(txt_value(txt,"data.phase_name").empty());

	std::error_code ec;
	std::filesystem::remove(out_path,ec);
#endif
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

TEST(CliGlobal, HelpTokenIsNotConsumedAsLangValue){
	try{
		(void)run_cli_args({"--lang","--help"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --lang");
	}
}

TEST(CliGlobal, HelpTokenIsNotConsumedAsEclipseMethodValue){
	try{
		(void)run_cli_args({"--eclipse-method","--help"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),
				  "missing value for option: --eclipse-method");
	}
}

TEST(CliGlobal, HelpTokenIsNotConsumedAsBspValue){
	try{
		(void)run_cli_args(
			{"next","--bsp","--help","--from","2025-01-01","--count","1","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --bsp");
	}
}

TEST(CliGlobal, EmptyBspEqualsValueIsRejected){
	try{
		(void)run_cli_args(
			{"next","--bsp=","--from","2025-01-01","--count","1","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --bsp");
	}
}

TEST(CliGlobal, EmptyLangEqualsValueIsRejected){
	try{
		(void)run_cli_args({"--lang=","--version"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --lang");
	}
}

TEST(CliGlobal, EmptyEclipseMethodEqualsValueIsRejected){
	try{
		(void)run_cli_args({"--eclipse-method=","--version"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),
				  "missing value for option: --eclipse-method");
	}
}

TEST(CliNext, MissingFromValueIsRejectedBeforeOptionParsing){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args({"next",test_ephem(),"--from","--count","1","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --from");
	}
}

TEST(CliNext, HelpTokenIsNotConsumedAsFromValue){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args({"next",test_ephem(),"--from","-h","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --from");
	}
}

TEST(CliNext, MissingFromValueWithoutExplicitBspKeepsSameError){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path dir=make_temp_dir("next_missing_from_auto_bsp");
	write_bin_text(dir/"lun_cfg.txt","def_bsp="+test_ephem()+"\n");
	{
		ScopedCwd cwd(dir);
		try{
			(void)run_cli_args({"next","--from","--count","1","--quiet"});
			FAIL()<<"expected invalid_argument";
		}catch(const std::invalid_argument&ex){
			EXPECT_EQ(std::string(ex.what()),"missing value for option: --from");
		}
	}

	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
}

TEST(CliNext, EclipseKindsActuallyReturnEclipseEvents){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path out_path=make_temp_path("next_eclipse",".txt");
	std::vector<std::string> args={
		"next",test_ephem(),
		"--from","2025-01-01T00:00:00+08:00",
		"--count","1",
		"--kinds","lunar_eclipse",
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_NE(txt.find("lunar_eclipse\t"),std::string::npos);
	EXPECT_NE(txt.find("ecl_type"),std::string::npos);

	std::error_code ec;
	std::filesystem::remove(out_path,ec);
}

TEST(CliSearch, UsesConfigDefaultsAndMatchesExactPhaseCode){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path dir=make_temp_dir("search_cfg_case");
	const std::filesystem::path out_path=dir/"search.out";
	write_bin_text(dir/"lun_cfg.txt","default_tz=Z\ndef_fmt=json\ndef_prety=0\n");
	{
		ScopedCwd cwd(dir);
		std::vector<std::string> args={
			"search",test_ephem(),"next full_moon",
			"--count","1",
			"--out",out_path.string(),
			"--quiet"
		};
		ASSERT_EQ(0,run_cli_args(args));
	}
	const std::string txt=read_file_text(out_path);
	EXPECT_FALSE(txt.empty());
	EXPECT_EQ(txt.front(),'{');
	EXPECT_NE(txt.find("\"tz_display\":\"Z\""),std::string::npos);
	EXPECT_NE(txt.find("\"code\":\"full_moon\""),std::string::npos);
	EXPECT_EQ(txt.find("\"code\":\"fst_qtr\""),std::string::npos);

	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
}

TEST(CliSearch, DefaultFromUsesCurrentUtcAndReportsSearchType){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path out_auto=make_temp_path("search_auto_now",".json");
	const std::filesystem::path out_explicit=
		make_temp_path("search_explicit_now",".json");
	const std::string now_utc=current_utc_iso_text();

	std::vector<std::string> auto_args={
		"search",test_ephem(),"next solar_term",
		"--count","1",
		"--format","json",
		"--out",out_auto.string(),
		"--quiet"
	};
	std::vector<std::string> explicit_args={
		"search",test_ephem(),"next solar_term",
		"--from",now_utc,
		"--count","1",
		"--format","json",
		"--out",out_explicit.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(auto_args));
	ASSERT_EQ(0,run_cli_args(explicit_args));

	const std::string auto_txt=read_file_text(out_auto);
	const std::string explicit_txt=read_file_text(out_explicit);
	EXPECT_EQ(auto_txt,explicit_txt);
	EXPECT_NE(auto_txt.find("\"type=search\""),std::string::npos);
	EXPECT_EQ(auto_txt.find("\"type=next\""),std::string::npos);

	std::error_code ec;
	std::filesystem::remove(out_auto,ec);
	std::filesystem::remove(out_explicit,ec);
}

TEST(CliSearch, NaturalLanguageEclipseAliasWorks){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path out_path=make_temp_path("search_lunar_eclipse",".txt");
	std::vector<std::string> args={
		"search",test_ephem(),"next lunar eclipse",
		"--from","2025-01-01T00:00:00+08:00",
		"--count","1",
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_NE(txt.find("lunar_eclipse\t"),std::string::npos);
	EXPECT_EQ(txt.find("solar_term\t"),std::string::npos);

	std::error_code ec;
	std::filesystem::remove(out_path,ec);
}

TEST(CliSearch, UnknownTargetIsRejected){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args({"search",test_ephem(),"next nonsense","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"unsupported search query: next nonsense");
	}
}

TEST(CliSearch, RejectsNonPositiveCount){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args({"search",test_ephem(),"next full_moon","--count","0","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"--count must be >=1");
	}
}

TEST(CliSky, PickQueryListsSolarSystemTargetsFirst){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path out_path=make_temp_path("sky",".txt");
	std::vector<std::string> args={
		"sky",test_ephem(),
		"2025-06-01T20:00:00+08:00",
		"--lat","31.23",
		"--lon","121.47",
		"--mode","pick",
		"--pick","sun,moon,Spica",
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_EQ(txt_value(txt,"input.mode"),"pick");
	EXPECT_EQ(txt_value(txt,"input.lat_deg"),"31.23");
	EXPECT_EQ(txt_value(txt,"input.lon_deg"),"121.47");

	std::istringstream iss(txt);
	std::string line;
	std::string first_row;
	std::string second_row;
	std::string third_row;
	while(std::getline(iss,line)){
		if(line.rfind("kind\tcode\tname\tregion\tis_solar_system",0)==0){
			ASSERT_TRUE(static_cast<bool>(std::getline(iss,first_row)));
			ASSERT_TRUE(static_cast<bool>(std::getline(iss,second_row)));
			ASSERT_TRUE(static_cast<bool>(std::getline(iss,third_row)));
			break;
		}
	}
	ASSERT_FALSE(first_row.empty());
	EXPECT_NE(first_row.find("\tsun\t"),std::string::npos);
	EXPECT_NE(second_row.find("\tmoon\t"),std::string::npos);
	EXPECT_NE(third_row.find("\tHR5056\t"),std::string::npos);

	std::error_code ec;
	std::filesystem::remove(out_path,ec);
}

TEST(CliDay, MissingTzValueIsRejectedBeforeOptionParsing){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args({"day",test_ephem(),"2025-06-01","--tz","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"missing value for option: --tz");
	}
}

TEST(CliEclipse, RejectsNonPositiveSampleMinutes){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args(
			{"eclipse",test_ephem(),"--near","2025-09-07","--sample-min","0","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),"--sample-min must be > 0");
	}
}

TEST(CliEclipse, PointHeightRequiresPointCoordinates){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args(
			{"eclipse",test_ephem(),"--near","2025-09-07","--point-height","10","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),
				  "--point-height/--point-refine require --point-lat and --point-lon");
	}
}

TEST(CliEclipse, PointRefineRequiresPointCoordinates){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args(
			{"eclipse",test_ephem(),"--near","2025-09-07","--point-refine","0","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(std::string(ex.what()),
				  "--point-height/--point-refine require --point-lat and --point-lon");
	}
}

TEST(CliEclipse, GridStepRequiresGlobalVisibility){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args(
			{"eclipse",test_ephem(),"--near","2025-09-07","--grid-lat-step","5","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(
			std::string(ex.what()),
			"--global-format/--grid-lat-step/--grid-lon-step require --global-vis 1 or --format geojson");
	}
}

TEST(CliEclipse, GlobalFormatRequiresGlobalVisibility){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	try{
		(void)run_cli_args(
			{"eclipse",test_ephem(),"--near","2025-09-07","--global-format","geojson","--quiet"});
		FAIL()<<"expected invalid_argument";
	}catch(const std::invalid_argument&ex){
		EXPECT_EQ(
			std::string(ex.what()),
			"--global-format/--grid-lat-step/--grid-lon-step require --global-vis 1 or --format geojson");
	}
}

TEST(CliSky, EnglishOutputLocalizesNamesAndRegion){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path out_path=make_temp_path("sky_en",".txt");
	std::vector<std::string> args={
		"--lang","en",
		"sky",test_ephem(),
		"2025-06-01T20:00:00+08:00",
		"--lat","31.23",
		"--lon","121.47",
		"--mode","pick",
		"--pick","sun,moon,Spica",
		"--format","txt",
		"--out",out_path.string(),
		"--quiet"
	};
	ASSERT_EQ(0,run_cli_args(args));
	const std::string txt=read_file_text(out_path);
	EXPECT_NE(txt.find("\tsun\tSun\t\t1\t"),std::string::npos);
	EXPECT_NE(txt.find("\tmoon\tMoon\t\t1\t"),std::string::npos);
	EXPECT_NE(txt.find("\tHR5056\tSpica\tJiao Mansion\t0\t"),std::string::npos);
	EXPECT_EQ(txt.find("角宿"),std::string::npos);

	std::error_code ec;
	std::filesystem::remove(out_path,ec);
}
