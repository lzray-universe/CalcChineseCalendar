struct RuleDayContext{
	int lunar_month=0;
	int lunar_day=0;
	int year_stem=0;
	int year_branch=0;
	int solar_month_branch=0;
	int god_month_branch=0;
	int season=0;
	int month_type=0;
	int day_stem=0;
	int day_branch=0;
	int day60=0;
	int next_solar_num=0;
	int xiu28_cycle=0;
	bool four_separations=false;
	bool four_extinctions=false;
	bool earth_king=false;
	std::string day_gz;
};

std::string_view han_at(std::string_view text,std::size_t index){
	constexpr std::size_t kHanBytes=3;
	std::size_t pos=index*kHanBytes;
	if(pos+kHanBytes>text.size()){
		return {};
	}
	return text.substr(pos,kHanBytes);
}

std::string_view gz_at(std::string_view text,std::size_t index){
	constexpr std::size_t kGzBytes=6;
	std::size_t pos=index*kGzBytes;
	if(pos+kGzBytes>text.size()){
		return {};
	}
	return text.substr(pos,kGzBytes);
}

bool day_has_han(const RuleDayContext&ctx,std::string_view value){
	return ctx.day_gz.find(value)!=std::string::npos;
}

bool month_char_matches(const RuleDayContext&ctx,const char*sequence){
	return day_has_han(ctx,han_at(sequence,
		static_cast<std::size_t>(ctx.god_month_branch)));
}

bool gz_in(std::string_view values,const std::string&day_gz){
	return values.find(day_gz)!=std::string_view::npos;
}

bool branch_in(int branch,std::string_view values){
	return values.find(kBranch[static_cast<std::size_t>(branch)])!=
		   std::string_view::npos;
}

int civil_term_cursor(LunCal6&lc,int year,int month,int day,int tz_off){
	static const std::array<const char*,24> kCodes={{
		"J12","Z12","J1","Z1","J2","Z2","J3","Z3","J4","Z4","J5","Z5",
		"J6","Z6","J7","Z7","J8","Z8","J9","Z9","J10","Z10","J11","Z11"
	}};
	int count=0;
	for(const char*code : kCodes){
		LocalDT term=lc.get_st(code,year);
		int ty=0;
		int tm=0;
		int td=0;
		int th=0;
		int tmin=0;
		double ts=0.0;
		jd_to_loc(term.toUtcJD(),tz_off,ty,tm,td,th,tmin,ts);
		if(std::array<int,3>{ty,tm,td}<=std::array<int,3>{year,month,day}){
			++count;
		}
	}
	return count%24;
}

GzNode month_gz_by_term_cursor(int gregorian_year,int next_solar_num,int month){
	int next=next_solar_num;
	if(next==0&&month==12){
		next=24;
	}
	int apart=(next+1)/2;
	int index=pos_mod(static_cast<long long>(gregorian_year-2019)*12+apart,60);
	return mk_gz_from_idx(index);
}

int xiu_cycle_by_civil_day(int gy,int gm,int gd,int tz_off){
	int current=day_jdn_local_noon(gy,gm,gd,tz_off);
	int anchor=day_jdn_local_noon(2019,1,17,tz_off);
	return pos_mod(static_cast<long long>(current)-anchor,28);
}

int days_to_next_season_term(LunCal6&lc,int gy,int gm,int gd,int tz_off){
	static const std::array<const char*,4> kCodes={{"J1","J4","J7","J10"}};
	int current=day_jdn_local_noon(gy,gm,gd,tz_off);
	int best=1000;
	for(int year : {gy,gy+1}){
		for(const char*code : kCodes){
			LocalDT term=lc.get_st(code,year);
			int ty=0;
			int tm=0;
			int td=0;
			int th=0;
			int tmin=0;
			double ts=0.0;
			jd_to_loc(term.toUtcJD(),tz_off,ty,tm,td,th,tmin,ts);
			int delta=day_jdn_local_noon(ty,tm,td,tz_off)-current;
			if(delta>=1&&delta<best){
				best=delta;
			}
		}
	}
	return best;
}

bool is_long_lunar_month(LunCal6&lc,int gy,int gm,int gd,int tz_off){
	int current=day_jdn_local_noon(gy,gm,gd,tz_off);
	for(int year : {gy,gy-1,gy+1}){
		const auto&months=lc.get_months(year);
		for(const auto&month : months){
			int start=day_jdn_local_noon(
				month.start_dt.year,month.start_dt.month,month.start_dt.day,tz_off);
			int end=day_jdn_local_noon(
				month.end_dt.year,month.end_dt.month,month.end_dt.day,tz_off);
			if(current>=start&&current<end){
				return end-start==30;
			}
		}
	}
	return false;
}

void add_pipe_acts(ActSeq&seq,std::string_view text){
	std::size_t begin=0;
	while(begin<text.size()){
		std::size_t end=text.find('|',begin);
		if(end==std::string_view::npos){
			end=text.size();
		}
		std::string item(text.substr(begin,end-begin));
		if(!item.empty()){
			act_add(seq,act_code(item.c_str()));
		}
		begin=end+1;
	}
}

void apply_god_activity_rules(const GodSeq&gods,ActSeq&yi,ActSeq&ji){
	for(int code : gods.order){
		const char*name=god_text(code);
		for(const auto&row : kGodActivityRules){
			if(std::string_view(row.god)==name){
				add_pipe_acts(yi,row.yi);
				add_pipe_acts(ji,row.ji);
				break;
			}
		}
	}
}

