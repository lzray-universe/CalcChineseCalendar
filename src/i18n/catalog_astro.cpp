#include "lunar/i18n.hpp"

#include<array>
#include<cstddef>
#include<stdexcept>
#include<string>
#include<utility>

#include "lunar/star.hpp"

namespace lunar::i18n{

namespace{

struct RegionText{
	const char*zh;
	const char*en;
	const char*ja;
	const char*ko;
	const char*zht;
};

bool has_ascii_qmark(const char*text){
	if(text==nullptr){
		return false;
	}
	for(const char*p=text;*p!='\0';++p){
		if(*p=='?'){
			return true;
		}
	}
	return false;
}

std::string pick_clean(const char*s0,const char*s1,const char*s2,
					   const char*s3){
	const char*items[4]={s0,s1,s2,s3};
	for(const char*item : items){
		if(item!=nullptr&&item[0]!='\0'&&!has_ascii_qmark(item)){
			return item;
		}
	}
	return "";
}

bool match_zh_text(const std::string&text,const char*zh){
	if(zh==nullptr||zh[0]=='\0'){
		return false;
	}
	return text==zh||text==to_zh_hant(zh);
}

const StarRecord* find_star_text(const std::string&text){
	if(text.empty()){
		return nullptr;
	}
	auto eq=[&](const char*s) -> bool{
		return s!=nullptr&&s[0]!='\0'&&text==s;
	};
	for(std::size_t i=0;i<B_STARS_COUNT;++i){
		const StarRecord&st=B_STARS[i];
		if(eq(st.id)||eq(st.en)||eq(st.zh)||eq(st.ja)||eq(st.ko)){
			return &st;
		}
		if(match_zh_text(text,st.zh)){
			return &st;
		}
	}
	return nullptr;
}

std::string pick_region(const RegionText&text){
	switch(current_lang()){
		case Lang::Zh:
			return text.zh;
		case Lang::ZhHant:
			return text.zht!=nullptr&&text.zht[0]!='\0'
					   ?text.zht
					   :to_zh_hant(text.zh);
		case Lang::En:
			return text.en;
		case Lang::Ja:
			return text.ja!=nullptr&&text.ja[0]!='\0' ? text.ja : text.zh;
		case Lang::Ko:
			return text.ko!=nullptr&&text.ko[0]!='\0' ? text.ko : text.zh;
	}
	return text.zh;
}

const RegionText* find_region_text(const std::string&text){
	static const std::array<std::pair<const char*,RegionText>,29> kRegions={{
		{"角宿",{"角宿","Jiao Mansion","角宿","角宿","角宿"}},
		{"亢宿",{"亢宿","Kang Mansion","亢宿","亢宿","亢宿"}},
		{"氐宿",{"氐宿","Di Mansion","氐宿","氐宿","氐宿"}},
		{"房宿",{"房宿","Fang Mansion","房宿","房宿","房宿"}},
		{"心宿",{"心宿","Xin Mansion","心宿","心宿","心宿"}},
		{"尾宿",{"尾宿","Wei Mansion","尾宿","尾宿","尾宿"}},
		{"箕宿",{"箕宿","Ji Mansion","箕宿","箕宿","箕宿"}},
		{"斗宿",{"斗宿","Dou Mansion","斗宿","斗宿","斗宿"}},
		{"牛宿",{"牛宿","Niu Mansion","牛宿","牛宿","牛宿"}},
		{"女宿",{"女宿","Nu Mansion","女宿","女宿","女宿"}},
		{"虚宿",{"虚宿","Xu Mansion","虚宿","虚宿","虛宿"}},
		{"危宿",{"危宿","Wei Mansion","危宿","危宿","危宿"}},
		{"室宿",{"室宿","Shi Mansion","室宿","室宿","室宿"}},
		{"壁宿",{"壁宿","Bi Mansion","壁宿","壁宿","壁宿"}},
		{"奎宿",{"奎宿","Kui Mansion","奎宿","奎宿","奎宿"}},
		{"娄宿",{"娄宿","Lou Mansion","婁宿","婁宿","婁宿"}},
		{"胃宿",{"胃宿","Wei Mansion","胃宿","胃宿","胃宿"}},
		{"昴宿",{"昴宿","Mao Mansion","昴宿","昴宿","昴宿"}},
		{"毕宿",{"毕宿","Bi Mansion","畢宿","畢宿","畢宿"}},
		{"觜宿",{"觜宿","Zui Mansion","觜宿","觜宿","觜宿"}},
		{"参宿",{"参宿","Shen Mansion","参宿","参宿","參宿"}},
		{"井宿",{"井宿","Jing Mansion","井宿","井宿","井宿"}},
		{"鬼宿",{"鬼宿","Gui Mansion","鬼宿","鬼宿","鬼宿"}},
		{"柳宿",{"柳宿","Liu Mansion","柳宿","柳宿","柳宿"}},
		{"星宿",{"星宿","Xing Mansion","星宿","星宿","星宿"}},
		{"张宿",{"张宿","Zhang Mansion","張宿","張宿","張宿"}},
		{"翼宿",{"翼宿","Yi Mansion","翼宿","翼宿","翼宿"}},
		{"轸宿",{"轸宿","Zhen Mansion","軫宿","軫宿","軫宿"}},
		{"紫微垣",{"紫微垣","Purple Forbidden Enclosure","紫微垣","자미원","紫微垣"}},
	}};
	for(const auto&item : kRegions){
		if(match_zh_text(text,item.first)){
			return &item.second;
		}
	}
	return nullptr;
}

}

std::string tr_body_name(int id){
	switch(id){
		case 10:
			return pick("太阳","Sun","太陽","태양","太陽");
		case 199:
			return pick("水星","Mercury","水星","수성","水星");
		case 299:
			return pick("金星","Venus","金星","금성","金星");
		case 301:
			return pick("月球","Moon","月","달","月球");
		case 499:
			return pick("火星","Mars","火星","화성","火星");
		case 599:
			return pick("木星","Jupiter","木星","목성","木星");
		case 699:
			return pick("土星","Saturn","土星","토성","土星");
		case 799:
			return pick("天王星","Uranus","天王星","천왕성","天王星");
		case 899:
			return pick("海王星","Neptune","海王星","해왕성","海王星");
		default:
			return pick("天体","Celestial Body","天体","천체","天體");
	}
}

std::string tr_star_name(const StarRecord&st){
	switch(current_lang()){
		case Lang::Zh:
			return pick_clean(st.zh,st.en,st.id,nullptr);
		case Lang::ZhHant:{
			std::string zh=pick_clean(st.zh,st.en,st.id,nullptr);
			return to_zh_hant(zh);
		}
		case Lang::En:
			return pick_clean(st.en,st.id,st.zh,nullptr);
		case Lang::Ja:
			return pick_clean(st.ja,st.zh,st.en,st.id);
		case Lang::Ko:
			return pick_clean(st.ko,st.zh,st.en,st.id);
	}
	return pick_clean(st.zh,st.en,st.id,nullptr);
}

std::string tr_star_name_text(const std::string&text){
	const StarRecord*st=find_star_text(text);
	return st!=nullptr ? tr_star_name(*st) : text;
}

std::string tr_star_region_text(const std::string&text){
	const RegionText*region=find_region_text(text);
	if(region!=nullptr){
		return pick_region(*region);
	}
	if(current_lang()==Lang::ZhHant){
		return to_zh_hant(text);
	}
	return text;
}

void localize_moon_xg(lunar::MoonXg*data){
	if(data==nullptr){
		throw std::invalid_argument("localize_moon_xg requires non-null data");
	}

	const StarRecord*st=find_star_text(data->star_id);
	if(st==nullptr){
		st=find_star_text(data->star_name);
	}
	if(st!=nullptr){
		data->region=
			st->region!=nullptr&&st->region[0]!='\0'
				?tr_star_region_text(st->region)
				:tr_star_region_text(data->region);
		data->star_name=tr_star_name(*st);
		return;
	}

	data->region=tr_star_region_text(data->region);
	data->star_name=tr_star_name_text(data->star_name);
}

}
