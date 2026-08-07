#include<algorithm>
#include<array>
#include<string>
#include<vector>

#include<gtest/gtest.h>

#include "lunar/almanac.hpp"
#include "lunar/core.hpp"
#include "lunar/i18n.hpp"

#include "test_common.hpp"

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

HliData make_hli_sample(){
	HliData h;
	h.y_lun={1,5,"乙巳"};
	h.y_lchun={0,4,"甲辰"};
	h.y_rule={1,5,"乙巳"};
	h.m_gz={0,8,"甲申"};
	h.d_gz={5,3,"己卯"};
	h.h_gz={6,6,"庚午"};
	h.h_gz_true={6,6,"庚午"};
	h.rule_profile_code=static_cast<int>(HliProfileCode::Folk);
	h.year_boundary_code=static_cast<int>(HliYearBoundary::LunarNewYear);
	h.month_boundary_code=static_cast<int>(HliMonthBoundary::LunarFirstDay);
	h.leap_month_mode_code=
		static_cast<int>(HliLeapMonthMode::InheritPrevious);
	h.day_boundary_code=static_cast<int>(HliDayBoundary::Hour23);
	h.jianchu_code=0;
	h.duty_god_code=0;
	h.duty_is_yellow=true;
	h.duty_tag_code=1;
	h.nayin_code=0;
	h.fetal_god_code=0;
	h.meridian_code=0;
	h.lucky_dir_code=0;
	h.wealth_dir_code=1;
	h.mascot_dir_code=2;
	h.sun_noble_dir_code=3;
	h.moon_noble_dir_code=4;
	h.xiu28_code=0;
	h.xiu28_mod28_code=1;
	h.xiu_id="Sadalmelik";
	h.yi_ji_rule_code=static_cast<int>(HliRuleCode::FollowBoth);
	for(int i=0;i<=static_cast<int>(HliGodCode::MonthlyHarm);++i){
		h.good_god_codes.push_back(i);
		h.bad_god_codes.push_back(i);
	}
	for(int i=0;i<=static_cast<int>(HliActCode::NoMajorTaboo);++i){
		h.yi_codes.push_back(i);
		h.ji_codes.push_back(i);
	}
	h.hour_jx.push_back(HliHour{0,0,false,"子(23:00-00:59)","甲子","吉"});
	return h;
}

bool clean_text(const std::string&text){
	return !text.empty()&&text.find('?')==std::string::npos;
}

bool all_non_empty(const std::vector<std::string>&items){
	return std::all_of(items.begin(),items.end(),[](const std::string&item){
		return !item.empty();
	});
}

bool all_clean(const std::vector<std::string>&items){
	return std::all_of(items.begin(),items.end(),[](const std::string&item){
		return clean_text(item);
	});
}

std::vector<std::string> sorted(std::initializer_list<const char*> items){
	std::vector<std::string> out;
	for(const char*item : items){
		out.emplace_back(item);
	}
	std::sort(out.begin(),out.end());
	return out;
}

std::vector<std::string> sorted(std::vector<std::string> items){
	std::sort(items.begin(),items.end());
	return items;
}

}

TEST(AlmanacSeries, CodesAndMod28ArePresent){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	DayResult day=lunar::core::compute_day(make_day_opt("2025-09-07"));
	const HliData&h=day.at_data.hli;
	EXPECT_GE(h.xiu28_code,0);
	EXPECT_LT(h.xiu28_code,28);
	EXPECT_GE(h.xiu28_mod28_code,0);
	EXPECT_LT(h.xiu28_mod28_code,28);
	EXPECT_FALSE(h.xiu28.empty());
	EXPECT_FALSE(h.xiu28_mod28.empty());
	EXPECT_EQ(h.good_god_codes.size(),h.good_gods.size());
	EXPECT_EQ(h.yi_codes.size(),h.yi.size());
	EXPECT_GE(h.nayin_code,0);
	EXPECT_GE(h.fetal_god_code,0);
	EXPECT_GE(h.lucky_dir_code,0);
	ASSERT_EQ(h.hour_jx.size(),13u);
	EXPECT_EQ(h.hour_jx.front().slot_index,0);
	EXPECT_GE(h.hour_jx.front().gz_index,0);
}