void apply_officer_activity_rules(int officer,ActSeq&yi,ActSeq&ji){
	auto yi_add=[&](std::initializer_list<const char*> values){
		for(const char*value : values){
			act_add(yi,value);
		}
	};
	auto ji_add=[&](std::initializer_list<const char*> values){
		for(const char*value : values){
			act_add(ji,value);
		}
	};
	switch(static_cast<HliJianchuCode>(officer)){
		case HliJianchuCode::Build:
			yi_add({"施恩","招贤","举正直","出行","上官","临政"});
			break;
		case HliJianchuCode::Remove:
			yi_add({"解除","沐浴","整容","剃头","整手足甲","求医疗病","扫舍宇"});
			break;
		case HliJianchuCode::Full:
			yi_add({"进人口","裁制","竖柱上梁","经络","开市","立券交易","纳财",
					"开仓","塞穴","补垣"});
			ji_add({"施恩","招贤","举正直","上官","临政","结婚姻","纳采",
					"求医疗病"});
			break;
		case HliJianchuCode::Balance:
			yi_add({"修饰垣墙","平治道涂"});
			ji_add({"祈福","求嗣","上册","上表章","颁诏","施恩","招贤",
					"举正直","宣政事","布政事","庆赐","宴会","冠带","出行",
					"安抚边境","选将","出师","上官","临政","结婚姻","纳采",
					"嫁娶","进人口","搬移","安床","解除","求医疗病","裁制",
					"营建","修宫室","缮城郭","筑堤防","修造","竖柱上梁",
					"修仓库","鼓铸","经络","酝酿","开市","立券交易","纳财",
					"开仓","修置产室","开渠","穿井","栽种","牧养","纳畜",
					"破土","安葬","启攒"});
			break;
		case HliJianchuCode::Settle:
			yi_add({"冠带"});
			break;
		case HliJianchuCode::Hold:
			yi_add({"捕捉"});
			break;
		case HliJianchuCode::Break:
			yi_add({"求医疗病"});
			break;
		case HliJianchuCode::Danger:
			yi_add({"安抚边境","选将","安床"});
			break;
		case HliJianchuCode::Success:
			yi_add({"入学","安抚边境","搬移","筑堤防","开市"});
			break;
		case HliJianchuCode::Receive:
			yi_add({"进人口","纳财","捕捉","纳畜"});
			ji_add({"祈福","求嗣","上册","上表章","颁诏","施恩","招贤",
					"举正直","宣政事","布政事","庆赐","宴会","冠带","出行",
					"安抚边境","选将","出师","上官","临政","结婚姻","纳采",
					"嫁娶","搬移","安床","解除","求医疗病","裁制","营建",
					"修宫室","缮城郭","筑堤防","修造","竖柱上梁","鼓铸",
					"经络","酝酿","开市","立券交易","开仓","修置产室",
					"开渠","穿井","破土","安葬","启攒"});
			break;
		case HliJianchuCode::Open:
			yi_add({"祭祀","祈福","求嗣","上册","上表章","颁诏","覃恩",
					"施恩","招贤","举正直","恤孤茕","宣政事","雪冤","庆赐",
					"宴会","入学","出行","上官","临政","搬移","解除",
					"求医疗病","裁制","修宫室","缮城郭","修造","修仓库",
					"开市","修置产室","开渠","穿井","安碓硙","栽种","牧养"});
			break;
		case HliJianchuCode::Close:
			yi_add({"筑堤防","塞穴","补垣"});
			ji_add({"上册","上表章","颁诏","施恩","招贤","举正直","宣政事",
					"布政事","庆赐","宴会","出行","出师","上官","临政",
					"结婚姻","纳采","嫁娶","进人口","搬移","安床",
					"求医疗病","疗目","营建","修宫室","修造","竖柱上梁",
					"开市","开仓","修置产室","开渠","穿井"});
			break;
	}
}

