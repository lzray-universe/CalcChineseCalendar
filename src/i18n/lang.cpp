#include "lunar/i18n.hpp"

#include<algorithm>
#include<array>
#include<cctype>
#include<stdexcept>
#include<string>
#include<utility>

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

void replace_all(std::string&text,const std::string&from,const std::string&to){
	if(from.empty()){
		return;
	}
	std::size_t pos=0;
	while((pos=text.find(from,pos))!=std::string::npos){
		text.replace(pos,from.size(),to);
		pos+=to.size();
	}
}

const std::array<std::pair<const char*,const char*>,191> kZhHantMap={{
	{"帮","幫"},{"毕","畢"},{"闭","閉"},{"边","邊"},{"变","變"},{"并","並"},
	{"补","補"},{"财","財"},{"参","參"},{"仓","倉"},{"厕","廁"},{"钗","釵"},
	{"产","產"},
	{"肠","腸"},{"尝","嘗"},{"车","車"},{"陈","陳"},{"冲","沖"},{"厨","廚"},
	{"处","處"},{"传","傳"},{"钏","釧"},{"疮","瘡"},{"词","詞"},{"赐","賜"},
	{"从","從"},{"错","錯"},{"带","帶"},{"单","單"},{"胆","膽"},{"当","當"},
	{"灯","燈"},{"敌","敵"},{"颠","顛"},{"东","東"},{"对","對"},{"断","斷"},
	{"锋","鋒"},{"抚","撫"},{"盖","蓋"},{"规","規"},{"贵","貴"},{"还","還"},
	{"号","號"},{"坏","壞"},{"环","環"},{"黄","黃"},{"会","會"},{"祸","禍"},
	{"击","擊"},{"机","機"},{"鸡","雞"},{"计","計"},{"继","繼"},{"间","間"},
	{"检","檢"},{"见","見"},{"剑","劍"},{"涧","澗"},{"键","鍵"},{"将","將"},
	{"酱","醬"},{"脚","腳"},{"阶","階"},{"节","節"},{"结","結"},{"仅","僅"},
	{"进","進"},
	{"经","經"},{"惊","驚"},{"径","徑"},{"旧","舊"},{"举","舉"},{"据","據"},
	{"绝","絕"},{"开","開"},{"库","庫"},{"匮","匱"},{"腊","臘"},{"蜡","蠟"},
	{"类","類"},{"离","離"},{"历","歷"},{"录","錄"},{"雳","靂"},{"疗","療"},
	{"猎","獵"},{"邻","鄰"},{"临","臨"},{"龙","龍"},{"娄","婁"},{"炉","爐"},
	{"络","絡"},{"马","馬"},{"码","碼"},{"满","滿"},{"门","門"},
	{"纳","納"},{"难","難"},{"内","內"},{"酿","釀"},{"农","農"},{"栖","棲"},
	{"启","啟"},{"气","氣"},{"强","強"},{"墙","墻"},{"请","請"},{"庆","慶"},
	{"区","區"},{"认","認"},{"闰","閏"},{"丧","喪"},{"扫","掃"},{"肾","腎"},
	{"师","師"},{"时","時"},{"视","視"},{"试","試"},{"饰","飾"},{"输","輸"},
	{"属","屬"},{"竖","豎"},{"网","網"},
	{"数","數"},{"双","雙"},{"讼","訟"},{"诉","訴"},{"岁","歲"},{"台","臺"},
	{"体","體"},{"统","統"},{"头","頭"},{"图","圖"},{"围","圍"},{"为","為"},
	{"问","問"},{"乌","烏"},{"无","無"},{"贤","賢"},{"显","顯"},{"乡","鄉"},
	{"响","響"},{"项","項"},{"写","寫"},{"谢","謝"},{"凶","兇"},{"虚","虛"},
	{"续","續"},{"选","選"},{"学","學"},{"询","詢"},{"阳","陽"},{"杨","楊"},
	{"养","養"},{"药","藥"},{"医","醫"},{"应","應"},{"驿","驛"},{"营","營"},
	{"鱼","魚"},{"与","與"},{"语","語"},{"远","遠"},{"运","運"},{"酝","醞"},
	{"灾","災"},{"载","載"},{"攒","攢"},{"则","則"},{"择","擇"},{"张","張"},
	{"长","長"},{"称","稱"},{"测","測"},{"蛰","蟄"},{"针","針"},{"轸","軫"},
	{"织","織"},{"执","執"},{"种","種"},{"诸","諸"},{"猪","豬"},{"转","轉"},
	{"编","編"},{"范","範"},{"败","敗"},{"过","過"},
	{"换","換"},{"额","額"},{"后","後"},
}};

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
		case Lang::ZhHant:
			return 0;
	}
	return 0;
}

const char*kZhMonthNames[12]={"正月","二月","三月","四月","五月","六月",
							  "七月","八月","九月","十月","十一月","腊月"};

const char*kZhHantMonthNames[12]={"正月","二月","三月","四月","五月","六月",
								  "七月","八月","九月","十月","十一月","臘月"};

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
	if(key=="zht"||key=="zh-tw"||key=="zh-hk"||key=="zh-hant"||key=="tw"||
	   key=="hk"||key=="hant"){
		*out=Lang::ZhHant;
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
			"invalid --lang: "+text+" (expected zh|zht|en|ja|ko)");
	}
	set_lang(parsed);
}

std::string lang_code(Lang lang){
	switch(lang){
		case Lang::Zh:
			return "zh";
		case Lang::ZhHant:
			return "zht";
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

std::string to_zh_hant(const std::string&text){
	std::string out=text;
	for(const auto&item : kZhHantMap){
		replace_all(out,item.first,item.second);
	}
	return out;
}

std::string pick(const char*zh,const char*en,const char*ja,const char*ko,
				 const char*zht){
	Lang lang=current_lang();
	if(lang==Lang::ZhHant){
		if(zht!=nullptr&&zht[0]!='\0'){
			return zht;
		}
		return zh?to_zh_hant(zh):std::string();
	}
	const char*items[4]={zh,en,ja,ko};
	const char*chosen=items[lang_index(lang)];
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
		case Lang::ZhHant:{
			std::string base=kZhHantMonthNames[month_no-1];
			return is_leap?("閏"+base):base;
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
		case Lang::ZhHant:
			return to_zh_hant(zh_lunar_day(day));
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
		case Lang::ZhHant:
			return "農曆"+std::to_string(lunar_year)+"年"+month+day;
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

