#pragma once

#include<string>

namespace lunar{
struct StarRecord;
}

namespace lunar::i18n{

enum class Lang{
	Zh,
	En,
	Ja,
	Ko,
};

Lang current_lang();

void set_lang(Lang lang);

bool try_parse_lang(const std::string&text,Lang*out);

void set_lang_from_text(const std::string&text);

std::string lang_code(Lang lang);

std::string current_lang_code();

std::string pick(const char*zh,const char*en,const char*ja,const char*ko);

std::string tz_note();

std::string day_rule_note();

std::string tr_event_name(const std::string&kind,const std::string&code,
						  const std::string&fallback="");

std::string tr_body_name(int id);

std::string tr_star_name(const StarRecord&st);

std::string tr_lunar_month(int month_no,bool is_leap,
						   const std::string&fallback="");

std::string tr_lunar_day(int day,const std::string&fallback="");

std::string tr_lunar_label(int lunar_year,int month_no,bool is_leap,int lunar_day,
						   const std::string&fallback="");

}