void collect_traditional_gods(const RuleDayContext&c,GodSeq&good,GodSeq&bad){
	auto G=[&](const char*name,bool match){
		if(match){
			god_add(good,name);
		}
	};
	auto B=[&](const char*name,bool match){
		if(match){
			god_add(bad,name);
		}
	};
	auto M=[&](const char*seq){ return month_char_matches(c,seq); };

	static const std::array<int,10> kYearDe={{0,6,2,8,4,0,6,2,8,4}};
	static const std::array<int,10> kYearDeHe={{5,1,7,3,9,5,1,7,3,9}};
	G("岁德",c.day_stem==kYearDe[static_cast<std::size_t>(c.year_stem)]);
	G("岁德合",c.day_stem==kYearDeHe[static_cast<std::size_t>(c.year_stem)]);
	G("月德",M("壬庚丙甲壬庚丙甲壬庚丙甲"));
	G("月德合",M("丁乙辛己丁乙辛己丁乙辛己"));
	static const std::array<const char*,12> kTianDe={{
		"巳辰","庚","丁","申未","壬","辛","亥戌","甲","癸","寅丑","丙","乙"
	}};
	std::string_view tian_de=kTianDe[static_cast<std::size_t>(c.god_month_branch)];
	bool tian_de_match=c.month_type==0
		?branch_in(c.day_branch,tian_de)
		:tian_de.find(kStem[static_cast<std::size_t>(c.day_stem)])!=
			 std::string_view::npos;
	G("天德",tian_de_match);
	G("天德合",M("空乙壬空丁丙空己戊空辛庚"));
	static const std::array<int,4> kPhoenix={{11,17,16,18}};
	static const std::array<int,4> kKylin={{21,5,8,13}};
	G("凤凰日",c.xiu28_cycle==kPhoenix[static_cast<std::size_t>(c.season)]);
	G("麒麟日",c.xiu28_cycle==kKylin[static_cast<std::size_t>(c.season)]);
	G("三合",pos_mod(c.day_branch-c.god_month_branch,4)==0);
	static const std::array<const char*,4> kFourPhase={{"丙丁","戊己","壬癸","甲乙"}};
	G("四相",std::string_view(kFourPhase[static_cast<std::size_t>(c.season)])
		.find(kStem[static_cast<std::size_t>(c.day_stem)])!=std::string_view::npos);
	G("五合",branch_in(c.day_branch,"寅卯"));
	G("五富",M("巳申亥寅巳申亥寅巳申亥寅"));
	G("六合",M("丑子亥戌酉申未午巳辰卯寅"));
	G("六仪",M("午巳辰卯寅丑子亥戌酉申未"));
	static const std::array<const char*,12> kBuJiang={{
		"壬寅壬辰辛丑辛卯辛巳庚寅庚辰丁丑丁卯丁巳戊寅戊辰",
		"辛丑辛卯庚子庚寅庚辰丁丑丁卯丙子丙寅丙辰戊子戊寅戊辰",
		"辛亥辛丑辛卯庚子庚寅丁亥丁丑丁卯丙子丙寅戊子戊寅",
		"庚戌庚子庚寅丁亥丁丑丙戌丙子丙寅乙亥乙丑戊戌戊子戊寅",
		"丁酉丁亥丁丑丙戌丙子乙酉乙亥乙丑甲戌甲子戊戌戊子",
		"丁酉丁亥丙申丙戌丙子乙酉乙亥甲申甲戌甲子戊申戊戌戊子",
		"丙申丙戌乙未乙酉乙亥甲申甲戌癸未癸酉癸亥戊申戊戌",
		"乙未乙酉甲午甲申甲戌癸未癸酉壬午壬申壬戌戊午戊申戊戌",
		"乙巳乙未乙酉甲午甲申癸巳癸未癸酉壬午壬申戊午戊申",
		"甲辰甲午甲申癸巳癸未壬辰壬午壬申辛巳辛未戊辰戊午戊申",
		"癸卯癸巳癸未壬辰壬午辛卯辛巳辛未庚辰庚午戊辰戊午",
		"癸卯癸巳壬寅壬辰壬午辛卯辛巳庚寅庚辰庚午戊寅戊辰戊午"
	}};
	G("不将",gz_in(kBuJiang[static_cast<std::size_t>(c.god_month_branch)],c.day_gz));
	static const std::array<int,4> kShiDe={{6,4,0,2}};
	G("时德",c.day_branch==kShiDe[static_cast<std::size_t>(c.season)]);
	G("大葬",gz_in("壬申癸酉壬午甲申乙酉丙申丁酉壬寅丙午己酉庚申辛酉",c.day_gz));
	G("鸣吠",gz_in("庚午壬申癸酉壬午甲申乙酉己酉丙申丁酉壬寅丙午庚寅庚申辛酉",c.day_gz));
	G("小葬",gz_in("庚午壬辰甲辰乙巳甲寅丙辰庚寅",c.day_gz));
	G("鸣吠对",gz_in("丙寅丁卯丙子辛卯甲午庚子癸卯壬子甲寅乙卯",c.day_gz));
	G("不守塚",gz_in("庚午辛未壬申癸酉戊寅己卯壬午癸未甲申乙酉丁未甲午乙未丙申丁酉壬寅癸卯丙午戊申己酉庚申辛酉",c.day_gz));
	static const std::array<int,4> kWang={{2,5,8,11}};
	static const std::array<int,4> kGuan={{3,6,9,0}};
	static const std::array<int,4> kShou={{9,0,3,6}};
	static const std::array<int,4> kXiang={{5,8,11,2}};
	static const std::array<int,4> kMin={{6,9,0,3}};
	G("王日",c.day_branch==kWang[static_cast<std::size_t>(c.season)]);
	G("官日",c.day_branch==kGuan[static_cast<std::size_t>(c.season)]);
	G("守日",c.day_branch==kShou[static_cast<std::size_t>(c.season)]);
	G("相日",c.day_branch==kXiang[static_cast<std::size_t>(c.season)]);
	G("民日",c.day_branch==kMin[static_cast<std::size_t>(c.season)]);
	G("临日",M("辰酉午亥申丑戌卯子巳寅未"));
	static const std::array<const char*,4> kTianGui={{"甲乙","丙丁","庚辛","壬癸"}};
	G("天贵",std::string_view(kTianGui[static_cast<std::size_t>(c.season)])
		.find(kStem[static_cast<std::size_t>(c.day_stem)])!=std::string_view::npos);
	G("天喜",M("申酉戌亥子丑寅卯辰巳午未"));
	G("天富",M("寅卯辰巳午未申酉戌亥子丑"));
	G("天恩",c.day60%15<5&&c.day60/15!=2);
	G("月恩",M("甲辛丙丁庚己戊辛壬癸庚乙"));
	static const std::array<const char*,12> kTianShe={{
		"甲子","甲子","戊寅","戊寅","戊寅","甲午",
		"甲午","甲午","戊申","戊申","戊申","甲子"
	}};
	static const std::array<const char*,12> kTianYuan={{
		"甲子","癸未","甲午","甲戌","乙酉","丙子",
		"丁丑","戊午","甲寅","丙辰","辛卯","戊辰"
	}};
	G("天赦",c.day_gz==kTianShe[static_cast<std::size_t>(c.god_month_branch)]);
	G("天愿",c.day_gz==kTianYuan[static_cast<std::size_t>(c.god_month_branch)]);
	G("天成",M("卯巳未酉亥丑卯巳未酉亥丑"));
	G("天官",M("午申戌子寅辰午申戌子寅辰"));
	G("天医",M("亥子丑寅卯辰巳午未申酉戌"));
	G("天马",M("寅辰午申戌子寅辰午申戌子"));
	G("驿马",M("寅亥申巳寅亥申巳寅亥申巳"));
	G("天财",M("子寅辰午申戌子寅辰午申戌"));
	G("福生",M("寅申酉卯戌辰亥巳子午丑未"));
	static const std::array<int,4> kFuHou={{2,5,8,11}};
	G("福厚",c.day_branch==kFuHou[static_cast<std::size_t>(c.season)]);
	G("福德",M("寅卯辰巳午未申酉戌亥子丑"));
	G("天巫",M("寅卯辰巳午未申酉戌亥子丑"));
	G("地财",M("丑卯巳未酉亥丑卯巳未酉亥"));
	G("月财",M("酉亥午巳巳未酉亥午巳巳未"));
	G("月空",M("丙甲壬庚丙甲壬庚丙甲壬庚"));
	static const std::array<const char*,4> kMother={{"亥子","寅卯","辰丑戌未","申酉"}};
	G("母仓",branch_in(c.day_branch,kMother[static_cast<std::size_t>(c.season)]));
	G("明星",M("辰午甲戌子寅辰午甲戌子寅"));
	G("圣心",M("辰戌亥巳子午丑未寅申卯酉"));
	G("禄库",M("寅卯辰巳午未申酉戌亥子丑"));
	G("吉庆",M("未子酉寅亥辰丑午卯申巳戌"));
	G("阴德",M("丑亥酉未巳卯丑亥酉未巳卯"));
	G("活曜",M("卯申巳戌未子酉寅亥辰丑午"));
	G("除神",branch_in(c.day_branch,"申酉"));
	G("解神",M("午午申申戌戌子子寅寅辰辰"));
	G("生气",M("戌亥子丑寅卯辰巳午未申酉"));
	G("普护",M("丑卯申寅酉卯戌辰亥巳子午"));
	G("益后",M("巳亥子午丑未寅申卯酉辰戌"));
	G("续世",M("午子丑未寅申卯酉辰戌巳亥"));
	G("要安",M("未丑寅申卯酉辰戌巳亥午子"));
	G("天后",M("寅亥申巳寅亥申巳寅亥申巳"));
	G("天仓",M("辰卯寅丑子亥戌酉申未午巳"));
	G("敬安",M("子午未丑申寅酉卯戌辰亥巳"));
	G("玉宇",M("申寅卯酉辰戌巳亥午子未丑"));
	G("金堂",M("酉卯辰戌巳亥午子未丑申寅"));
	G("吉期",M("丑寅卯辰巳午未申酉戌亥子"));
	G("小时",M("子丑寅卯辰巳午未申酉戌亥"));
	G("兵福",M("子丑寅卯辰巳午未申酉戌亥"));
	G("兵宝",M("丑寅卯辰巳午未申酉戌亥子"));
	static const std::array<const char*,12> kBingJi={{
		"寅卯辰巳","丑寅卯辰","子丑寅卯","亥子丑寅","戌亥子丑","酉戌亥子",
		"申酉戌亥","未申酉戌","午未申酉","巳午未申","辰巳午未","卯辰巳午"
	}};
	G("兵吉",branch_in(c.day_branch,kBingJi[static_cast<std::size_t>(c.god_month_branch)]));

	B("岁破",c.day_branch==(c.year_branch+6)%12);
	B("天罡",M("卯戌巳子未寅酉辰亥午丑申"));
	B("河魁",M("酉辰亥午丑申卯戌巳子未寅"));
	B("死神",M("卯辰巳午未申酉戌亥子丑寅"));
	B("死气",M("辰巳午未申酉戌亥子丑寅卯"));
	static const std::array<int,4> kFuBing={{2,0,8,6}};
	static const std::array<int,4> kDaHuo={{3,1,9,7}};
	B("伏兵",c.day_stem==kFuBing[static_cast<std::size_t>(c.year_branch%4)]);
	B("官符",M("辰巳午未申酉戌亥子丑寅卯"));
	B("月建",M("子丑寅卯辰巳午未申酉戌亥"));
	B("月破",M("午未申酉戌亥子丑寅卯辰巳"));
	B("月煞",M("未辰丑戌未辰丑戌未辰丑戌"));
	B("月害",M("未午巳辰卯寅丑子亥戌酉申"));
	B("月刑",M("卯戌巳子辰申午丑寅酉未亥"));
	B("月厌",M("子亥戌酉申未午巳辰卯寅丑"));
	B("月忌",c.lunar_day==5||c.lunar_day==14||c.lunar_day==23);
	B("月虚",M("未辰丑戌未辰丑戌未辰丑戌"));
	B("灾煞",M("午卯子酉午卯子酉午卯子酉"));
	B("劫煞",M("巳寅亥申巳寅亥申巳寅亥申"));
	B("厌对",M("午巳辰卯寅丑子亥戌酉申未"));
	B("招摇",M("午巳辰卯寅丑子亥戌酉申未"));
	B("小红砂",M("酉丑巳酉丑巳酉丑巳酉丑巳"));
	B("往亡",M("戌丑寅巳申亥卯午酉子辰未"));
	B("重丧",M("癸己甲乙己丙丁己庚辛己壬"));
	B("重复",M("癸己庚辛己壬癸戊甲乙己壬"));
	B("杨公忌",is_yang_gong(c.lunar_month,c.lunar_day));
	B("神号",M("申酉戌亥子丑寅卯辰巳午未"));
	B("妨择",M("辰辰午午申申戌戌子子寅寅"));
	B("披麻",M("午卯子酉午卯子酉午卯子酉"));
	B("大耗",M("辰巳午未申酉戌亥子丑寅卯"));
	B("大祸",c.day_stem==kDaHuo[static_cast<std::size_t>(c.year_branch%4)]);
	B("天吏",M("卯子酉午卯子酉午卯子酉午"));
	B("天瘟",M("丑卯未戌辰寅午子酉申巳亥"));
	B("天狱",M("午酉子卯午酉子卯午酉子卯"));
	B("天火",M("午酉子卯午酉子卯午酉子卯"));
	B("天棒",M("寅辰午申戌子寅辰午申戌子"));
	B("天狗",M("寅卯辰巳午未申酉戌亥子丑"));
	B("天狗下食",M("戌亥子丑寅卯辰巳午未申酉"));
	B("天贼",M("卯寅丑子亥戌酉申未午巳辰"));
	static const std::array<const char*,12> kDiNang={{
		"辛未辛酉","乙酉乙未","庚子庚午","癸未癸丑","甲子甲寅","己卯己丑",
		"戊辰戊午","癸未癸巳","丙寅丙申","丁卯丁巳","戊辰戊子","庚戌庚子"
	}};
	B("地囊",gz_in(kDiNang[static_cast<std::size_t>(c.god_month_branch)],c.day_gz));
	B("地火",M("子亥戌酉申未午巳辰卯寅丑"));
	B("独火",M("未午巳辰卯寅丑子亥戌酉申"));
	B("受死",M("卯酉戌辰亥巳子午丑未寅申"));
	B("黄沙",M("寅子午寅子午寅子午寅子午"));
	B("六不成",M("卯未寅午戌巳酉丑申子辰亥"));
	B("小耗",M("卯辰巳午未申酉戌亥子丑寅"));
	B("神隔",M("酉未巳卯丑亥酉未巳卯丑亥"));
	B("朱雀",M("亥丑卯巳未酉亥丑卯巳未酉"));
	B("白虎",M("寅辰午申戌子寅辰午申戌子"));
	B("玄武",M("巳未酉亥丑卯巳未酉亥丑卯"));
	B("勾陈",M("未酉亥丑卯巳未酉亥丑卯巳"));
	B("木马",M("辰午巳未酉申戌子亥丑卯寅"));
	B("破败",M("辰午申戌子寅辰午申戌子寅"));
	B("殃败",M("巳辰卯寅丑子亥戌酉申未午"));
	B("雷公",M("巳申寅亥巳申寅亥巳申寅亥"));
	B("飞廉",M("申酉戌巳午未寅卯辰亥子丑"));
	B("大煞",M("申酉戌巳午未寅卯辰亥子丑"));
	B("枯鱼",M("申巳辰丑戌未卯子酉午寅亥"));
	B("九空",M("申巳辰丑戌未卯子酉午寅亥"));
	B("八座",M("酉戌亥子丑寅卯辰巳午未申"));
	static const std::array<const char*,4> kEightWind={{"丁丑己酉","甲申甲辰","辛未丁未","甲戌甲寅"}};
	B("八风触水龙",gz_in(kEightWind[static_cast<std::size_t>(c.season)],c.day_gz));
	B("血忌",M("午子丑未寅申卯酉辰戌巳亥"));
	B("阴错",c.day_gz==gz_at("壬子癸丑庚寅辛卯庚辰丁巳丙午丁未甲申乙酉甲戌癸亥",
		static_cast<std::size_t>(c.god_month_branch)));
	B("三娘煞",c.lunar_day==3||c.lunar_day==7||c.lunar_day==13||
		c.lunar_day==18||c.lunar_day==22||c.lunar_day==27);
	B("四绝",c.four_extinctions);
	B("四离",c.four_separations);
	B("四击",M("未未戌戌戌丑丑丑辰辰辰未"));
	static const std::array<const char*,4> kFourLoss={{"壬子","乙卯","戊午","辛酉"}};
	static const std::array<const char*,4> kFourPoor={{"乙亥","丁亥","辛亥","癸亥"}};
	static const std::array<const char*,4> kFourTaboo={{"甲子","丙子","庚子","壬子"}};
	static const std::array<const char*,4> kFourWaste={{"庚申辛酉","壬子癸亥","甲寅乙卯","丁巳丙午"}};
	B("四耗",c.day_gz==kFourLoss[static_cast<std::size_t>(c.season)]);
	B("四穷",c.day_gz==kFourPoor[static_cast<std::size_t>(c.season)]);
	B("四忌",c.day_gz==kFourTaboo[static_cast<std::size_t>(c.season)]);
	B("四废",gz_in(kFourWaste[static_cast<std::size_t>(c.season)],c.day_gz));
	static const std::array<const char*,12> kFiveTomb={{
		"壬辰","戊辰","乙未","乙未","戊辰","丙戌",
		"丙戌","戊辰","辛丑","辛丑","戊辰","壬辰"
	}};
	B("五墓",c.day_gz==kFiveTomb[static_cast<std::size_t>(c.god_month_branch)]);
	static const std::array<const char*,4> kFiveVoid={{"巳酉丑","申子辰","亥卯未","寅午戌"}};
	B("五虚",branch_in(c.day_branch,kFiveVoid[static_cast<std::size_t>(c.season)]));
	B("五离",branch_in(c.day_branch,"申酉"));
	B("五鬼",M("未戌午寅辰酉卯申丑巳子亥"));
	B("八专",c.day_gz=="丁未"||c.day_gz=="己未"||c.day_gz=="庚申"||
		c.day_gz=="甲寅"||c.day_gz=="癸丑");
	B("九坎",M("申巳辰丑戌未卯子酉午寅亥"));
	B("九焦",M("申巳辰丑戌未卯子酉午寅亥"));
	static const std::array<const char*,4> kSkyTurn={{"乙卯","丙午","辛酉","壬子"}};
	static const std::array<const char*,4> kEarthTurn={{"辛卯","戊午","癸酉","丙子"}};
	static const std::array<int,4> kMonthTurn={{3,6,9,0}};
	B("天转",c.day_gz==kSkyTurn[static_cast<std::size_t>(c.season)]);
	B("地转",c.day_gz==kEarthTurn[static_cast<std::size_t>(c.season)]);
	B("月建转杀",c.day_branch==kMonthTurn[static_cast<std::size_t>(c.season)]);
	static const std::array<const char*,4> kBarren={{"巳酉丑","申子辰","亥卯未","寅午戌"}};
	B("荒芜",branch_in(c.day_branch,kBarren[static_cast<std::size_t>(c.season)]));
	B("蚩尤",M("戌子寅辰午申戌子寅辰午申"));
	B("大时",M("酉午卯子酉午卯子酉午卯子"));
	B("大败",M("酉午卯子酉午卯子酉午卯子"));
	B("咸池",M("酉午卯子酉午卯子酉午卯子"));
	B("土符",M("申子丑巳酉寅午戌卯未亥辰"));
	B("土府",M("子丑寅卯辰巳午未申酉戌亥"));
	B("土王用事",c.earth_king);
	B("血支",M("亥子丑寅卯辰巳午未申酉戌"));
	B("游祸",M("亥申巳寅亥申巳寅亥申巳寅"));
	B("归忌",M("寅子丑寅子丑寅子丑寅子丑"));
	B("岁薄",(c.lunar_month==4&&(c.day_gz=="戊午"||c.day_gz=="丙午"))||
		(c.lunar_month==10&&(c.day_gz=="壬子"||c.day_gz=="戊子")));
	B("逐阵",(c.lunar_month==6&&(c.day_gz=="戊午"||c.day_gz=="丙午"))||
		(c.lunar_month==12&&(c.day_gz=="壬子"||c.day_gz=="戊子")));
	B("阴阳交破",c.lunar_month==10&&c.day_gz=="丁巳");
	static const std::array<const char*,12> kTreasure={{
		"丁未","丁丑","丙戌","甲午","庚子","壬寅",
		"癸卯","乙巳","戊申","己酉","辛亥","丙辰"
	}};
	static const std::array<const char*,12> kRighteous={{
		"甲子","丙寅","丁卯","己巳","辛未","壬申",
		"癸酉","乙亥","庚辰","辛丑","庚戌","戊午"
	}};
	static const std::array<const char*,12> kControl={{
		"乙丑","甲戌","壬午","戊子","庚寅","辛卯",
		"癸巳","乙未","丙申","丁酉","己亥","甲辰"
	}};
	static const std::array<const char*,12> kAttack={{
		"庚午","辛巳","丙子","戊寅","己卯","癸未",
		"癸丑","甲申","乙酉","丁亥","壬辰","壬戌"
	}};
	static const std::array<const char*,12> kSpecial={{
		"甲寅","乙卯","丁巳","丙午","庚申","辛酉",
		"癸亥","壬子","戊辰","戊戌","己丑","己未"
	}};
	auto gz_in_array=[&](const auto&items){
		return std::find_if(items.begin(),items.end(),[&](const char*value){
			return c.day_gz==value;
		})!=items.end();
	};
	B("宝日",gz_in_array(kTreasure));
	B("义日",gz_in_array(kRighteous));
	B("制日",gz_in_array(kControl));
	B("伐日",gz_in_array(kAttack));
	B("专日",gz_in_array(kSpecial));
	B("重日",c.day_branch==5||c.day_branch==11);
	B("复日",M("癸巳甲乙戊丙丁巳庚辛戊壬"));
}

