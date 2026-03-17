#include "lunar/i18n.hpp"

#include<string>

#include "lunar/star.hpp"

namespace lunar::i18n{

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
	auto nz=[](const char*s) -> const char*{ return s?s:""; };
	auto pick_non_empty=[&](const char*s0,const char*s1,const char*s2,
							const char*s3) -> std::string{
		const char*items[4]={s0,s1,s2,s3};
		for(const char*item : items){
			if(item&&item[0]!='\0'){
				return item;
			}
		}
		return "";
	};
	switch(current_lang()){
		case Lang::Zh:
			return pick_non_empty(nz(st.zh),nz(st.en),nz(st.id),nullptr);
		case Lang::ZhHant:{
			std::string zh=pick_non_empty(nz(st.zh),nz(st.en),nz(st.id),nullptr);
			return to_zh_hant(zh);
		}
		case Lang::En:
			return pick_non_empty(nz(st.en),nz(st.id),nz(st.zh),nullptr);
		case Lang::Ja:
			return pick_non_empty(nz(st.ja),nz(st.zh),nz(st.en),nz(st.id));
		case Lang::Ko:
			return pick_non_empty(nz(st.ko),nz(st.zh),nz(st.en),nz(st.id));
	}
	return pick_non_empty(nz(st.zh),nz(st.en),nz(st.id),nullptr);
}

}
