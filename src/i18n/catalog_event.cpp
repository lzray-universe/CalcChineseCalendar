#include "lunar/i18n.hpp"

#include<array>
#include<string>

namespace lunar::i18n{

namespace{

struct EventNameItem{
	const char*kind;
	const char*code;
	const char*zh;
	const char*en;
	const char*ja;
	const char*ko;
	const char*zht=nullptr;
};

const std::array<EventNameItem,43> kEventNames={{
	{"solar_term","J1","立春","Start of Spring","立春","??"},
	{"solar_term","Z1","雨水","Rain Water","雨水","??"},
	{"solar_term","J2","惊蛰","Awakening of Insects","啓蟄","??"},
	{"solar_term","Z2","春分","Spring Equinox","春分","??"},
	{"solar_term","J3","清明","Pure Brightness","清明","??"},
	{"solar_term","Z3","谷雨","Grain Rain","穀雨","??"},
	{"solar_term","J4","立夏","Start of Summer","立夏","??"},
	{"solar_term","Z4","小满","Grain Buds","小満","??"},
	{"solar_term","J5","芒种","Grain in Ear","芒種","??"},
	{"solar_term","Z5","夏至","Summer Solstice","夏至","??"},
	{"solar_term","J6","小暑","Minor Heat","小暑","??"},
	{"solar_term","Z6","大暑","Major Heat","大暑","??"},
	{"solar_term","J7","立秋","Start of Autumn","立秋","??"},
	{"solar_term","Z7","处暑","Limit of Heat","処暑","??"},
	{"solar_term","J8","白露","White Dew","白露","??"},
	{"solar_term","Z8","秋分","Autumn Equinox","秋分","??"},
	{"solar_term","J9","寒露","Cold Dew","寒露","??"},
	{"solar_term","Z9","霜降","Frost Descent","霜降","??"},
	{"solar_term","J10","立冬","Start of Winter","立冬","??"},
	{"solar_term","Z10","小雪","Minor Snow","小雪","??"},
	{"solar_term","J11","大雪","Major Snow","大雪","??"},
	{"solar_term","Z11","冬至","Winter Solstice","冬至","??"},
	{"solar_term","J12","小寒","Minor Cold","小寒","??"},
	{"solar_term","Z12","大寒","Major Cold","大寒","??"},
	{"lunar_phase","new_moon","朔","New Moon","新月","?"},
	{"lunar_phase","fst_qtr","上弦","First Quarter","上弦","??"},
	{"lunar_phase","full_moon","望","Full Moon","満月","?"},
	{"lunar_phase","lst_qtr","下弦","Last Quarter","下弦","??"},
	{"lunar_eclipse","total","月全食","Total Lunar Eclipse","皆既月食","????"},
	{"lunar_eclipse","partial","月偏食","Partial Lunar Eclipse","部分月食","????"},
	{"lunar_eclipse","penumbral","半影月食","Penumbral Lunar Eclipse","半影月食","????"},
	{"solar_eclipse","total","日全食","Total Solar Eclipse","皆既日食","????"},
	{"solar_eclipse","partial","日偏食","Partial Solar Eclipse","部分日食","????"},
	{"solar_eclipse","annular","日环食","Annular Solar Eclipse","金環日食","????"},
	{"solar_eclipse","hybrid","全环食","Hybrid Solar Eclipse","金環皆既日食","????"},
	{"festival","1-1","春节","Spring Festival","春節","??"},
	{"festival","1-15","元宵","Lantern Festival","元宵節","?????"},
	{"festival","5-5","端午","Dragon Boat Festival","端午節","??"},
	{"festival","7-7","七夕","Qixi Festival","七夕","??"},
	{"festival","8-15","中秋","Mid-Autumn Festival","中秋節","??"},
	{"festival","9-9","重阳","Double Ninth Festival","重陽節","???"},
	{"festival","12-8","腊八","Laba Festival","臘八","??"},
	{"festival","12-last","除夕","Chinese New Year's Eve","大晦日","????"},
}};

std::string pick_name(const EventNameItem&item){
	switch(current_lang()){
		case Lang::Zh:
			return item.zh;
		case Lang::ZhHant:
			if(item.zht&&item.zht[0]!='\0'){
				return item.zht;
			}
			return to_zh_hant(item.zh);
		case Lang::En:
			return item.en;
		case Lang::Ja:
			return item.ja;
		case Lang::Ko:
			return item.ko;
	}
	return item.zh;
}

}

std::string tr_event_name(const std::string&kind,const std::string&code,
						  const std::string&fallback){
	for(const auto&item : kEventNames){
		if(kind==item.kind&&code==item.code){
			return pick_name(item);
		}
	}
	return fallback;
}

}