bool day_rule_has(const GodSeq&good,const GodSeq&bad,int officer,
					  std::string_view name){
	if(name.size()>=3&&name.substr(name.size()-3)=="日"){
		std::string officer_name=kJianchuText[static_cast<std::size_t>(officer)];
		officer_name+="日";
		if(officer_name==name){
			return true;
		}
	}
	for(std::size_t i=0;i<kGodText.size();++i){
		if(std::string_view(kGodText[i])==name){
			int code=static_cast<int>(i);
			return god_has(good,code)||god_has(bad,code);
		}
	}
	return false;
}

struct ActivityLevelRule{
	const char*trigger;
	const char*branches;
	const char*companions;
	int level;
};

struct ActivityResolution{
	int thing=1;
	int today=-1;
	bool is_de=false;
};

ActivityResolution resolve_activity_level(const RuleDayContext&c,
								 const GodSeq&good,const GodSeq&bad,
								 int officer){
	static const ActivityLevelRule rows[]={
		{"平日","亥","相日|时德|六合",0},{"平日","巳","相日|六合|月刑",1},
		{"平日","申","相日|月害",2},{"平日","寅","相日|月害|月刑",3},
		{"平日","卯午酉","天吏",3},{"平日","辰戌丑未","月煞",4},
		{"平日","子","天吏|月刑",4},
		{"收日","寅申","长生|六合|劫煞",0},{"收日","巳亥","长生|劫煞",2},
		{"收日","辰未","月害",2},{"收日","子午酉","大时",3},
		{"收日","丑戌","月刑",3},{"收日","卯","大时",4},
		{"闭日","子午卯酉","王日",3},{"闭日","辰戌丑未","官日|天吏",3},
		{"闭日","寅申巳亥","月煞",4},
		{"劫煞","寅申","长生|六合",0},{"劫煞","辰戌丑未","除日|相日",1},
		{"劫煞","巳亥","长生|月害",2},{"劫煞","子午卯酉","执日",3},
		{"灾煞","寅申巳亥","开日",1},{"灾煞","辰戌丑未","满日|民日",2},
		{"灾煞","子午","月破",4},{"灾煞","卯酉","月破|月厌",5},
		{"月煞","卯酉","六合|危日",1},{"月煞","子午","月害|危日",3},
		{"月刑","巳","平日|六合|相日",1},{"月刑","寅","相日|月害|平日",3},
		{"月刑","辰酉亥","建日",3},{"月刑","子","平日|天吏",4},
		{"月刑","卯","收日|大时|天破",4},{"月刑","未申","月破",4},
		{"月刑","午","月建|月厌|德大会",4},
		{"月害","卯酉","守日|除日",2},{"月害","丑未","执日|大时",2},
		{"月害","巳亥","长生|劫煞",2},{"月害","申","相日|平日",2},
		{"月害","子午","月煞",3},{"月害","辰戌","官日|闭日|天吏",3},
		{"月害","寅","相日|平日|月刑",3},
		{"月厌","寅申","成日",2},{"月厌","丑未","开日",2},
		{"月厌","辰戌","定日",3},{"月厌","巳亥","满日",3},
		{"月厌","子","月建|德大会",4},{"月厌","午","月建|月刑|德大会",4},
		{"月厌","卯酉","月破|灾煞",5},
		{"大时","寅申巳亥","除日|官日",0},{"大时","辰戌","执日|六合",0},
		{"大时","丑未","执日|月害",2},{"大时","子午酉","收日",3},
		{"大时","卯","收日|月刑",4},
		{"天吏","寅申巳亥","危日",2},{"天吏","辰戌丑未","闭日",3},
		{"天吏","卯午酉","平日",3},{"天吏","子","平日|月刑",4}
	};
	int today_level=-1;
	for(const auto&row : rows){
		if(!day_rule_has(good,bad,officer,row.trigger)||
		   !branch_in(c.solar_month_branch,row.branches)){
			continue;
		}
		std::string_view companions=row.companions;
		std::size_t begin=0;
		while(begin<companions.size()){
			std::size_t end=companions.find('|',begin);
			if(end==std::string_view::npos){
				end=companions.size();
			}
			if(day_rule_has(good,bad,officer,
				companions.substr(begin,end-begin))){
				today_level=std::max(today_level,row.level);
				break;
			}
			begin=end+1;
		}
	}
	bool is_de=god_has(good,"岁德")||god_has(good,"岁德合")||
			   god_has(good,"月德")||god_has(good,"月德合")||
			   god_has(good,"天德")||god_has(good,"天德合");
	int thing_level=1;
	switch(today_level){
		case 5: thing_level=3; break;
		case 4: thing_level=is_de?2:3; break;
		case 3: thing_level=is_de?1:2; break;
		case 2: thing_level=is_de?0:2; break;
		case 1: thing_level=is_de?0:1; break;
		case 0: thing_level=0; break;
		default: thing_level=1; break;
	}
	return {thing_level,today_level,is_de};
}