TEST(AlmanacRules, FolkAndXieJiProfilesMatchExpectedCase){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	lunar::core::DayComputeOptions old_opt=make_day_opt("2025-09-17");
	old_opt.hli_rules=make_hli_rule_set(HliProfileCode::Folk);
	lunar::core::DayComputeOptions xieji_opt=old_opt;
	xieji_opt.hli_rules=make_hli_rule_set(HliProfileCode::XieJi);

	DayResult old_day=lunar::core::compute_day(old_opt);
	const HliData&old=old_day.at_data.hli;
	EXPECT_EQ(old.m_gz.text,"乙酉");
	EXPECT_EQ(old.jianchu,"执");
	EXPECT_EQ(old.duty_god,"明堂");
	EXPECT_EQ(sorted(old.good_gods),sorted({"母仓"}));
	EXPECT_EQ(sorted(old.bad_gods),sorted({"受死","五鬼","归忌","专日"}));
	EXPECT_EQ(sorted(old.yi),sorted({"纳财","捕捉","栽种","牧养","纳畜"}));
	EXPECT_EQ(sorted(old.ji),sorted({
		"出行","冠带","畋猎","搬移","远回","安抚边境","选将","出师"
	}));
	EXPECT_EQ(old.yi_ji_rule,"从宜亦从忌");

	DayResult xieji_day=lunar::core::compute_day(xieji_opt);
	const HliData&xieji=xieji_day.at_data.hli;
	EXPECT_EQ(xieji.m_gz.text,"乙酉");
	EXPECT_EQ(xieji.jianchu,"定");
	EXPECT_EQ(xieji.duty_god,"勾陈");
	EXPECT_EQ(sorted(xieji.good_gods),
			  sorted({"天德","三合","母仓","金堂"}));
	EXPECT_EQ(sorted(xieji.bad_gods),
			  sorted({"死气","官符","大耗","勾陈","木马","专日"}));
	EXPECT_EQ(sorted(xieji.ji),
			  sorted({"开市","修置产室","畋猎","开仓","取鱼"}));
	EXPECT_EQ(xieji.yi_ji_rule,"从宜亦从忌");
}

TEST(AlmanacRules, TraditionalProfilesUseDateLevelSolarTermBoundary){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	for(HliProfileCode profile :
		{HliProfileCode::Folk,HliProfileCode::XieJi}){
		lunar::core::DayComputeOptions opt=make_day_opt("2024-02-04");
		opt.hli_rules=make_hli_rule_set(profile);
		DayResult day=lunar::core::compute_day(opt);
		const HliData&h=day.at_data.hli;
		EXPECT_EQ(h.m_gz.text,"丙寅");
		EXPECT_EQ(h.y_rule.text,"癸卯");
		EXPECT_EQ(h.rule_profile_code,static_cast<int>(profile));
	}
}

TEST(AlmanacRules, FolkAndZiPingKeepDistinctYearBoundary){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	lunar::core::DayComputeOptions folk_opt=make_day_opt("2025-01-31");
	folk_opt.hli_rules=make_hli_rule_set(HliProfileCode::Folk);
	lunar::core::DayComputeOptions ziping_opt=folk_opt;
	ziping_opt.hli_rules=make_hli_rule_set(HliProfileCode::ZiPing);

	DayResult folk=lunar::core::compute_day(folk_opt);
	DayResult ziping=lunar::core::compute_day(ziping_opt);
	EXPECT_EQ(folk.at_data.hli.rule_profile_code,
			  static_cast<int>(HliProfileCode::Folk));
	EXPECT_EQ(ziping.at_data.hli.rule_profile_code,
			  static_cast<int>(HliProfileCode::ZiPing));
	EXPECT_NE(folk.at_data.hli.y_rule.text,ziping.at_data.hli.y_rule.text);
	EXPECT_EQ(folk.at_data.hli.m_gz.text,ziping.at_data.hli.m_gz.text);
}

TEST(AlmanacRules, DayBoundaryChangesDayGanzhi){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	lunar::core::DayComputeOptions hour23_opt=make_day_opt("2025-09-07");
	hour23_opt.at_time="23:30:00";
	hour23_opt.hli_rules=make_hli_rule_set(HliProfileCode::Folk);

	lunar::core::DayComputeOptions hour0_opt=hour23_opt;
	hour0_opt.hli_rules=hour23_opt.hli_rules;
	hour0_opt.hli_rules.day_boundary=static_cast<int>(HliDayBoundary::Hour0);
	hour0_opt.hli_rules=normalize_hli_rule_set(hour0_opt.hli_rules);

	DayResult hour23=lunar::core::compute_day(hour23_opt);
	DayResult hour0=lunar::core::compute_day(hour0_opt);
	EXPECT_NE(hour23.at_data.hli.day_boundary_code,hour0.at_data.hli.day_boundary_code);
	EXPECT_NE(hour23.at_data.hli.d_gz.text,hour0.at_data.hli.d_gz.text);
	EXPECT_EQ(hour0.at_data.hli.rule_profile_code,
			  static_cast<int>(HliProfileCode::Custom));
}

