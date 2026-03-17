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
	{"solar_term","J1","立春","Start of Spring","立春","입춘"},
	{"solar_term","Z1","雨水","Rain Water","雨水","우수"},
	{"solar_term","J2","惊蛰","Awakening of Insects","啓蟄","경칩"},
	{"solar_term","Z2","春分","Spring Equinox","春分","춘분"},
	{"solar_term","J3","清明","Clear and Bright","清明","청명"},
	{"solar_term","Z3","谷雨","Grain Rain","穀雨","곡우"},
	{"solar_term","J4","立夏","Start of Summer","立夏","입하"},
	{"solar_term","Z4","小满","Grain Full","小満","소만"},
	{"solar_term","J5","芒种","Grain in Ear","芒種","망종"},
	{"solar_term","Z5","夏至","Summer Solstice","夏至","하지"},
	{"solar_term","J6","小暑","Minor Heat","小暑","소서"},
	{"solar_term","Z6","大暑","Major Heat","大暑","대서"},
	{"solar_term","J7","立秋","Start of Autumn","立秋","입추"},
	{"solar_term","Z7","处暑","End of Heat","処暑","처서"},
	{"solar_term","J8","白露","White Dew","白露","백로"},
	{"solar_term","Z8","秋分","Autumn Equinox","秋分","추분"},
	{"solar_term","J9","寒露","Cold Dew","寒露","한로"},
	{"solar_term","Z9","霜降","Frost Descent","霜降","상강"},
	{"solar_term","J10","立冬","Start of Winter","立冬","입동"},
	{"solar_term","Z10","小雪","Minor Snow","小雪","소설"},
	{"solar_term","J11","大雪","Major Snow","大雪","대설"},
	{"solar_term","Z11","冬至","Winter Solstice","冬至","동지"},
	{"solar_term","J12","小寒","Minor Cold","小寒","소한"},
	{"solar_term","Z12","大寒","Major Cold","大寒","대한"},
	{"lunar_phase","new_moon","朔","New Moon","新月","삭"},
	{"lunar_phase","fst_qtr","上弦","First Quarter","上弦","상현"},
	{"lunar_phase","full_moon","望","Full Moon","満月","망"},
	{"lunar_phase","lst_qtr","下弦","Last Quarter","下弦","하현"},
	{"lunar_eclipse","total","月全食","Total Lunar Eclipse","皆既月食","개기월식"},
	{"lunar_eclipse","partial","月偏食","Partial Lunar Eclipse","部分月食","부분월식"},
	{"lunar_eclipse","penumbral","半影月食","Penumbral Lunar Eclipse","半影月食","반영월식"},
	{"solar_eclipse","total","日全食","Total Solar Eclipse","皆既日食","개기일식"},
	{"solar_eclipse","partial","日偏食","Partial Solar Eclipse","部分日食","부분일식"},
	{"solar_eclipse","annular","日环食","Annular Solar Eclipse","金環日食","금환일식"},
	{"solar_eclipse","hybrid","全环食","Hybrid Solar Eclipse","金環皆既日食","혼성일식"},
	{"festival","1-1","春节","Spring Festival","春節","설날"},
	{"festival","1-15","元宵","Lantern Festival","元宵節","정월대보름"},
	{"festival","5-5","端午","Dragon Boat Festival","端午節","단오"},
	{"festival","7-7","七夕","Qixi Festival","七夕","칠석"},
	{"festival","8-15","中秋","Mid-Autumn Festival","中秋節","추석"},
	{"festival","9-9","重阳","Double Ninth Festival","重陽節","중양절"},
	{"festival","12-8","腊八","Laba Festival","臘八","납팔"},
	{"festival","12-last","除夕","Chinese New Year's Eve","大晦日","섣달그믐"},
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