void remove_activities(ActSeq&seq,std::initializer_list<const char*> values){
	for(const char*value : values){
		act_remove(seq,value);
	}
}

void add_activities(ActSeq&seq,std::initializer_list<const char*> values){
	for(const char*value : values){
		act_add(seq,value);
	}
}

void remove_activity_conflicts(ActSeq&from,const ActSeq&other){
	std::vector<int> common;
	for(int code : from.order){
		if(act_has(other,code)){
			common.push_back(code);
		}
	}
	for(int code : common){
		act_remove(from,code);
	}
}

void apply_calendar_activity_rules(const RuleDayContext&c,int officer,
								bool lunar_month_long,
								ActSeq&yi,ActSeq&ji){
	if(c.next_solar_num>=4&&c.next_solar_num<9&&
	   (officer==5||officer==7||officer==9)){
		act_add(yi,"取鱼");
	}
	if((c.next_solar_num>=20||c.next_solar_num<3)&&
	   (officer==5||officer==7||officer==9)){
		act_add(yi,"畋猎");
	}
	if((c.next_solar_num>=21||c.next_solar_num<3)&&officer==7){
		act_add(yi,"伐木");
	}
	if(c.lunar_day==1||c.lunar_day==6||c.lunar_day==15||
	   c.lunar_day==19||c.lunar_day==21||c.lunar_day==23){
		act_add(ji,"整手足甲");
	}
	if(c.lunar_day==12||c.lunar_day==15){
		add_activities(ji,{"整容","剃头"});
	}
	bool phase_day=c.lunar_day==1||c.lunar_day==7||c.lunar_day==8||
		c.lunar_day==22||c.lunar_day==23||
		(lunar_month_long?c.lunar_day==16:c.lunar_day==15);
	if(c.lunar_day==15||phase_day){
		act_add(ji,"求医疗病");
	}
}