TEST(AlmanacRules, LeapModeChangesLeapMonthGanzhi){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	lunar::core::DayComputeOptions folk_opt=make_day_opt("2025-08-10");
	folk_opt.hli_rules=make_hli_rule_set(HliProfileCode::Folk);
	lunar::core::DayComputeOptions purple_opt=folk_opt;
	purple_opt.hli_rules=make_hli_rule_set(HliProfileCode::PurpleStar);

	DayResult folk=lunar::core::compute_day(folk_opt);
	DayResult purple=lunar::core::compute_day(purple_opt);
	EXPECT_NE(folk_opt.hli_rules.leap_month_mode,
			  purple_opt.hli_rules.leap_month_mode);
	EXPECT_NE(folk.at_data.hli.leap_month_mode_code,
			  purple.at_data.hli.leap_month_mode_code);
	EXPECT_EQ(folk.at_data.hli.leap_month_mode_code,
			  folk_opt.hli_rules.leap_month_mode);
	EXPECT_EQ(purple.at_data.hli.leap_month_mode_code,
			  purple_opt.hli_rules.leap_month_mode);
	EXPECT_EQ(purple.at_data.hli.rule_profile_code,
			  static_cast<int>(HliProfileCode::PurpleStar));
}

TEST(AlmanacRules, AliasesNormalizeCorrectly){
	HliProfileCode p0=HliProfileCode::Custom;
	HliProfileCode p1=HliProfileCode::Custom;
	HliYearBoundary year_boundary=HliYearBoundary::WinterSolstice;
	HliMonthBoundary month_boundary=HliMonthBoundary::SolarTerm;
	HliLeapMonthMode leap_mode=HliLeapMonthMode::Ignore;
	HliDayBoundary day_boundary=HliDayBoundary::Hour0;
	ASSERT_TRUE(parse_hli_profile("老黄历",&p0));
	ASSERT_TRUE(parse_hli_profile("四柱八字",&p1));
	ASSERT_TRUE(parse_hli_year_boundary("春节",&year_boundary));
	ASSERT_TRUE(parse_hli_month_boundary("朔日",&month_boundary));
	ASSERT_TRUE(parse_hli_leap_month_mode("闰月中分",&leap_mode));
	ASSERT_TRUE(parse_hli_day_boundary("午夜",&day_boundary));
	EXPECT_EQ(p0,HliProfileCode::Folk);
	EXPECT_EQ(p1,HliProfileCode::ZiPing);
	EXPECT_EQ(year_boundary,HliYearBoundary::LunarNewYear);
	EXPECT_EQ(month_boundary,HliMonthBoundary::LunarFirstDay);
	EXPECT_EQ(leap_mode,HliLeapMonthMode::SplitMidway);
	EXPECT_EQ(day_boundary,HliDayBoundary::Hour0);
}

