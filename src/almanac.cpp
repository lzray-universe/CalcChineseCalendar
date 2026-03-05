#include "lunar/almanac.hpp"

#include<algorithm>
#include<array>
#include<cmath>
#include<map>
#include<set>
#include<sstream>
#include<stdexcept>
#include<unordered_map>

#include "lunar/math.hpp"
#include "lunar/time_scale.hpp"

namespace{

struct ActPair{
	std::vector<std::string> yi;
	std::vector<std::string> ji;
};

constexpr std::array<const char*,10> kStem={
	"甲","乙","丙","丁","戊","己","庚","辛","壬","癸"
};

constexpr std::array<const char*,12> kBranch={
	"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"
};

constexpr std::array<const char*,12> kZodiac={
	"鼠","牛","虎","兔","龙","蛇","马","羊","猴","鸡","狗","猪"
};

constexpr std::array<const char*,10> kStemWx={
	"木","木","火","火","土","土","金","金","水","水"
};

constexpr std::array<const char*,12> kBranchWx={
	"水","土","木","木","土","火","火","土","金","金","土","水"
};

constexpr std::array<const char*,30> kNayin={
	"海中金","炉中火","大林木","路旁土","剑锋金","山头火","涧下水","城头土","白蜡金","杨柳木",
	"井泉水","屋上土","霹雳火","松柏木","长流水","砂中金","山下火","平地木","壁上土","金箔金",
	"覆灯火","天河水","大驿土","钗钏金","桑柘木","大溪水","砂中土","天上火","石榴木","大海水"
};

constexpr std::array<const char*,12> kJianchu={
	"建","除","满","平","定","执","破","危","成","收","开","闭"
};

constexpr std::array<const char*,12> kDutyGod={
	"青龙","明堂","天刑","朱雀","金匮","天德","白虎","玉堂","天牢","玄武","司命","勾陈"
};

constexpr std::array<const char*,10> kPengStem={
	"甲不开仓 财物耗散","乙不栽植 千株不长","丙不修灶 必见灾殃","丁不剃头 头必生疮","戊不受田 田主不祥",
	"己不破券 二比并亡","庚不经络 织机虚张","辛不合酱 主人不尝","壬不泱水 更难提防","癸不词讼 理弱敌强"
};

constexpr std::array<const char*,12> kPengBranch={
	"子不问卜 自惹祸殃","丑不冠带 主不还乡","寅不祭祀 神鬼不尝","卯不穿井 水泉不香","辰不哭泣 必主重丧","巳不远行 财物伏藏",
	"午不苫盖 屋主更张","未不服药 毒气入肠","申不安床 鬼祟入房","酉不会客 醉坐颠狂","戌不吃犬 作怪上床","亥不嫁娶 不利新郎"
};

constexpr std::array<const char*,28> kXiu28={
	"角木蛟","亢金龙","氐土貉","房日兔","心月狐","尾火虎","箕水豹","斗木獬","牛金牛","女土蝠","虚日鼠","危月燕","室火猪","壁水貐",
	"奎木狼","娄金狗","胃土雉","昴日鸡","毕月乌","觜火猴","参水猿","井木犴","鬼金羊","柳土獐","星日马","张月鹿","翼火蛇","轸水蚓"
};

constexpr std::array<const char*,8> kDir8={
	"正北","东北","正东","东南","正南","西南","正西","西北"
};

constexpr std::array<const char*,8> kTri8={
	"坎","艮","震","巽","离","坤","兑","乾"
};

constexpr std::array<const char*,10> kLuckTri={
	"艮","乾","坤","离","巽","艮","乾","坤","离","巽"
};

constexpr std::array<const char*,10> kWealthTri={
	"艮","艮","坤","坤","坎","坎","震","震","离","离"
};

constexpr std::array<const char*,10> kMascotTri={
	"坎","坤","乾","巽","艮","坎","坤","乾","巽","艮"
};

constexpr std::array<const char*,10> kSunNobleTri={
	"坤","坤","兑","乾","艮","坎","离","艮","震","巽"
};

constexpr std::array<const char*,10> kMoonNobleTri={
	"艮","坎","乾","兑","坤","坤","艮","离","巽","震"
};

constexpr std::array<const char*,12> kMeridian={
	"胆","肝","肺","大肠","胃","脾","心","小肠","膀胱","肾","心包","三焦"
};

constexpr std::array<const char*,60> kFetalGod={
	"碓磨门外东南","碓磨厕外东南","厨灶炉外正南","仓库门外正南","房床厕外正南","占门床外正南","占碓磨外正南","厨灶厕外西南","仓库炉外西南","房床门外西南",
	"门碓栖外西南","碓磨床外西南","厨灶碓外西南","仓库厕外西南","房床厕外正南","房床炉外正西","碓磨栖外正西","厨灶床外正西","仓库碓外西北","房床厕外西北",
	"占门炉外西北","碓磨门外西北","厨灶栖外西北","仓库床外西北","房床碓外正北","占门厕外正北","碓磨炉外正北","厨灶门外正北","仓库栖外正北","占房床房内北",
	"占门碓房内北","碓磨门房内北","厨灶炉房内北","仓库门房内北","房床栖房内中","占门床房内中","占碓磨房内南","厨灶厕房内南","仓库炉房内南","房床门房内南",
	"门鸡栖房内东","碓磨床房内东","厨灶碓房内东","仓库厕房内东","房床炉房内东","占大门外东北","碓磨栖外东北","厨灶床外东北","仓库碓外东北","房床厕外东北",
	"占门炉外东北","碓磨门外正东","厨灶栖外正东","仓库床外正东","房床碓外正东","占门厕外正东","碓磨炉外东南","仓库栖外东南","占房床外东南","占门碓外东南"
};

constexpr std::array<const char*,38> kActOrder={
	"祭祀","出行","移徙","结婚姻","宴会","嫁娶","安床","沐浴","剃头","修造","求医疗病","上表章","上官","入学","冠带","进人口",
	"裁衣","竖柱上梁","经络","开市","立券交易","纳财","修置产室","开渠","穿井","安碓硙","扫舍宇","平治道涂","破屋坏垣","伐木","捕捉",
	"畋猎","栽种","牧养","破土","安葬","启攒","诸事不宜"
};

constexpr std::array<int,60> kHourMask={
	0x2d3,0xcb4,0x32d,0x4cb,0xd32,0xb4c,0x2d3,0xcb4,0x32d,0x4cb,0xd22,0xb5c,
	0x2d3,0xcb4,0x32d,0x4cb,0xd3a,0xb4d,0x2d3,0xcb4,0x32d,0x4cb,0xd32,0xb4c,
	0x2d3,0xcb5,0x32d,0x4cb,0xd32,0xb4c,0x2d3,0xcb4,0x32d,0x4cb,0xd32,0xb4c,
	0x2d3,0xcb4,0x32d,0x4db,0xd32,0xb5c,0x2d7,0xcb4,0x32d,0x4cb,0xd32,0xb5c,
	0x2d3,0xcb4,0x32d,0x4cb,0xd32,0xb4c,0x2d3,0xcb4,0x30d,0x4cb,0xd32,0xb4c
};

constexpr std::array<std::pair<int,int>,13> kYangGong={
	std::pair<int,int>{1,13},{2,11},{3,9},{4,7},{5,5},{6,2},{7,1},
	{7,29},{8,27},{9,25},{10,23},{11,21},{12,19}
};

int pos_mod(long long v,int m){
	long long r=v%m;
	if(r<0){
		r+=m;
	}
	return static_cast<int>(r);
}

double norm_rad(double x){
	double v=std::fmod(x,TWO_PI);
	if(v<0.0){
		v+=TWO_PI;
	}
	return v;
}

double norm_min(double x){
	double v=std::fmod(x,1440.0);
	if(v<0.0){
		v+=1440.0;
	}
	return v;
}

std::string gz_text(int stem,int branch){
	return std::string(kStem[static_cast<std::size_t>(stem)])+
		   kBranch[static_cast<std::size_t>(branch)];
}

GzNode mk_gz_from_idx(int idx60){
	int stem=idx60%10;
	int branch=idx60%12;
	GzNode out;
	out.stem=stem;
	out.branch=branch;
	out.text=gz_text(stem,branch);
	return out;
}

GzNode mk_gz_from_year(int year){
	int stem=pos_mod(static_cast<long long>(year)-4,10);
	int branch=pos_mod(static_cast<long long>(year)-4,12);
	GzNode out;
	out.stem=stem;
	out.branch=branch;
	out.text=gz_text(stem,branch);
	return out;
}

void uniq_keep_order(std::vector<std::string>&v){
	std::set<std::string> seen;
	std::vector<std::string> out;
	out.reserve(v.size());
	for(const auto&s : v){
		if(seen.insert(s).second){
			out.push_back(s);
		}
	}
	v.swap(out);
}

void del_items(std::vector<std::string>&v,const std::set<std::string>&rm){
	std::vector<std::string> out;
	out.reserve(v.size());
	for(const auto&s : v){
		if(rm.find(s)==rm.end()){
			out.push_back(s);
		}
	}
	v.swap(out);
}

void rm_text(std::vector<std::string>&v,const std::string&s){
	v.erase(std::remove(v.begin(),v.end(),s),v.end());
}

void sort_acts(std::vector<std::string>&v){
	static std::unordered_map<std::string,int> rank=[](){
		std::unordered_map<std::string,int> m;
		for(std::size_t i=0;i<kActOrder.size();++i){
			m.emplace(kActOrder[i],static_cast<int>(i));
		}
		return m;
	}();
	std::sort(v.begin(),v.end(),[&](const std::string&a,const std::string&b){
		int ra=9999;
		int rb=9999;
		auto ia=rank.find(a);
		if(ia!=rank.end()){
			ra=ia->second;
		}
		auto ib=rank.find(b);
		if(ib!=rank.end()){
			rb=ib->second;
		}
		if(ra!=rb){
			return ra<rb;
		}
		return a<b;
	});
}

bool has_text(const std::vector<std::string>&v,const std::string&s){
	return std::find(v.begin(),v.end(),s)!=v.end();
}

bool in_set_branch(int b,std::initializer_list<int> a){
	for(int x : a){
		if(b==x){
			return true;
		}
	}
	return false;
}

std::string tri_to_dir(const char*tri){
	for(std::size_t i=0;i<kTri8.size();++i){
		if(std::string(kTri8[i])==tri){
			return kDir8[i];
		}
	}
	return "正北";
}

std::string sha_dir_by_branch(int b){
	if(in_set_branch(b,{8,0,4})){
		return "南";
	}
	if(in_set_branch(b,{2,6,10})){
		return "北";
	}
	if(in_set_branch(b,{11,3,7})){
		return "西";
	}
	return "东";
}

ActPair base_act(const std::string&jc){
	static const std::map<std::string,ActPair> db={
		{"建",{{"施恩","招贤","举正直","出行","上官","临政"},{}}},
		{"除",{{"解除","沐浴","整容","剃头","整手足甲","求医疗病","扫舍宇"},{}}},
		{"满",{{"进人口","裁制","竖柱上梁","经络","开市","立券交易","纳财","开仓","塞穴","补垣"},
			  {"施恩","招贤","举正直","上官","临政","结婚姻","纳采","求医疗病"}}},
		{"平",{{"修饰垣墙","平治道涂"},
			  {"祈福","求嗣","宴会","冠带","出行","上官","临政","结婚姻","纳采","嫁娶","搬移","安床","求医疗病","营建","修造","开市","立券交易","纳财","开仓","破土","安葬","启攒"}}},
		{"定",{{"冠带"},{}}},
		{"执",{{"捕捉"},{}}},
		{"破",{{"求医疗病"},{"嫁娶","开市","安葬"}}},
		{"危",{{"安抚边境","选将","安床"},{"登高","行船"}}},
		{"成",{{"入学","安抚边境","搬移","筑堤防","开市"},{"诉讼"}}},
		{"收",{{"进人口","纳财","捕捉","纳畜"},
			  {"结婚姻","纳采","嫁娶","开市","立券交易","开仓","安葬"}}},
		{"开",{{"祭祀","祈福","求嗣","上表章","施恩","招贤","举正直","宴会","入学","出行","上官","临政","搬移","求医疗病","修造","开市","开渠","穿井","栽种","牧养"},
			  {}}},
		{"闭",{{"筑堤防","塞穴","补垣"},
			  {"上表章","施恩","招贤","举正直","宴会","出行","上官","临政","结婚姻","纳采","嫁娶","进人口","搬移","安床","求医疗病","修造","竖柱上梁","开市","开仓","开渠","穿井"}}}
	};
	auto it=db.find(jc);
	if(it==db.end()){
		return {};
	}
	return it->second;
}

void add_stem_branch_acts(int day_stem,int day_branch,std::vector<std::string>&yi,
						  std::vector<std::string>&ji){
	static const std::array<ActPair,10> stem_fix={
		ActPair{{},{"开仓"}},ActPair{{},{"栽种"}},ActPair{},ActPair{{},{"整容","剃头"}},ActPair{},
		ActPair{},ActPair{{},{"经络"}},ActPair{{},{"酝酿"}},ActPair{{},{"开渠","穿井"}},ActPair{}
	};
	static const std::array<ActPair,12> branch_fix={
		ActPair{{"沐浴"},{}},ActPair{{},{"冠带"}},ActPair{{},{"祭祀"}},ActPair{{},{"穿井"}},ActPair{},
		ActPair{{},{"出行"}},ActPair{{},{"苫盖"}},ActPair{{},{"求医疗病"}},ActPair{{},{"安床"}},ActPair{{},{"宴会"}},
		ActPair{},ActPair{{"沐浴"},{"嫁娶"}}
	};
	const ActPair&s=stem_fix[static_cast<std::size_t>(day_stem)];
	const ActPair&b=branch_fix[static_cast<std::size_t>(day_branch)];
	yi.insert(yi.end(),s.yi.begin(),s.yi.end());
	yi.insert(yi.end(),b.yi.begin(),b.yi.end());
	ji.insert(ji.end(),s.ji.begin(),s.ji.end());
	ji.insert(ji.end(),b.ji.begin(),b.ji.end());
}

bool is_yang_gong(int lun_m,int lun_d){
	for(const auto&p : kYangGong){
		if(p.first==lun_m&&p.second==lun_d){
			return true;
		}
	}
	return false;
}

void jd_to_loc(double jd_utc,int tz_off,int&y,int&m,int&d,int&hh,int&mm,
			   double&ss){
	jd2greg(jd_utc+static_cast<double>(tz_off)/1440.0,y,m,d,hh,mm,ss);
}

bool is_same_ymd(double jd_utc,int tz_off,int y,int m,int d){
	int yy=0;
	int mm=0;
	int dd=0;
	int hh=0;
	int mi=0;
	double ss=0.0;
	jd_to_loc(jd_utc,tz_off,yy,mm,dd,hh,mi,ss);
	return yy==y&&mm==m&&dd==d;
}

bool is_next_day_term(LunCal6&lc,int gy,int gm,int gd,int tz_off,
					  const std::string&term_code){
	double day0=greg2jd(gy,gm,gd,0,0,0.0)-static_cast<double>(tz_off)/1440.0;
	double day1=day0+1.0;
	int ny=0;
	int nm=0;
	int nd=0;
	int hh=0;
	int mi=0;
	double ss=0.0;
	jd_to_loc(day1,tz_off,ny,nm,nd,hh,mi,ss);
	for(int y : {ny-1,ny,ny+1}){
		LocalDT t=lc.get_st(term_code,y);
		if(is_same_ymd(t.toUtcJD(),tz_off,ny,nm,nd)){
			return true;
		}
	}
	return false;
}

int y_for_lchun(LunCal6&lc,double jd_utc,int gy){
	double jd_lc=lc.get_st("J1",gy).toUtcJD();
	if(jd_utc<jd_lc){
		return gy-1;
	}
	return gy;
}

int month_branch(LunCal6&lc,double jd_utc,int gy){
	struct Bnd{
		double jd=0.0;
		int b=0;
	};
	static const std::array<const char*,12> code={
		"J1","J2","J3","J4","J5","J6","J7","J8","J9","J10","J11","J12"
	};
	static const std::array<int,12> br={2,3,4,5,6,7,8,9,10,11,0,1};
	std::vector<Bnd> all;
	all.reserve(36);
	for(int y : {gy-1,gy,gy+1}){
		for(std::size_t i=0;i<code.size();++i){
			LocalDT t=lc.get_st(code[i],y);
			all.push_back({t.toUtcJD(),br[i]});
		}
	}
	std::sort(all.begin(),all.end(),[](const Bnd&a,const Bnd&b){
		return a.jd<b.jd;
	});
	int out=1;
	for(const auto&x : all){
		if(x.jd<=jd_utc){
			out=x.b;
		}else{
			break;
		}
	}
	return out;
}

int day_jdn_local_noon(int gy,int gm,int gd,int tz_off){
	double jd_noon_utc=
		greg2jd(gy,gm,gd,12,0,0.0)-static_cast<double>(tz_off)/1440.0;
	return static_cast<int>(std::llround(jd_noon_utc));
}

int hour_branch_by_min(double minute_of_day){
	int hh=static_cast<int>(std::floor(minute_of_day/60.0));
	return ((hh+1)/2)%12;
}

int duty_start(int month_branch_idx){
	switch(month_branch_idx){
	case 2:
	case 8:
		return 0;
	case 3:
	case 9:
		return 2;
	case 4:
	case 10:
		return 4;
	case 5:
	case 11:
		return 6;
	case 0:
	case 6:
		return 8;
	case 1:
	case 7:
		return 10;
	default:
		return 0;
	}
}

bool bad_slot(int mask,int slot0){
	int bit=11-slot0;
	return (mask&(1<<bit))!=0;
}

std::string three_he_text(int b){
	if(b==8||b==0||b==4){
		return "申子辰";
	}
	if(b==11||b==3||b==7){
		return "亥卯未";
	}
	if(b==2||b==6||b==10){
		return "寅午戌";
	}
	return "巳酉丑";
}

std::string xiu_from_region(const std::string&region,double moon_lam){
	static const std::map<std::string,std::string> mp={
		{"角宿","角木蛟"},{"亢宿","亢金龙"},{"氐宿","氐土貉"},{"房宿","房日兔"},{"心宿","心月狐"},{"尾宿","尾火虎"},{"箕宿","箕水豹"},
		{"斗宿","斗木獬"},{"牛宿","牛金牛"},{"女宿","女土蝠"},{"虚宿","虚日鼠"},{"危宿","危月燕"},{"室宿","室火猪"},{"壁宿","壁水貐"},
		{"奎宿","奎木狼"},{"娄宿","娄金狗"},{"胃宿","胃土雉"},{"昴宿","昴日鸡"},{"毕宿","毕月乌"},{"觜宿","觜火猴"},{"参宿","参水猿"},
		{"井宿","井木犴"},{"鬼宿","鬼金羊"},{"柳宿","柳土獐"},{"星宿","星日马"},{"张宿","张月鹿"},{"翼宿","翼火蛇"},{"轸宿","轸水蚓"}
	};
	auto it=mp.find(region);
	if(it!=mp.end()){
		return it->second;
	}
	double step=TWO_PI/28.0;
	int idx=static_cast<int>(std::floor(norm_rad(moon_lam)/step));
	idx%=28;
	if(idx<0){
		idx+=28;
	}
	return kXiu28[static_cast<std::size_t>(idx)];
}

void push_many(std::vector<std::string>&out,
			   std::initializer_list<const char*> in){
	for(const char*s : in){
		out.push_back(s);
	}
}

}

