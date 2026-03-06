#include "lunar/i18n.hpp"

#include<algorithm>
#include<cctype>
#include<stdexcept>
#include<string>

namespace lunar::i18n{

namespace{

thread_local Lang g_lang=Lang::Zh;

std::string low_ascii_copy(const std::string&text){
	std::string out=text;
	std::transform(out.begin(),out.end(),out.begin(),[](unsigned char c){
		return static_cast<char>(std::tolower(c));
	});
	return out;
}

int lang_index(Lang lang){
	switch(lang){
		case Lang::Zh:
			return 0;
		case Lang::En:
			return 1;
		case Lang::Ja:
			return 2;
		case Lang::Ko:
			return 3;
	}
	return 0;
}

const char*kZhMonthNames[12]={"正月","二月","三月","四月","五月","六月",
							  "七月","八月","九月","十月","十一月","腊月"};

std::string zh_lunar_day(int day){
	static const char*units[10]={"一","二","三","四","五","六","七","八","九","十"};
	if(day<1||day>30){
		return std::to_string(day);
	}
	if(day<=10){
		return std::string("初")+units[day-1];
	}
	if(day<20){
		return std::string("十")+units[day-11];
	}
	if(day==20){
		return "二十";
	}
	if(day<30){
		return std::string("廿")+units[day-21];
	}
	return "三十";
}

}

Lang current_lang(){ return g_lang; }

void set_lang(Lang lang){ g_lang=lang; }

bool try_parse_lang(const std::string&text,Lang*out){
	if(out==nullptr){
		return false;
	}
	std::string key=low_ascii_copy(text);
	if(key=="zh"||key=="zh-cn"||key=="cn"||key=="zh-hans"){
		*out=Lang::Zh;
		return true;
	}
	if(key=="en"||key=="en-us"||key=="us"){
		*out=Lang::En;
		return true;
	}
	if(key=="ja"||key=="ja-jp"||key=="jp"){
		*out=Lang::Ja;
		return true;
	}
	if(key=="ko"||key=="ko-kr"||key=="kr"){
		*out=Lang::Ko;
		return true;
	}
	return false;
}

void set_lang_from_text(const std::string&text){
	Lang parsed=Lang::Zh;
	if(!try_parse_lang(text,&parsed)){
		throw std::invalid_argument(
			"invalid --lang: "+text+" (expected zh|en|ja|ko)");
	}
	set_lang(parsed);
}

std::string lang_code(Lang lang){
	switch(lang){
		case Lang::Zh:
			return "zh";
		case Lang::En:
			return "en";
		case Lang::Ja:
			return "ja";
		case Lang::Ko:
			return "ko";
	}
	return "zh";
}

std::string current_lang_code(){ return lang_code(current_lang()); }

std::string pick(const char*zh,const char*en,const char*ja,const char*ko){
	const char*items[4]={zh,en,ja,ko};
	const char*chosen=items[lang_index(current_lang())];
	return chosen?std::string(chosen):std::string();
}

std::string tz_note(){
	return pick("--tz仅影响显示，不改变农历规则与计算流程",
				"--tz only affects display formatting and does not change calculation rules.",
				"--tz は表示形式にのみ影響し、暦計算ルールは変わりません。",
				"--tz 는 표시 형식에만 영향을 주며 계산 규칙은 바뀌지 않습니다.");
}

std::string day_rule_note(){
	return pick("农历判日固定按UTC+8民用日执行",
				"Lunar day boundaries are fixed to civil day UTC+8.",
				"旧暦の日付判定は UTC+8 の民用日で固定します。",
				"음력 날짜 판정은 UTC+8 민간 날짜 기준으로 고정됩니다.");
}

std::string tr_lunar_month(int month_no,bool is_leap,const std::string&fallback){
	if(month_no<1||month_no>12){
		return fallback.empty()?std::to_string(month_no):fallback;
	}
	switch(current_lang()){
		case Lang::Zh:{
			std::string base=kZhMonthNames[month_no-1];
			return is_leap?("闰"+base):base;
		}
		case Lang::En:
			return is_leap?("Leap Month "+std::to_string(month_no))
						  :("Month "+std::to_string(month_no));
		case Lang::Ja:
			return is_leap?("閏"+std::to_string(month_no)+"月")
						  :(std::to_string(month_no)+"月");
		case Lang::Ko:
			return is_leap?("윤"+std::to_string(month_no)+"월")
						  :(std::to_string(month_no)+"월");
	}
	return fallback;
}

std::string tr_lunar_day(int day,const std::string&fallback){
	if(day<1||day>30){
		return fallback.empty()?std::to_string(day):fallback;
	}
	switch(current_lang()){
		case Lang::Zh:
			return zh_lunar_day(day);
		case Lang::En:
			return "Day "+std::to_string(day);
		case Lang::Ja:
			return std::to_string(day)+"日";
		case Lang::Ko:
			return std::to_string(day)+"일";
	}
	return fallback;
}

std::string tr_lunar_label(int lunar_year,int month_no,bool is_leap,int lunar_day,
						   const std::string&fallback){
	std::string month=tr_lunar_month(month_no,is_leap);
	std::string day=tr_lunar_day(lunar_day);
	switch(current_lang()){
		case Lang::Zh:
			return "农历"+std::to_string(lunar_year)+"年"+month+day;
		case Lang::En:
			return "Lunar "+std::to_string(lunar_year)+" "+month+" "+day;
		case Lang::Ja:
			return "旧暦"+std::to_string(lunar_year)+"年"+month+day;
		case Lang::Ko:
			return "음력 "+std::to_string(lunar_year)+"년 "+month+" "+day;
	}
	return fallback;
}

}