TEST(AlmanacI18n, CatalogCoverageStaysClean){
	const std::array<lunar::i18n::Lang,5> langs={{
		lunar::i18n::Lang::Zh,
		lunar::i18n::Lang::ZhHant,
		lunar::i18n::Lang::En,
		lunar::i18n::Lang::Ja,
		lunar::i18n::Lang::Ko,
	}};

	const lunar::i18n::Lang old=lunar::i18n::current_lang();
	bool ok=true;
	for(lunar::i18n::Lang lang : langs){
		lunar::i18n::set_lang(lang);

		HliData catalog=make_hli_sample();
		lunar::i18n::localize_hli(&catalog);
		ok=ok&&clean_text(catalog.rule_profile)&&
		   clean_text(catalog.year_boundary_text)&&
		   clean_text(catalog.month_boundary_text)&&
		   clean_text(catalog.leap_month_mode_text)&&
		   clean_text(catalog.day_boundary_text)&&
		   clean_text(catalog.duty_god)&&clean_text(catalog.duty_tag)&&
		   clean_text(catalog.pengzu)&&clean_text(catalog.nayin)&&
		   clean_text(catalog.wx_day)&&clean_text(catalog.fetal_god)&&
		   clean_text(catalog.meridian)&&clean_text(catalog.lucky_dir)&&
		   clean_text(catalog.wealth_dir)&&clean_text(catalog.mascot_dir)&&
		   clean_text(catalog.sun_noble_dir)&&
		   clean_text(catalog.moon_noble_dir)&&clean_text(catalog.xiu28)&&
		   clean_text(catalog.xiu28_mod28)&&clean_text(catalog.xiu_id)&&
		   clean_text(catalog.yi_ji_rule)&&
		   catalog.good_gods.size()==catalog.good_god_codes.size()&&
		   catalog.bad_gods.size()==catalog.bad_god_codes.size()&&
		   catalog.yi.size()==catalog.yi_codes.size()&&
		   catalog.ji.size()==catalog.ji_codes.size()&&
		   all_non_empty(catalog.good_gods)&&all_non_empty(catalog.bad_gods)&&
		   all_non_empty(catalog.yi)&&all_non_empty(catalog.ji)&&
		   all_clean(catalog.good_gods)&&all_clean(catalog.bad_gods)&&
		   all_clean(catalog.yi)&&all_clean(catalog.ji)&&
		   catalog.hour_jx.size()==1&&
		   clean_text(catalog.hour_jx.front().slot)&&
		   clean_text(catalog.hour_jx.front().gz)&&
		   clean_text(catalog.hour_jx.front().luck);

		lunar::MoonXg moon_xg;
		moon_xg.region="危宿";
		moon_xg.star_id="HR8414";
		moon_xg.star_name="Sadalmelik";
		lunar::i18n::localize_moon_xg(&moon_xg);
		ok=ok&&clean_text(moon_xg.region)&&clean_text(moon_xg.star_name);

		lunar::MoonXg moon_xg_ph;
		moon_xg_ph.region="紫微垣";
		moon_xg_ph.star_id="HR4905";
		moon_xg_ph.star_name="Alioth";
		lunar::i18n::localize_moon_xg(&moon_xg_ph);
		ok=ok&&clean_text(moon_xg_ph.region)&&clean_text(moon_xg_ph.star_name);

		ok=ok&&clean_text(lunar::i18n::tr_body_name(10))&&
		   clean_text(lunar::i18n::tr_body_name(301))&&
		   clean_text(lunar::i18n::tr_star_name_text("Spica"))&&
		   clean_text(lunar::i18n::tr_star_region_text("角宿"))&&
		   clean_text(lunar::i18n::tr_star_region_text("紫微垣"));

		for(int code=0;code<=static_cast<int>(HliJianchuCode::Close);++code){
			HliData h=make_hli_sample();
			h.jianchu_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.jianchu);
		}
		for(int code=0;code<=static_cast<int>(HliXiuCode::Zhen);++code){
			HliData h=make_hli_sample();
			h.xiu28_code=code;
			h.xiu28_mod28_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.xiu28)&&clean_text(h.xiu28_mod28);
		}
		for(int code=0;code<30;++code){
			HliData h=make_hli_sample();
			h.nayin_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.nayin);
		}
		for(int code=0;code<60;++code){
			HliData h=make_hli_sample();
			h.fetal_god_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.fetal_god);
		}
		for(int code=0;code<12;++code){
			HliData h=make_hli_sample();
			h.meridian_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.meridian);
		}
		for(int code=0;code<8;++code){
			HliData h=make_hli_sample();
			h.lucky_dir_code=code;
			h.wealth_dir_code=code;
			h.mascot_dir_code=code;
			h.sun_noble_dir_code=code;
			h.moon_noble_dir_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.lucky_dir)&&clean_text(h.wealth_dir)&&
			   clean_text(h.mascot_dir)&&clean_text(h.sun_noble_dir)&&
			   clean_text(h.moon_noble_dir);
		}
		for(int code=0;code<=static_cast<int>(HliProfileCode::Custom);++code){
			HliData h=make_hli_sample();
			h.rule_profile_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.rule_profile);
		}
		for(int code=0;code<=static_cast<int>(HliYearBoundary::WinterSolstice);++code){
			HliData h=make_hli_sample();
			h.year_boundary_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.year_boundary_text);
		}
		for(int code=0;code<=static_cast<int>(HliMonthBoundary::LunarFirstDay);++code){
			HliData h=make_hli_sample();
			h.month_boundary_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.month_boundary_text);
		}
		for(int code=0;code<=static_cast<int>(HliLeapMonthMode::ShiftToNext);++code){
			HliData h=make_hli_sample();
			h.leap_month_mode_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.leap_month_mode_text);
		}
		for(int code=0;code<=static_cast<int>(HliDayBoundary::Hour0);++code){
			HliData h=make_hli_sample();
			h.day_boundary_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.day_boundary_text);
		}
		for(int code=0;code<=static_cast<int>(HliRuleCode::AvoidEverything);++code){
			HliData h=make_hli_sample();
			h.yi_ji_rule_code=code;
			lunar::i18n::localize_hli(&h);
			ok=ok&&clean_text(h.yi_ji_rule);
		}
		if(!ok){
			break;
		}
	}
	lunar::i18n::set_lang(old);
	EXPECT_TRUE(ok);
}