HliData calc_hli(EphRead&eph,LunCal6&lc,SolLunCal&solver,AppLon&app,
				 const HliInput&in){
	(void)eph;
	(void)solver;

	HliData out;

	out.lon_deg=in.lon_deg;

	out.y_lun=mk_gz_from_year(in.lun_year);

	int y_lc=y_for_lchun(lc,in.jd_utc,in.gy);
	out.y_lchun=mk_gz_from_year(y_lc);

	int mon_b=month_branch(lc,in.jd_utc,in.gy);
	int sn=((mon_b-2+12)%12)/3;
	int mon_off=pos_mod(static_cast<long long>(mon_b)-2,12);
	int mon_st=((out.y_lchun.stem%5)*2+2+mon_off)%10;
	out.m_gz.stem=mon_st;
	out.m_gz.branch=mon_b;
	out.m_gz.text=gz_text(mon_st,mon_b);

	int jdn=day_jdn_local_noon(in.gy,in.gm,in.gd,in.tz_off);
	int day60=pos_mod(static_cast<long long>(jdn)-11,60);
	out.d_gz=mk_gz_from_idx(day60);

	double clock_min=
		static_cast<double>(in.hh)*60.0+static_cast<double>(in.mm)+in.ss/60.0;
	int hb=hour_branch_by_min(clock_min);
	int day_stem_for_h=out.d_gz.stem;
	if(clock_min>=1380.0){
		day_stem_for_h=(day_stem_for_h+1)%10;
	}
	int hs=(day_stem_for_h*2+hb)%10;
	out.h_gz.stem=hs;
	out.h_gz.branch=hb;
	out.h_gz.text=gz_text(hs,hb);

	EoTData eot=app.eot_calc(in.jd_utc,in.lon_deg);
	out.eot_min=eot.eot_minutes;
	double zone_lon=static_cast<double>(in.tz_off)/60.0*15.0;
	double tst=norm_min(clock_min+eot.eot_minutes+(in.lon_deg-zone_lon)*4.0);
	out.tst_min=tst;
	int hb_tst=hour_branch_by_min(tst);
	int day_stem_for_tst=out.d_gz.stem;
	if(tst>=1380.0){
		day_stem_for_tst=(day_stem_for_tst+1)%10;
	}
	int hs_tst=(day_stem_for_tst*2+hb_tst)%10;
	out.h_gz_true.stem=hs_tst;
	out.h_gz_true.branch=hb_tst;
	out.h_gz_true.text=gz_text(hs_tst,hb_tst);
	out.bazi_clock=out.y_lchun.text+" "+out.m_gz.text+" "+out.d_gz.text+" "+
				   out.h_gz.text;
	out.bazi_true=out.y_lchun.text+" "+out.m_gz.text+" "+out.d_gz.text+" "+
				  out.h_gz_true.text;

	int jc_idx=pos_mod(static_cast<long long>(out.d_gz.branch)-mon_b,12);
	out.jianchu=kJianchu[static_cast<std::size_t>(jc_idx)];

	int dstart=duty_start(mon_b);
	int dg_idx=pos_mod(static_cast<long long>(out.d_gz.branch)-dstart,12);
	out.duty_god=kDutyGod[static_cast<std::size_t>(dg_idx)];
	bool is_yellow=
		(dg_idx==0||dg_idx==1||dg_idx==4||dg_idx==5||dg_idx==7||dg_idx==10);
	out.duty_tag=is_yellow?"黄道日":"黑道日";

	int clash_b=(out.d_gz.branch+6)%12;
	out.clash=std::string(kZodiac[static_cast<std::size_t>(out.d_gz.branch)])+
			  "日冲"+
			  kZodiac[static_cast<std::size_t>(clash_b)];
	out.chong_sha=std::string("冲")+
				  kZodiac[static_cast<std::size_t>(clash_b)]+"煞"+
				  sha_dir_by_branch(out.d_gz.branch);
	out.zodiac_day=kZodiac[static_cast<std::size_t>(out.d_gz.branch)];

	static const std::array<int,12> kLiuHe={1,0,11,10,9,8,7,6,5,4,3,2};
	out.six_he=std::string(kBranch[static_cast<std::size_t>(out.d_gz.branch)])+
			   "合"+
			   kBranch[static_cast<std::size_t>(
				   kLiuHe[static_cast<std::size_t>(out.d_gz.branch)])];
	out.three_he=three_he_text(out.d_gz.branch);

	out.pengzu=std::string(kPengStem[static_cast<std::size_t>(out.d_gz.stem)])+
			   " "+kPengBranch[static_cast<std::size_t>(out.d_gz.branch)];

	out.nayin=kNayin[static_cast<std::size_t>(day60/2)];
	out.wx_day=std::string("天干")+kStem[static_cast<std::size_t>(out.d_gz.stem)]+"属"+
			   kStemWx[static_cast<std::size_t>(out.d_gz.stem)]+" 地支"+
			   kBranch[static_cast<std::size_t>(out.d_gz.branch)]+"属"+
			   kBranchWx[static_cast<std::size_t>(out.d_gz.branch)]+" 纳音"+
			   out.nayin;
	out.fetal_god=kFetalGod[static_cast<std::size_t>(day60)];
	out.meridian=kMeridian[static_cast<std::size_t>(hb)];
	out.lucky_dir=tri_to_dir(kLuckTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.wealth_dir=
		tri_to_dir(kWealthTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.mascot_dir=
		tri_to_dir(kMascotTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.sun_noble_dir=
		tri_to_dir(kSunNobleTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.moon_noble_dir=
		tri_to_dir(kMoonNobleTri[static_cast<std::size_t>(out.d_gz.stem)]);

	double jd_tdb=TimeScale::utc_to_tdb(in.jd_utc);
	double moon_lam=app.moon_calc(jd_tdb).first;
	out.xiu28=xiu_from_region(in.moon_xg.region,moon_lam);
	out.xiu_id=in.moon_xg.star_name;

	if(is_yellow){
		out.good_gods.push_back(out.duty_god);
	}else{
		out.bad_gods.push_back(out.duty_god);
	}

	if(out.d_gz.branch==((mon_b+6)%12)){
		out.bad_gods.push_back("月破");
	}
	if(out.d_gz.branch==((out.y_lchun.branch+6)%12)){
		out.bad_gods.push_back("岁破");
	}
	if(is_yang_gong(in.lun_month,in.lun_day)){
		out.bad_gods.push_back("杨公忌");
	}
	if(is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z2")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z5")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z8")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z11")){
		out.bad_gods.push_back("四离");
	}
	if(is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J1")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J4")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J7")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J10")){
		out.bad_gods.push_back("四绝");
	}

	if(pos_mod(static_cast<long long>(out.d_gz.branch)-mon_b,4)==0){
		out.good_gods.push_back("三合");
	}
	if(out.d_gz.branch==kLiuHe[static_cast<std::size_t>(mon_b)]){
		out.good_gods.push_back("六合");
	}

	static const std::array<int,12> kMonDeStem={8,6,2,0,8,6,2,0,8,6,2,0};
	static const std::array<int,12> kMonDeHeStem={3,1,7,5,3,1,7,5,3,1,7,5};
	if(out.d_gz.stem==kMonDeStem[static_cast<std::size_t>(mon_b)]){
		out.good_gods.push_back("月德");
	}
	if(out.d_gz.stem==kMonDeHeStem[static_cast<std::size_t>(mon_b)]){
		out.good_gods.push_back("月德合");
	}

	static const std::array<int,12> kTdeStem={-1,6,3,-1,8,7,-1,0,9,-1,2,1};
	static const std::array<std::array<int,2>,12> kTdeBranch={
		std::array<int,2>{5,4},std::array<int,2>{-1,-1},std::array<int,2>{-1,-1},std::array<int,2>{8,7},
		std::array<int,2>{-1,-1},std::array<int,2>{-1,-1},std::array<int,2>{11,10},std::array<int,2>{-1,-1},
		std::array<int,2>{-1,-1},std::array<int,2>{2,1},std::array<int,2>{-1,-1},std::array<int,2>{-1,-1}
	};
	bool is_tde=false;
	if(mon_b%3==0){
		const auto&tg=kTdeBranch[static_cast<std::size_t>(mon_b)];
		is_tde=(out.d_gz.branch==tg[0]||out.d_gz.branch==tg[1]);
	}else{
		is_tde=(out.d_gz.stem==kTdeStem[static_cast<std::size_t>(mon_b)]);
	}
	if(is_tde){
		out.good_gods.push_back("天德");
	}
	static const std::array<int,12> kTdeHeStem={-1,1,8,-1,3,2,-1,5,4,-1,7,6};
	if(kTdeHeStem[static_cast<std::size_t>(mon_b)]>=0&&
	   out.d_gz.stem==kTdeHeStem[static_cast<std::size_t>(mon_b)]){
		out.good_gods.push_back("天德合");
	}

	static const std::array<int,10> kYearDeStem={0,6,2,8,4,0,6,2,8,4};
	static const std::array<int,10> kYearDeHeStem={5,1,7,3,9,5,1,7,3,9};
	if(out.d_gz.stem==kYearDeStem[static_cast<std::size_t>(out.y_lchun.stem)]){
		out.good_gods.push_back("岁德");
	}
	if(out.d_gz.stem==
	   kYearDeHeStem[static_cast<std::size_t>(out.y_lchun.stem)]){
		out.good_gods.push_back("岁德合");
	}

	static const std::array<int,4> kWang={2,5,8,11};
	static const std::array<int,4> kGuan={3,6,9,0};
	static const std::array<int,4> kShou={9,0,3,6};
	static const std::array<int,4> kXiang={5,8,11,2};
	static const std::array<int,4> kMin={6,9,0,3};
	static const std::array<int,4> kShiDe={6,4,0,2};
	if(out.d_gz.branch==kWang[static_cast<std::size_t>(sn)]){
		out.good_gods.push_back("王日");
	}
	if(out.d_gz.branch==kGuan[static_cast<std::size_t>(sn)]){
		out.good_gods.push_back("官日");
	}
	if(out.d_gz.branch==kShou[static_cast<std::size_t>(sn)]){
		out.good_gods.push_back("守日");
	}
	if(out.d_gz.branch==kXiang[static_cast<std::size_t>(sn)]){
		out.good_gods.push_back("相日");
	}
	if(out.d_gz.branch==kMin[static_cast<std::size_t>(sn)]){
		out.good_gods.push_back("民日");
	}
	if(out.d_gz.branch==kShiDe[static_cast<std::size_t>(sn)]){
		out.good_gods.push_back("时德");
	}

	static const std::array<std::array<int,2>,4> kTianGuiStem={
		std::array<int,2>{0,1},std::array<int,2>{2,3},std::array<int,2>{6,7},
		std::array<int,2>{8,9}
	};
	const auto&tgs=kTianGuiStem[static_cast<std::size_t>(sn)];
	if(out.d_gz.stem==tgs[0]||out.d_gz.stem==tgs[1]){
		out.good_gods.push_back("天贵");
	}

	static const std::array<int,12> kTianXi={
		8,9,10,11,0,1,2,3,4,5,6,7
	};
	if(out.d_gz.branch==kTianXi[static_cast<std::size_t>(mon_b)]){
		out.good_gods.push_back("天喜");
	}

	static const std::array<int,12> kYueEnStem={
		2,3,6,5,4,7,8,9,6,1,0,7
	};
	int lun_m_fix=in.lun_month;
	if(lun_m_fix<1){
		lun_m_fix=1;
	}
	if(lun_m_fix>12){
		lun_m_fix=12;
	}
	if(out.d_gz.stem==
	   kYueEnStem[static_cast<std::size_t>(lun_m_fix-1)]){
		out.good_gods.push_back("月恩");
	}

	static const std::array<std::pair<int,int>,4> kTianSheGz={
		std::pair<int,int>{4,2},std::pair<int,int>{0,6},
		std::pair<int,int>{4,8},std::pair<int,int>{0,0}
	};
	const auto&ts_gz=kTianSheGz[static_cast<std::size_t>(sn)];
	if(out.d_gz.stem==ts_gz.first&&out.d_gz.branch==ts_gz.second){
		out.good_gods.push_back("天赦");
	}

	static const std::array<int,12> kDaHao={
		4,5,6,7,8,9,10,11,0,1,2,3
	};
	static const std::array<int,12> kWuGui={
		7,10,6,2,4,9,3,8,1,5,0,11
	};
	static const std::array<int,12> kXianChi={
		9,6,3,0,9,6,3,0,9,6,3,0
	};
	static const std::array<int,12> kXueZhi={
		11,0,1,2,3,4,5,6,7,8,9,10
	};
	static const std::array<int,12> kTianGou={
		2,3,4,5,6,7,8,9,10,11,0,1
	};
	static const std::array<int,12> kWangWang={
		10,1,2,5,8,11,3,6,9,0,4,7
	};
	static const std::array<int,12> kSiJi={
		7,7,10,10,10,1,1,1,4,4,4,7
	};
	static const std::array<int,12> kYueXing={
		3,10,5,0,4,8,6,1,2,9,7,11
	};
	static const std::array<int,12> kYueHai={
		7,6,5,4,3,2,1,0,11,10,9,8
	};
	if(out.d_gz.branch==kDaHao[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("大耗");
	}
	if(out.d_gz.branch==kWuGui[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("五鬼");
	}
	if(out.d_gz.branch==kXianChi[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("咸池");
	}
	if(out.d_gz.branch==kXueZhi[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("血支");
	}
	if(out.d_gz.branch==kTianGou[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("天狗");
	}
	if(out.d_gz.branch==kWangWang[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("往亡");
	}
	if(out.d_gz.branch==kSiJi[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("四击");
	}
	if(out.d_gz.branch==kYueXing[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("月刑");
	}
	if(out.d_gz.branch==kYueHai[static_cast<std::size_t>(mon_b)]){
		out.bad_gods.push_back("月害");
	}

	ActPair base=base_act(out.jianchu);
	out.yi=base.yi;
	out.ji=base.ji;
	add_stem_branch_acts(out.d_gz.stem,out.d_gz.branch,out.yi,out.ji);

	if(in.lun_day==1||in.lun_day==6||in.lun_day==15||in.lun_day==19||
	   in.lun_day==21||in.lun_day==23){
		out.ji.push_back("整手足甲");
	}
	if(in.lun_day==12||in.lun_day==15){
		out.ji.push_back("整容");
		out.ji.push_back("剃头");
	}
	if(in.lun_day==15||!in.phase_name.empty()){
		out.ji.push_back("求医疗病");
	}
	if(has_text(out.good_gods,"月德")||has_text(out.good_gods,"月德合")||
	   has_text(out.good_gods,"天德")||has_text(out.good_gods,"天德合")){
		push_many(out.yi,{"祭祀","祈福","求嗣","宴会","结婚姻","嫁娶","出行","上官","临政","修造","开市","纳财"});
	}
	if(has_text(out.good_gods,"岁德")||has_text(out.good_gods,"岁德合")){
		push_many(out.yi,{"修造","嫁娶","纳采","搬移"});
	}
	if(has_text(out.good_gods,"天赦")){
		push_many(out.yi,{"祭祀","祈福","求嗣","上官","临政","嫁娶","搬移","修造","开市","纳财","安葬"});
	}
	if(has_text(out.good_gods,"月恩")){
		push_many(out.yi,{"祭祀","宴会","出行","结婚姻","嫁娶"});
	}
	if(has_text(out.good_gods,"天喜")){
		push_many(out.yi,{"宴会","结婚姻","嫁娶","纳采"});
	}
	if(has_text(out.good_gods,"王日")||has_text(out.good_gods,"官日")||
	   has_text(out.good_gods,"守日")||has_text(out.good_gods,"相日")||
	   has_text(out.good_gods,"民日")||has_text(out.good_gods,"时德")){
		push_many(out.yi,{"上官","临政","施恩","宴会","纳财","开市"});
	}
	if(has_text(out.bad_gods,"月破")){
		push_many(out.ji,{"嫁娶","搬移","开市","立券交易","纳财","破土","安葬"});
	}
	if(has_text(out.bad_gods,"岁破")){
		push_many(out.ji,{"修造","搬移","嫁娶","开市","安葬"});
	}
	if(has_text(out.bad_gods,"杨公忌")){
		push_many(out.ji,{"开市","修造","嫁娶","立券交易"});
	}
	if(has_text(out.bad_gods,"大耗")){
		push_many(out.ji,{"修仓库","开市","立券交易","纳财","开仓"});
	}
	if(has_text(out.bad_gods,"五鬼")){
		push_many(out.ji,{"出行"});
	}
	if(has_text(out.bad_gods,"咸池")){
		push_many(out.ji,{"嫁娶","取鱼","乘船渡水"});
	}
	if(has_text(out.bad_gods,"血支")){
		push_many(out.ji,{"针刺"});
	}
	if(has_text(out.bad_gods,"天狗")){
		push_many(out.ji,{"祭祀"});
	}
	if(has_text(out.bad_gods,"往亡")){
		push_many(out.ji,{"出行","上官","嫁娶","搬移","求医疗病"});
	}
	if(has_text(out.bad_gods,"四击")){
		push_many(out.ji,{"安抚边境","选将","出师"});
	}
	if(has_text(out.bad_gods,"月刑")){
		push_many(out.ji,{"出行","嫁娶","修造","开市","立券交易","纳财"});
	}
	if(has_text(out.bad_gods,"月害")){
		push_many(out.ji,{"嫁娶","开市","立券交易","纳财","安葬"});
	}

	if(out.d_gz.branch==3){
		out.ji.push_back("穿井");
		rm_text(out.yi,"开渠");
	}
	if(out.d_gz.stem==8){
		out.ji.push_back("开渠");
		rm_text(out.yi,"穿井");
	}
	if(out.d_gz.branch==5){
		out.ji.push_back("出行");
		rm_text(out.yi,"出师");
		rm_text(out.yi,"出行");
	}
	if(out.d_gz.branch==9){
		out.ji.push_back("宴会");
		rm_text(out.yi,"庆赐");
		rm_text(out.yi,"宴会");
	}
	if(out.d_gz.stem==3){
		out.ji.push_back("剃头");
		rm_text(out.yi,"整容");
	}
	if(out.d_gz.branch==11){
		out.ji.push_back("嫁娶");
	}
	if(has_text(out.bad_gods,"天狗")||out.d_gz.branch==2){
		out.ji.push_back("祭祀");
		rm_text(out.yi,"祭祀");
		rm_text(out.yi,"祈福");
		rm_text(out.yi,"求嗣");
	}
	if(has_text(out.bad_gods,"四离")||has_text(out.bad_gods,"四绝")){
		out.yi.clear();
		out.ji.clear();
		out.yi.push_back("诸事不宜");
		out.ji.push_back("诸事不宜");
	}

	uniq_keep_order(out.good_gods);
	uniq_keep_order(out.bad_gods);
	uniq_keep_order(out.yi);
	uniq_keep_order(out.ji);

	int score=0;
	if(is_yellow){
		++score;
	}else{
		--score;
	}
	for(const auto&s : out.good_gods){
		if(s=="天赦"||s.find("德")!=std::string::npos){
			score+=2;
		}else{
			++score;
		}
	}
	for(const auto&s : out.bad_gods){
		if(s=="月破"||s=="岁破"||s=="四离"||s=="四绝"||s=="杨公忌"){
			score-=3;
		}else if(s=="大耗"||s=="月刑"||s=="月害"){
			score-=2;
		}else{
			--score;
		}
	}

	if(score>=2){
		out.yi_ji_level=0;
		out.yi_ji_rule="从宜不从忌";
	}else if(score>=0){
		out.yi_ji_level=1;
		out.yi_ji_rule="从宜亦从忌";
	}else if(score>=-2){
		out.yi_ji_level=2;
		out.yi_ji_rule="从忌不从宜";
	}else{
		out.yi_ji_level=3;
		out.yi_ji_rule="诸事皆忌";
	}

	if(out.yi_ji_level==3){
		out.yi={"诸事不宜"};
		out.ji={"诸事不宜"};
	}else{
		std::set<std::string> inter;
		for(const auto&s : out.yi){
			if(has_text(out.ji,s)){
				inter.insert(s);
			}
		}
		if(out.yi_ji_level==0){
			del_items(out.ji,inter);
		}else if(out.yi_ji_level==1){
			del_items(out.yi,inter);
			del_items(out.ji,inter);
		}else{
			del_items(out.yi,inter);
		}
		if(out.yi.empty()){
			out.yi.push_back("诸事不宜");
		}
		if(out.ji.empty()){
			out.ji.push_back("诸事不忌");
		}
	}

	sort_acts(out.yi);
	sort_acts(out.ji);

	int day_slot_start=(day60*12)%60;
	int mask_today=kHourMask[static_cast<std::size_t>(day60)];
	int mask_next=kHourMask[static_cast<std::size_t>((day60+1)%60)];
	static const std::array<const char*,12> kHourSpan={
		"子(23:00-00:59)","丑(01:00-02:59)","寅(03:00-04:59)","卯(05:00-06:59)",
		"辰(07:00-08:59)","巳(09:00-10:59)","午(11:00-12:59)","未(13:00-14:59)",
		"申(15:00-16:59)","酉(17:00-18:59)","戌(19:00-20:59)","亥(21:00-22:59)"
	};
	out.hour_jx.reserve(13);
	for(int i=0;i<13;++i){
		int idx60=(day_slot_start+i)%60;
		bool bad=false;
		std::string slot_name;
		if(i<12){
			bad=bad_slot(mask_today,i);
			slot_name=kHourSpan[static_cast<std::size_t>(i)];
		}else{
			bad=bad_slot(mask_next,0);
			slot_name="子(次日23:00-00:59)";
		}
		HliHour rec;
		rec.slot=slot_name;
		rec.gz=mk_gz_from_idx(idx60).text;
		rec.luck=bad?"凶":"吉";
		out.hour_jx.push_back(std::move(rec));
	}

	return out;
}