ActSeq collect_virtue_exceptions(const GodSeq&good){
	ActSeq result;
	for(const char*name : {"岁德","岁德合","月德","月德合","天德","天德合"}){
		if(!god_has(good,name)){
			continue;
		}
		for(const auto&row : kGodActivityRules){
			if(std::string_view(row.god)==name){
				add_pipe_acts(result,row.ji);
				break;
			}
		}
	}
	return result;
}

void merge_activities(ActSeq&target,const ActSeq&source){
	for(int code : source.order){
		act_add(target,code);
	}
}

void sort_traditional_activities(std::vector<int>&codes){
	static const std::array<const char*,38> order={{
		"祭祀","出行","移徙","结婚姻","宴会","嫁娶","安床","沐浴","剃头",
		"修造","求医疗病","上表章","上官","入学","冠带","进人口","裁衣",
		"竖柱上梁","经络","开市","立券","交易","纳财","修置产室","开渠",
		"穿井","安碓硙","扫舍宇","平治道涂","破屋坏垣","伐木","捕捉",
		"畋猎","栽种","牧养","破土","安葬","启攒"
	}};
	std::array<int,kActCount> rank{};
	rank.fill(1000);
	for(std::size_t i=0;i<order.size();++i){
		int code=find_act_code(order[i]);
		if(code>=0&&code<static_cast<int>(rank.size())){
			rank[static_cast<std::size_t>(code)]=static_cast<int>(i);
		}
	}
	std::stable_sort(codes.begin(),codes.end(),[&](int a,int b){
		return rank[static_cast<std::size_t>(a)]<
			   rank[static_cast<std::size_t>(b)];
	});
}

void resolve_activity_rules(const RuleDayContext&c,const GodSeq&good,
						  const GodSeq&bad,int officer,int thing_level,
						  int today_level,bool is_de,ActSeq&yi,ActSeq&ji){
	if(thing_level==3){
		yi=ActSeq{};
		ji=ActSeq{};
		act_add(yi,"诸事不宜");
		act_add(ji,"诸事不宜");
	}else if(thing_level==2){
		remove_activity_conflicts(yi,ji);
	}else if(thing_level==1){
		ActSeq old_yi=yi;
		remove_activity_conflicts(yi,ji);
		remove_activity_conflicts(ji,old_yi);
	}else{
		remove_activity_conflicts(ji,yi);
	}

	ActSeq de_bad=collect_virtue_exceptions(good);
	if(thing_level!=3){
		if(act_has(yi,"宣政事")&&act_has(yi,"布政事")){
			act_remove(yi,"布政事");
		}
		if(act_has(yi,"营建宫室")&&act_has(yi,"修宫室")){
			act_remove(yi,"修宫室");
		}
		bool high_power=god_has(good,"岁德合")||god_has(good,"月德合")||
			god_has(good,"天德合")||god_has(good,"天赦")||
			god_has(good,"天愿")||god_has(good,"月恩")||
			god_has(good,"四相")||god_has(good,"时德");
		if(high_power&&thing_level!=2){
			remove_activities(ji,{"进人口","安床","经络","酝酿","开市",
				"立券交易","纳财","开仓库","出货财"});
			merge_activities(ji,de_bad);
		}
		if(god_has(bad,"天狗")||c.day_branch==2){
			act_add(ji,"祭祀");
			remove_activities(yi,{"祭祀","求福","祈嗣"});
		}
		if(c.day_branch==3){
			act_add(ji,"穿井");
			remove_activities(yi,{"穿井","开渠"});
		}
		if(c.day_stem==8){
			act_add(ji,"开渠");
			remove_activities(yi,{"开渠","穿井"});
		}
		if(c.day_branch==5){
			act_add(ji,"出行");
			remove_activities(yi,{"出行","出师","遣使"});
		}
		if(c.day_branch==9){
			act_add(ji,"宴会");
			remove_activities(yi,{"宴会","庆赐","赏贺"});
		}
		if(c.day_stem==3){
			act_add(ji,"剃头");
			remove_activities(yi,{"剃头","整容"});
		}
		if(today_level==0&&thing_level==0){
			merge_activities(ji,de_bad);
		}
		if(today_level==1){
			merge_activities(ji,de_bad);
			if(!act_has(ji,"祈福")){
				act_remove(ji,"求嗣");
			}
			if(!act_has(ji,"结婚姻")&&!is_de){
				remove_activities(ji,{"冠带","纳采问名","嫁娶","进人口"});
			}
			if(!act_has(ji,"嫁娶")&&!is_de&&!god_has(good,"不将")){
				remove_activities(ji,{"冠带","纳采问名","结婚姻",
					"进人口","搬移","安床"});
			}
		}
		if(c.day_branch==11){
			act_add(ji,"嫁娶");
		}
		if(today_level==1&&!is_de){
			if(!act_has(ji,"搬移")){
				act_remove(ji,"安床");
			}
			if(!act_has(ji,"安床")){
				act_remove(ji,"搬移");
			}
			if(!act_has(ji,"解除")){
				remove_activities(ji,{"整容","剃头","整手足甲"});
			}
			if(!act_has(ji,"修造")||!act_has(ji,"竖柱上梁")){
				remove_activities(ji,{"修宫室","缮城郭","整手足甲","筑提",
					"修仓库","鼓铸","苫盖","修置产室","开渠穿井",
					"安碓硙","补垣塞穴","修饰垣墙","平治道涂","破屋坏垣"});
			}
		}
		if(today_level==1){
			if(!act_has(ji,"开市")){
				remove_activities(ji,{"立券交易","纳财","开仓库","出货财"});
			}
			if(!act_has(ji,"纳财")){
				remove_activities(ji,{"立券交易","开市"});
			}
			if(!act_has(ji,"立券交易")){
				remove_activities(ji,{"纳财","开市","开仓库","出货财"});
			}
			if(!act_has(ji,"牧养")){
				act_remove(ji,"纳畜");
			}
			if(!act_has(ji,"纳畜")){
				act_remove(ji,"牧养");
			}
			if(act_has(yi,"安葬")){
				act_remove(ji,"启攒");
			}
			if(act_has(yi,"启攒")){
				act_remove(ji,"安葬");
			}
		}
		if(act_has(ji,"诏命公卿")||act_has(ji,"招贤")){
			remove_activities(yi,{"施恩","举正直"});
		}
		if(act_has(ji,"施恩")||act_has(ji,"举正直")){
			remove_activities(yi,{"诏命公卿","招贤"});
		}
		if(act_has(yi,"宣政事")&&god_has(bad,"往亡")){
			act_remove(yi,"宣政事");
			act_add(yi,"布政事");
		}
		if(god_has(bad,"月厌")){
			remove_activities(yi,{"颁诏","施恩","招贤","举正直","宣政事"});
			act_add(yi,"布政事");
			act_add(ji,"补垣");
			if(god_has(bad,"土府")||god_has(bad,"土符")||god_has(bad,"地囊")){
				act_remove(yi,"塞穴");
			}
		}
		if(officer==10){
			remove_activities(yi,{"破土","安葬","启攒"});
		}
		if(god_has(bad,"四忌")||god_has(bad,"四穷")){
			act_add(ji,"安葬");
			remove_activities(yi,{"破土","启攒"});
		}
		if(god_has(good,"鸣吠")||god_has(good,"鸣吠对")){
			remove_activities(yi,{"破土","启攒"});
		}
		static const std::array<const char*,12> kNoTabooDay={{
			"空","甲戌","空","丙申","空","甲子",
			"戊申","庚辰","辛卯","甲子","空","甲子"
		}};
		if(gz_in(kNoTabooDay[static_cast<std::size_t>(c.lunar_month-1)],c.day_gz)||
		   ((god_has(good,"岁德合")||god_has(good,"月德合")||
			 god_has(good,"天德合"))&&
			(god_has(good,"天赦")||god_has(good,"天愿")))){
			ji=ActSeq{};
			act_add(ji,"诸事不忌");
		}
	}
	std::vector<int> overlap;
	for(int code : ji.order){
		if(act_has(yi,code)){
			overlap.push_back(code);
		}
	}
	bool only_all=overlap.size()==1&&
		std::string_view(act_text(static_cast<HliActCode>(overlap.front()))).find("诸事")!=
		std::string_view::npos;
	if(!only_all){
		for(int code : overlap){
			act_remove(yi,code);
		}
	}
	if(ji.order.empty()){
		act_add(ji,"诸事不忌");
	}
	if(yi.order.empty()){
		act_add(yi,"诸事不宜");
	}
	sort_traditional_activities(yi.order);
	sort_traditional_activities(ji.order);
}

void evaluate_traditional_rules(const RuleDayContext&ctx,int officer,
								bool lunar_month_long,GodSeq&good,GodSeq&bad,
								ActSeq&yi,ActSeq&ji,HliData&out){
	collect_traditional_gods(ctx,good,bad);
	apply_officer_activity_rules(officer,yi,ji);
	add_stem_branch_acts(ctx.day_stem,ctx.day_branch,yi,ji);
	apply_calendar_activity_rules(ctx,officer,lunar_month_long,yi,ji);
	apply_god_activity_rules(good,yi,ji);
	apply_god_activity_rules(bad,yi,ji);

	ActivityResolution resolution=
		resolve_activity_level(ctx,good,bad,officer);
	out.yi_ji_level=resolution.thing;
	out.yi_ji_rule_code=resolution.thing;
	resolve_activity_rules(
		ctx,good,bad,officer,resolution.thing,resolution.today,
		resolution.is_de,yi,ji);
}
