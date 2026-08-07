#include "lunar/almanac.hpp"

#include<algorithm>
#include<array>
#include<cmath>
#include<cstdint>
#include<initializer_list>
#include<map>
#include<stdexcept>
#include<string_view>

#include "lunar/math.hpp"
#include "lunar/time_scale.hpp"

namespace{

template<typename Enum>
constexpr std::size_t to_idx(Enum code){
	return static_cast<std::size_t>(code);
}

constexpr std::size_t kGodCount=178;
constexpr std::size_t kActCount=87;
constexpr std::size_t kXiuCount=
	static_cast<std::size_t>(HliXiuCode::Zhen)+1;

struct ActSeq{
	std::array<std::uint64_t,2> mask{{0,0}};
	std::vector<int> order;
};

struct GodSeq{
	std::array<std::uint64_t,3> mask{{0,0,0}};
	std::vector<int> order;
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

constexpr std::array<const char*,12> kJianchuText={
	"建","除","满","平","定","执","破","危","成","收","开","闭"
};

constexpr std::array<const char*,kGodCount> kGodText={{
	"青龙","明堂","天刑","朱雀","金匮","天德","白虎","玉堂","天牢","玄武","司命","勾陈",
	"月破","岁破","杨公忌","四离","四绝","三合","六合","月德","月德合","天德合","岁德",
	"岁德合","王日","官日","守日","相日","民日","时德","天贵","天喜","月恩","天赦","大耗",
	"五鬼","咸池","血支","天狗","往亡","四击","月刑","月害",
	"凤凰日","麒麟日","四相","五合","五富","六仪","不将","大葬","鸣吠","小葬","鸣吠对",
	"不守塚","临日","天富","天恩","天愿","天成","天官","天医","天马","驿马","天财",
	"福生","福厚","福德","天巫","地财","月财","月空","母仓","明星","圣心","禄库",
	"吉庆","阴德","活曜","除神","解神","生气","普护","益后","续世","要安","天后",
	"天仓","敬安","玉宇","金堂","吉期","小时","兵福","兵宝","兵吉","天罡","河魁",
	"死神","死气","伏兵","官符","月建","月煞","月厌","月忌","月虚","灾煞","劫煞",
	"厌对","招摇","小红砂","重丧","重复","神号","妨择","披麻","大祸","天吏","天瘟",
	"天狱","天火","天棒","天狗下食","天贼","地囊","地火","独火","受死","黄沙",
	"六不成","小耗","神隔","木马","破败","殃败","雷公","飞廉","大煞","枯鱼",
	"九空","八座","八风触水龙","血忌","阴错","三娘煞","四耗","四穷","四忌",
	"四废","五墓","五虚","五离","八专","九坎","九焦","天转","地转","月建转杀",
	"荒芜","蚩尤","大时","大败","土符","土府","土王用事","游祸","归忌","岁薄",
	"逐阵","阴阳交破","宝日","义日","制日","伐日","专日","重日","复日"
}};

constexpr std::array<const char*,10> kPengStem={
	"甲不开仓 财物耗散","乙不栽植 千株不长","丙不修灶 必见灾殃","丁不剃头 头必生疮","戊不受田 田主不祥",
	"己不破券 二比并亡","庚不经络 织机虚张","辛不合酱 主人不尝","壬不泱水 更难提防","癸不词讼 理弱敌强"
};

constexpr std::array<const char*,12> kPengBranch={
	"子不问卜 自惹祸殃","丑不冠带 主不还乡","寅不祭祀 神鬼不尝","卯不穿井 水泉不香","辰不哭泣 必主重丧","巳不远行 财物伏藏",
	"午不苫盖 屋主更张","未不服药 毒气入肠","申不安床 鬼祟入房","酉不会客 醉坐颠狂","戌不吃犬 作怪上床","亥不嫁娶 不利新郎"
};

constexpr std::array<const char*,kXiuCount> kXiuText={{
	"角木蛟","亢金龙","氐土貉","房日兔","心月狐","尾火虎","箕水豹","斗木獬","牛金牛","女土蝠","虚日鼠","危月燕","室火猪","壁水貐",
	"奎木狼","娄金狗","胃土雉","昴日鸡","毕月乌","觜火猴","参水猿","井木犴","鬼金羊","柳土獐","星日马","张月鹿","翼火蛇","轸水蚓"
}};

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

constexpr std::array<const char*,kActCount> kActText={{
	"祭祀","出行","移徙","结婚姻","宴会","嫁娶","安床","沐浴","剃头","修造","求医疗病","上表章",
	"上官","入学","冠带","进人口","裁衣","竖柱上梁","经络","开市","立券交易","纳财","修置产室","开渠",
	"穿井","安碓硙","扫舍宇","平治道涂","破屋坏垣","伐木","捕捉","畋猎","栽种","牧养","破土","安葬",
	"启攒","施恩","招贤","举正直","临政","解除","整容","整手足甲","裁制","开仓","塞穴","补垣","修饰垣墙",
	"祈福","求嗣","纳采","搬移","营建","筑堤防","安抚边境","选将","诉讼","纳畜","酝酿","修仓库","取鱼",
	"乘船渡水","针刺","出师","庆赐","行船","登高","苫盖","诸事不宜","诸事不忌",
	"上册","修宫室","入宅","宣政事","布政事","开张","恤孤茕","疗目","立券",
	"缮城郭","覃恩","赴任","远回","雪冤","颁诏","鼓铸"
}};

constexpr std::array<HliActCode,71> kActOrder={{
	HliActCode::Sacrifice,HliActCode::Travel,HliActCode::Relocation,HliActCode::ArrangeMarriage,HliActCode::Banquet,HliActCode::Wedding,
	HliActCode::SetBed,HliActCode::Bathing,HliActCode::Haircut,HliActCode::Construction,HliActCode::SeekMedical,HliActCode::SubmitMemorial,
	HliActCode::TakeOffice,HliActCode::EnterSchool,HliActCode::Crowning,HliActCode::AddPeople,HliActCode::CutClothes,HliActCode::Tailoring,
	HliActCode::RaisePillarsBeam,HliActCode::LoomWork,HliActCode::OpenMarket,HliActCode::SignContractTrade,HliActCode::ReceiveWealth,HliActCode::PrepareBirthRoom,
	HliActCode::OpenGranary,HliActCode::RepairStorehouse,HliActCode::OpenCanal,HliActCode::DigWell,HliActCode::SetMillstone,HliActCode::CleanHouse,
	HliActCode::LevelRoads,HliActCode::DecorateWalls,HliActCode::RepairWall,HliActCode::SealHoles,HliActCode::DemolishHouseWall,HliActCode::Logging,
	HliActCode::Capture,HliActCode::Hunting,HliActCode::Planting,HliActCode::Herding,HliActCode::BreakGround,HliActCode::Burial,HliActCode::OpenTomb,
	HliActCode::BestowFavor,HliActCode::RecruitTalent,HliActCode::PromoteUprightness,HliActCode::HandleGovernance,HliActCode::RemoveRelieve,HliActCode::CosmeticGrooming,
	HliActCode::TrimNails,HliActCode::PrayBlessing,HliActCode::PrayOffspring,HliActCode::Betrothal,HliActCode::MoveResidence,HliActCode::BuildProject,
	HliActCode::BuildEmbankment,HliActCode::PacifyBorder,HliActCode::SelectGenerals,HliActCode::Litigation,HliActCode::AcquireLivestock,HliActCode::BrewFermentation,
	HliActCode::Fishing,HliActCode::BoatCrossing,HliActCode::Needling,HliActCode::DeployTroops,HliActCode::CelebrationGrant,HliActCode::Sailing,
	HliActCode::ClimbHeights,HliActCode::ThatchCovering,HliActCode::AvoidAll,HliActCode::NoMajorTaboo
}};

#include "almanac_rule_data.inc"

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

std::string to_low_ascii(std::string text){
	for(char&ch : text){
		if(ch>='A'&&ch<='Z'){
			ch=static_cast<char>(ch-'A'+'a');
		}
	}
	return text;
}

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

const char*act_text(HliActCode code){
	return kActText[to_idx(code)];
}

const char*god_text(HliGodCode code){
	return kGodText[to_idx(code)];
}

const char*god_text(int code){
	if(code<0||code>=static_cast<int>(kGodText.size())){
		return "";
	}
	return kGodText[static_cast<std::size_t>(code)];
}

int god_code(const char*text){
	for(std::size_t i=0;i<kGodText.size();++i){
		if(std::string_view(kGodText[i])==text){
			return static_cast<int>(i);
		}
	}
	throw std::logic_error(std::string("unknown almanac god: ")+text);
}

int act_code(const char*text){
	for(std::size_t i=0;i<kActText.size();++i){
		if(std::string_view(kActText[i])==text){
			return static_cast<int>(i);
		}
	}
	throw std::logic_error(std::string("unknown almanac activity: ")+text);
}

int find_act_code(const char*text){
	for(std::size_t i=0;i<kActText.size();++i){
		if(std::string_view(kActText[i])==text){
			return static_cast<int>(i);
		}
	}
	return -1;
}

const char*xiu_text(int code){
	if(code<0||code>=static_cast<int>(kXiuCount)){
		return "";
	}
	return kXiuText[static_cast<std::size_t>(code)];
}

bool act_has(const ActSeq&seq,HliActCode code){
	int idx=static_cast<int>(code);
	int word=idx/64;
	int bit=idx%64;
	return (seq.mask[static_cast<std::size_t>(word)]&(std::uint64_t{1}<<bit))!=0;
}

bool act_has(const ActSeq&seq,int code){
	int word=code/64;
	int bit=code%64;
	return (seq.mask[static_cast<std::size_t>(word)]&
			(std::uint64_t{1}<<bit))!=0;
}

bool act_has(const ActSeq&seq,const char*text){
	int code=find_act_code(text);
	return code>=0&&act_has(seq,code);
}

void act_add(ActSeq&seq,int code){
	int word=code/64;
	int bit=code%64;
	std::uint64_t mask=std::uint64_t{1}<<bit;
	if((seq.mask[static_cast<std::size_t>(word)]&mask)!=0){
		return;
	}
	seq.mask[static_cast<std::size_t>(word)]|=mask;
	seq.order.push_back(code);
}

void act_add(ActSeq&seq,HliActCode code){
	act_add(seq,static_cast<int>(code));
}

void act_add(ActSeq&seq,const char*text){
	act_add(seq,act_code(text));
}

void act_add_many(ActSeq&seq,std::initializer_list<HliActCode> list){
	for(HliActCode code : list){
		act_add(seq,code);
	}
}

void act_remove(ActSeq&seq,HliActCode code){
	int idx=static_cast<int>(code);
	int word=idx/64;
	int bit=idx%64;
	seq.mask[static_cast<std::size_t>(word)]&=~(std::uint64_t{1}<<bit);
	seq.order.erase(std::remove(seq.order.begin(),seq.order.end(),idx),
					seq.order.end());
}

void act_remove(ActSeq&seq,int code){
	int word=code/64;
	int bit=code%64;
	seq.mask[static_cast<std::size_t>(word)]&=~(std::uint64_t{1}<<bit);
	seq.order.erase(std::remove(seq.order.begin(),seq.order.end(),code),
					seq.order.end());
}

void act_remove(ActSeq&seq,const char*text){
	int code=find_act_code(text);
	if(code>=0){
		act_remove(seq,code);
	}
}

bool god_has(const GodSeq&seq,HliGodCode code){
	int idx=static_cast<int>(code);
	return (seq.mask[static_cast<std::size_t>(idx/64)]&
			(std::uint64_t{1}<<(idx%64)))!=0;
}

bool god_has(const GodSeq&seq,int code){
	return (seq.mask[static_cast<std::size_t>(code/64)]&
			(std::uint64_t{1}<<(code%64)))!=0;
}

bool god_has(const GodSeq&seq,const char*text){
	for(std::size_t i=0;i<kGodText.size();++i){
		if(std::string_view(kGodText[i])==text){
			return god_has(seq,static_cast<int>(i));
		}
	}
	return false;
}

void god_add(GodSeq&seq,int code){
	std::size_t word=static_cast<std::size_t>(code/64);
	std::uint64_t mask=std::uint64_t{1}<<(code%64);
	if((seq.mask[word]&mask)!=0){
		return;
	}
	seq.mask[word]|=mask;
	seq.order.push_back(code);
}

void god_add(GodSeq&seq,HliGodCode code){
	god_add(seq,static_cast<int>(code));
}

void god_add(GodSeq&seq,const char*text){
	god_add(seq,god_code(text));
}

bool in_set_branch(int b,std::initializer_list<int> a){
	for(int x : a){
		if(b==x){
			return true;
		}
	}
	return false;
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

std::string tri_to_dir(const char*tri){
	for(std::size_t i=0;i<kTri8.size();++i){
		if(std::string(kTri8[i])==tri){
			return kDir8[i];
		}
	}
	return "正北";
}

int tri_to_dir_code(const char*tri){
	for(std::size_t i=0;i<kTri8.size();++i){
		if(std::string(kTri8[i])==tri){
			return static_cast<int>(i);
		}
	}
	return 0;
}

const char*dir_text(int code){
	if(code<0||code>=static_cast<int>(kDir8.size())){
		return kDir8[0];
	}
	return kDir8[static_cast<std::size_t>(code)];
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

int sha_dir_code(int b){
	if(in_set_branch(b,{8,0,4})){
		return 4;
	}
	if(in_set_branch(b,{2,6,10})){
		return 0;
	}
	if(in_set_branch(b,{11,3,7})){
		return 6;
	}
	return 2;
}

int three_he_group_code(int b){
	if(in_set_branch(b,{8,0,4})){
		return 0;
	}
	if(in_set_branch(b,{11,3,7})){
		return 1;
	}
	if(in_set_branch(b,{2,6,10})){
		return 2;
	}
	return 3;
}

void apply_base_act(HliJianchuCode jc,ActSeq&yi,ActSeq&ji){
	switch(jc){
		case HliJianchuCode::Build:
			act_add_many(yi,{HliActCode::BestowFavor,HliActCode::RecruitTalent,
							 HliActCode::PromoteUprightness,HliActCode::Travel,
							 HliActCode::TakeOffice,HliActCode::HandleGovernance});
			return;
		case HliJianchuCode::Remove:
			act_add_many(yi,{HliActCode::RemoveRelieve,HliActCode::Bathing,
							 HliActCode::CosmeticGrooming,HliActCode::Haircut,
							 HliActCode::TrimNails,HliActCode::SeekMedical,
							 HliActCode::CleanHouse});
			return;
		case HliJianchuCode::Full:
			act_add_many(yi,{HliActCode::AddPeople,HliActCode::Tailoring,
							 HliActCode::RaisePillarsBeam,HliActCode::LoomWork,
							 HliActCode::OpenMarket,HliActCode::SignContractTrade,
							 HliActCode::ReceiveWealth,HliActCode::OpenGranary,
							 HliActCode::SealHoles,HliActCode::RepairWall});
			act_add_many(ji,{HliActCode::BestowFavor,HliActCode::RecruitTalent,
							 HliActCode::PromoteUprightness,HliActCode::TakeOffice,
							 HliActCode::HandleGovernance,HliActCode::ArrangeMarriage,
							 HliActCode::Betrothal,HliActCode::SeekMedical});
			return;
		case HliJianchuCode::Balance:
			act_add_many(yi,{HliActCode::DecorateWalls,HliActCode::LevelRoads});
			act_add_many(ji,{HliActCode::PrayBlessing,HliActCode::PrayOffspring,
							 HliActCode::Banquet,HliActCode::Crowning,HliActCode::Travel,
							 HliActCode::TakeOffice,HliActCode::HandleGovernance,
							 HliActCode::ArrangeMarriage,HliActCode::Betrothal,
							 HliActCode::Wedding,HliActCode::MoveResidence,
							 HliActCode::SetBed,HliActCode::SeekMedical,
							 HliActCode::BuildProject,HliActCode::Construction,
							 HliActCode::OpenMarket,HliActCode::SignContractTrade,
							 HliActCode::ReceiveWealth,HliActCode::OpenGranary,
							 HliActCode::BreakGround,HliActCode::Burial,
							 HliActCode::OpenTomb});
			return;
		case HliJianchuCode::Settle:
			act_add(yi,HliActCode::Crowning);
			return;
		case HliJianchuCode::Hold:
			act_add(yi,HliActCode::Capture);
			return;
		case HliJianchuCode::Break:
			act_add(yi,HliActCode::SeekMedical);
			act_add_many(ji,{HliActCode::Wedding,HliActCode::OpenMarket,
							 HliActCode::Burial});
			return;
		case HliJianchuCode::Danger:
			act_add_many(yi,{HliActCode::PacifyBorder,HliActCode::SelectGenerals,
							 HliActCode::SetBed});
			act_add_many(ji,{HliActCode::ClimbHeights,HliActCode::Sailing});
			return;
		case HliJianchuCode::Success:
			act_add_many(yi,{HliActCode::EnterSchool,HliActCode::PacifyBorder,
							 HliActCode::MoveResidence,HliActCode::BuildEmbankment,
							 HliActCode::OpenMarket});
			act_add(ji,HliActCode::Litigation);
			return;
		case HliJianchuCode::Receive:
			act_add_many(yi,{HliActCode::AddPeople,HliActCode::ReceiveWealth,
							 HliActCode::Capture,HliActCode::AcquireLivestock});
			act_add_many(ji,{HliActCode::ArrangeMarriage,HliActCode::Betrothal,
							 HliActCode::Wedding,HliActCode::OpenMarket,
							 HliActCode::SignContractTrade,HliActCode::OpenGranary,
							 HliActCode::Burial});
			return;
		case HliJianchuCode::Open:
			act_add_many(yi,{HliActCode::Sacrifice,HliActCode::PrayBlessing,
							 HliActCode::PrayOffspring,HliActCode::SubmitMemorial,
							 HliActCode::BestowFavor,HliActCode::RecruitTalent,
							 HliActCode::PromoteUprightness,HliActCode::Banquet,
							 HliActCode::EnterSchool,HliActCode::Travel,
							 HliActCode::TakeOffice,HliActCode::HandleGovernance,
							 HliActCode::MoveResidence,HliActCode::SeekMedical,
							 HliActCode::Construction,HliActCode::OpenMarket,
							 HliActCode::OpenCanal,HliActCode::DigWell,
							 HliActCode::Planting,HliActCode::Herding});
			return;
		case HliJianchuCode::Close:
			act_add_many(yi,{HliActCode::BuildEmbankment,HliActCode::SealHoles,
							 HliActCode::RepairWall});
			act_add_many(ji,{HliActCode::SubmitMemorial,HliActCode::BestowFavor,
							 HliActCode::RecruitTalent,HliActCode::PromoteUprightness,
							 HliActCode::Banquet,HliActCode::Travel,
							 HliActCode::TakeOffice,HliActCode::HandleGovernance,
							 HliActCode::ArrangeMarriage,HliActCode::Betrothal,
							 HliActCode::Wedding,HliActCode::AddPeople,
							 HliActCode::MoveResidence,HliActCode::SetBed,
							 HliActCode::SeekMedical,HliActCode::Construction,
							 HliActCode::RaisePillarsBeam,HliActCode::OpenMarket,
							 HliActCode::OpenGranary,HliActCode::OpenCanal,
							 HliActCode::DigWell});
			return;
	}
}

void add_stem_branch_acts(int day_stem,int day_branch,ActSeq&yi,ActSeq&ji){
	switch(day_stem){
		case 0:
			act_add(ji,HliActCode::OpenGranary);
			break;
		case 1:
			act_add(ji,HliActCode::Planting);
			break;
		case 3:
			act_add_many(ji,{HliActCode::CosmeticGrooming,HliActCode::Haircut});
			break;
		case 6:
			act_add(ji,HliActCode::LoomWork);
			break;
		case 7:
			act_add(ji,HliActCode::BrewFermentation);
			break;
		case 8:
			act_add_many(ji,{HliActCode::OpenCanal,HliActCode::DigWell});
			break;
		default:
			break;
	}
	switch(day_branch){
		case 0:
			act_add(yi,HliActCode::Bathing);
			break;
		case 1:
			act_add(ji,HliActCode::Crowning);
			break;
		case 2:
			act_add(ji,HliActCode::Sacrifice);
			break;
		case 3:
			act_add(ji,HliActCode::DigWell);
			break;
		case 5:
			act_add(ji,HliActCode::Travel);
			break;
		case 6:
			act_add(ji,HliActCode::ThatchCovering);
			break;
		case 7:
			act_add(ji,HliActCode::SeekMedical);
			break;
		case 8:
			act_add(ji,HliActCode::SetBed);
			break;
		case 9:
			act_add(ji,HliActCode::Banquet);
			break;
		case 11:
			act_add(yi,HliActCode::Bathing);
			act_add(ji,HliActCode::Wedding);
			break;
		default:
			break;
	}
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

int xiu_mod28_code(double moon_lam){
	double step=TWO_PI/28.0;
	int idx=static_cast<int>(std::floor(norm_rad(moon_lam)/step));
	idx%=28;
	if(idx<0){
		idx+=28;
	}
	return idx;
}

int xiu_exact_code(const std::string&region,double moon_lam){
	static const std::map<std::string,int> table={
		{"角宿",static_cast<int>(HliXiuCode::Jiao)},
		{"亢宿",static_cast<int>(HliXiuCode::Kang)},
		{"氐宿",static_cast<int>(HliXiuCode::Di)},
		{"房宿",static_cast<int>(HliXiuCode::Fang)},
		{"心宿",static_cast<int>(HliXiuCode::Xin)},
		{"尾宿",static_cast<int>(HliXiuCode::Wei)},
		{"箕宿",static_cast<int>(HliXiuCode::Ji)},
		{"斗宿",static_cast<int>(HliXiuCode::Dou)},
		{"牛宿",static_cast<int>(HliXiuCode::Niu)},
		{"女宿",static_cast<int>(HliXiuCode::Nu)},
		{"虚宿",static_cast<int>(HliXiuCode::Xu)},
		{"危宿",static_cast<int>(HliXiuCode::WeiYan)},
		{"室宿",static_cast<int>(HliXiuCode::Shi)},
		{"壁宿",static_cast<int>(HliXiuCode::Bi)},
		{"奎宿",static_cast<int>(HliXiuCode::Kui)},
		{"娄宿",static_cast<int>(HliXiuCode::Lou)},
		{"胃宿",static_cast<int>(HliXiuCode::WeiZhi)},
		{"昴宿",static_cast<int>(HliXiuCode::Mao)},
		{"毕宿",static_cast<int>(HliXiuCode::BiWu)},
		{"觜宿",static_cast<int>(HliXiuCode::Zui)},
		{"参宿",static_cast<int>(HliXiuCode::Shen)},
		{"井宿",static_cast<int>(HliXiuCode::Jing)},
		{"鬼宿",static_cast<int>(HliXiuCode::Gui)},
		{"柳宿",static_cast<int>(HliXiuCode::Liu)},
		{"星宿",static_cast<int>(HliXiuCode::Xing)},
		{"张宿",static_cast<int>(HliXiuCode::Zhang)},
		{"翼宿",static_cast<int>(HliXiuCode::Yi)},
		{"轸宿",static_cast<int>(HliXiuCode::Zhen)}
	};
	auto it=table.find(region);
	if(it!=table.end()){
		return it->second;
	}
	return xiu_mod28_code(moon_lam);
}

void sort_act_codes(std::vector<int>&codes){
	static const std::array<int,kActCount> rank=[](){
		std::array<int,kActCount> out{};
		for(std::size_t i=0;i<out.size();++i){
			out[i]=9999;
		}
		for(std::size_t i=0;i<kActOrder.size();++i){
			out[to_idx(kActOrder[i])]=static_cast<int>(i);
		}
		return out;
	}();
	std::sort(codes.begin(),codes.end(),[&](int a,int b){
		int ra=(a>=0&&a<static_cast<int>(rank.size()))?rank[static_cast<std::size_t>(a)]:9999;
		int rb=(b>=0&&b<static_cast<int>(rank.size()))?rank[static_cast<std::size_t>(b)]:9999;
		if(ra!=rb){
			return ra<rb;
		}
		return a<b;
	});
}

void sync_codes(HliData&out,const GodSeq&good,const GodSeq&bad,const ActSeq&yi,
			   const ActSeq&ji){
	out.good_god_mask=good.mask[0];
	out.bad_god_mask=bad.mask[0];
	out.yi_mask=yi.mask;
	out.ji_mask=ji.mask;
	out.good_god_codes=good.order;
	out.bad_god_codes=bad.order;
	out.yi_codes=yi.order;
	out.ji_codes=ji.order;
}

void materialize_zh(HliData&out,int day60,int meridian_branch){
	switch(static_cast<HliProfileCode>(out.rule_profile_code)){
		case HliProfileCode::ZiPing:
			out.rule_profile="子平八字";
			break;
		case HliProfileCode::PurpleStar:
			out.rule_profile="紫微斗数";
			break;
		case HliProfileCode::XieJi:
			out.rule_profile="协纪辨方书";
			break;
		case HliProfileCode::Custom:
			out.rule_profile="自定义";
			break;
		case HliProfileCode::Folk:
		default:
			out.rule_profile="民俗黄历";
			break;
	}
	switch(static_cast<HliYearBoundary>(out.year_boundary_code)){
		case HliYearBoundary::LiChun:
			out.year_boundary_text="立春换年";
			break;
		case HliYearBoundary::WinterSolstice:
			out.year_boundary_text="冬至换年";
			break;
		case HliYearBoundary::LunarNewYear:
		default:
			out.year_boundary_text="正月初一换年";
			break;
	}
	switch(static_cast<HliMonthBoundary>(out.month_boundary_code)){
		case HliMonthBoundary::SolarTerm:
			out.month_boundary_text="节气换月";
			break;
		case HliMonthBoundary::LunarFirstDay:
		default:
			out.month_boundary_text="初一换月";
			break;
	}
	switch(static_cast<HliLeapMonthMode>(out.leap_month_mode_code)){
		case HliLeapMonthMode::Ignore:
			out.leap_month_mode_text="忽略闰月";
			break;
		case HliLeapMonthMode::SplitMidway:
			out.leap_month_mode_text="闰月前半随前月后半作下月";
			break;
		case HliLeapMonthMode::ShiftToNext:
			out.leap_month_mode_text="闰月作下月";
			break;
		case HliLeapMonthMode::InheritPrevious:
		default:
			out.leap_month_mode_text="闰月随前月";
			break;
	}
	switch(static_cast<HliDayBoundary>(out.day_boundary_code)){
		case HliDayBoundary::Hour0:
			out.day_boundary_text="子正换日";
			break;
		case HliDayBoundary::Hour23:
		default:
			out.day_boundary_text="子初换日";
			break;
	}

	out.jianchu=(out.jianchu_code>=0&&out.jianchu_code<12)
					?kJianchuText[static_cast<std::size_t>(out.jianchu_code)]
					:"";
	out.duty_god=(out.duty_god_code>=0&&out.duty_god_code<static_cast<int>(kGodCount))
					 ?god_text(static_cast<HliGodCode>(out.duty_god_code))
					 :"";
	out.duty_tag_code=out.duty_is_yellow?1:0;
	out.duty_tag=out.duty_is_yellow?"黄道日":"黑道日";

	int clash_b=(out.d_gz.branch+6)%12;
	out.clash_branch_code=clash_b;
	out.sha_dir_code=sha_dir_code(out.d_gz.branch);
	out.zodiac_day_code=out.d_gz.branch;
	out.clash=std::string(kZodiac[static_cast<std::size_t>(out.d_gz.branch)])+
			  "日冲"+kZodiac[static_cast<std::size_t>(clash_b)];
	out.chong_sha=std::string("冲")+
				  kZodiac[static_cast<std::size_t>(clash_b)]+"煞"+
				  dir_text(out.sha_dir_code);
	out.zodiac_day=kZodiac[static_cast<std::size_t>(out.d_gz.branch)];

	static const std::array<int,12> kLiuHe={1,0,11,10,9,8,7,6,5,4,3,2};
	out.six_he_branch_code=
		kLiuHe[static_cast<std::size_t>(out.d_gz.branch)];
	out.three_he_group_code=three_he_group_code(out.d_gz.branch);
	out.six_he=std::string(kBranch[static_cast<std::size_t>(out.d_gz.branch)])+
			   "合"+
			   kBranch[static_cast<std::size_t>(
				   out.six_he_branch_code)];
	out.three_he=three_he_text(out.d_gz.branch);
	out.pengzu=std::string(kPengStem[static_cast<std::size_t>(out.d_gz.stem)])+
			   " "+kPengBranch[static_cast<std::size_t>(out.d_gz.branch)];
	out.nayin_code=day60/2;
	out.nayin=kNayin[static_cast<std::size_t>(day60/2)];
	out.wx_day=std::string("天干")+kStem[static_cast<std::size_t>(out.d_gz.stem)]+"属"+
			   kStemWx[static_cast<std::size_t>(out.d_gz.stem)]+" 地支"+
			   kBranch[static_cast<std::size_t>(out.d_gz.branch)]+"属"+
			   kBranchWx[static_cast<std::size_t>(out.d_gz.branch)]+" 纳音"+
			   out.nayin;
	out.fetal_god_code=day60;
	out.fetal_god=kFetalGod[static_cast<std::size_t>(day60)];
	out.meridian_code=meridian_branch;
	out.meridian=kMeridian[static_cast<std::size_t>(meridian_branch)];
	out.lucky_dir_code=
		tri_to_dir_code(kLuckTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.wealth_dir_code=
		tri_to_dir_code(kWealthTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.mascot_dir_code=
		tri_to_dir_code(kMascotTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.sun_noble_dir_code=
		tri_to_dir_code(kSunNobleTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.moon_noble_dir_code=
		tri_to_dir_code(kMoonNobleTri[static_cast<std::size_t>(out.d_gz.stem)]);
	out.lucky_dir=dir_text(out.lucky_dir_code);
	out.wealth_dir=dir_text(out.wealth_dir_code);
	out.mascot_dir=dir_text(out.mascot_dir_code);
	out.sun_noble_dir=dir_text(out.sun_noble_dir_code);
	out.moon_noble_dir=dir_text(out.moon_noble_dir_code);
	out.xiu28=xiu_text(out.xiu28_code);
	out.xiu28_mod28=xiu_text(out.xiu28_mod28_code);

	out.good_gods.clear();
	out.bad_gods.clear();
	out.yi.clear();
	out.ji.clear();
	out.good_gods.reserve(out.good_god_codes.size());
	out.bad_gods.reserve(out.bad_god_codes.size());
	out.yi.reserve(out.yi_codes.size());
	out.ji.reserve(out.ji_codes.size());
	for(int code : out.good_god_codes){
		out.good_gods.push_back(god_text(static_cast<HliGodCode>(code)));
	}
	for(int code : out.bad_god_codes){
		out.bad_gods.push_back(god_text(static_cast<HliGodCode>(code)));
	}
	for(int code : out.yi_codes){
		out.yi.push_back(act_text(static_cast<HliActCode>(code)));
	}
	for(int code : out.ji_codes){
		out.ji.push_back(act_text(static_cast<HliActCode>(code)));
	}
	switch(static_cast<HliRuleCode>(out.yi_ji_rule_code)){
		case HliRuleCode::FollowYiIgnoreJi:
			out.yi_ji_rule="从宜不从忌";
			break;
		case HliRuleCode::FollowBoth:
			out.yi_ji_rule="从宜亦从忌";
			break;
		case HliRuleCode::FollowJiIgnoreYi:
			out.yi_ji_rule="从忌不从宜";
			break;
		case HliRuleCode::AvoidEverything:
			out.yi_ji_rule="诸事皆忌";
			break;
	}
}

int good_score(HliGodCode code){
	switch(code){
		case HliGodCode::HeavenlyVirtue:
		case HliGodCode::MonthVirtue:
		case HliGodCode::MonthVirtueCombine:
		case HliGodCode::HeavenlyVirtueCombine:
		case HliGodCode::YearlyVirtue:
		case HliGodCode::YearlyVirtueCombine:
		case HliGodCode::HeavenlyPardon:
			return 2;
		default:
			return 1;
	}
}

int bad_score(HliGodCode code){
	switch(code){
		case HliGodCode::MonthBreak:
		case HliGodCode::YearBreak:
		case HliGodCode::YangGongTaboo:
		case HliGodCode::FourSeparations:
		case HliGodCode::FourExtinctions:
			return -3;
		case HliGodCode::GreatLoss:
		case HliGodCode::MonthlyPunishment:
		case HliGodCode::MonthlyHarm:
			return -2;
		default:
			return -1;
	}
}

int effective_day_number(double jd_utc,int tz_off,HliDayBoundary boundary){
	double jd_local=jd_utc+static_cast<double>(tz_off)/1440.0;
	if(boundary==HliDayBoundary::Hour23){
		jd_local+=1.0/24.0;
	}
	return static_cast<int>(std::floor(jd_local+0.5));
}

int lunar_month_ord(int month_1_12,bool is_leap,int day_in_month,
					HliLeapMonthMode leap_mode){
	if(!is_leap){
		return pos_mod(static_cast<long long>(month_1_12)+11,12);
	}
	switch(leap_mode){
		case HliLeapMonthMode::Ignore:
		case HliLeapMonthMode::InheritPrevious:
			return pos_mod(static_cast<long long>(month_1_12)+11,12);
		case HliLeapMonthMode::SplitMidway:
			if(day_in_month<=15){
				return pos_mod(static_cast<long long>(month_1_12)+11,12);
			}
			return pos_mod(static_cast<long long>(month_1_12),12);
		case HliLeapMonthMode::ShiftToNext:
			return pos_mod(static_cast<long long>(month_1_12),12);
	}
	return pos_mod(static_cast<long long>(month_1_12)+11,12);
}

int y_for_dongzhi(LunCal6&lc,double jd_utc,int gy){
	double jd_dz=lc.get_st("Z11",gy).toUtcJD();
	if(jd_utc<jd_dz){
		return gy-1;
	}
	return gy;
}

GzNode select_year_gz(const HliData&out,HliYearBoundary boundary,LunCal6&lc,
					  double jd_utc,int gy){
	switch(boundary){
		case HliYearBoundary::LiChun:
			return out.y_lchun;
		case HliYearBoundary::LunarNewYear:
			return out.y_lun;
		case HliYearBoundary::WinterSolstice:
			return mk_gz_from_year(y_for_dongzhi(lc,jd_utc,gy));
	}
	return out.y_lun;
}

GzNode month_gz_solar(const GzNode&year_gz,int month_branch_idx){
	int mon_off=pos_mod(static_cast<long long>(month_branch_idx)-2,12);
	int mon_st=((year_gz.stem%5)*2+2+mon_off)%10;
	GzNode out;
	out.stem=mon_st;
	out.branch=month_branch_idx;
	out.text=gz_text(mon_st,month_branch_idx);
	return out;
}

GzNode month_gz_lunar(const GzNode&year_gz,int lun_month,bool is_leap,int lun_day,
					  HliLeapMonthMode leap_mode){
	int month_ord=lunar_month_ord(lun_month,is_leap,lun_day,leap_mode);
	int month_branch_idx=(month_ord+2)%12;
	int mon_st=((year_gz.stem%5)*2+2+month_ord)%10;
	GzNode out;
	out.stem=mon_st;
	out.branch=month_branch_idx;
	out.text=gz_text(mon_st,month_branch_idx);
	return out;
}

#include "almanac_rule_engine.inc"

HliRuleSet rule_set_from_profile(HliProfileCode profile){
	switch(profile){
		case HliProfileCode::ZiPing:
			return {static_cast<int>(profile),
					static_cast<int>(HliYearBoundary::LiChun),
					static_cast<int>(HliMonthBoundary::SolarTerm),
					static_cast<int>(HliLeapMonthMode::Ignore),
					static_cast<int>(HliDayBoundary::Hour23)};
		case HliProfileCode::PurpleStar:
			return {static_cast<int>(profile),
					static_cast<int>(HliYearBoundary::LunarNewYear),
					static_cast<int>(HliMonthBoundary::LunarFirstDay),
					static_cast<int>(HliLeapMonthMode::SplitMidway),
					static_cast<int>(HliDayBoundary::Hour0)};
		case HliProfileCode::XieJi:
			return {static_cast<int>(profile),
					static_cast<int>(HliYearBoundary::LunarNewYear),
					static_cast<int>(HliMonthBoundary::SolarTerm),
					static_cast<int>(HliLeapMonthMode::InheritPrevious),
					static_cast<int>(HliDayBoundary::Hour23)};
		case HliProfileCode::Custom:
			return {static_cast<int>(profile),
					static_cast<int>(HliYearBoundary::LunarNewYear),
					static_cast<int>(HliMonthBoundary::LunarFirstDay),
					static_cast<int>(HliLeapMonthMode::InheritPrevious),
					static_cast<int>(HliDayBoundary::Hour23)};
		case HliProfileCode::Folk:
		default:
			return {static_cast<int>(HliProfileCode::Folk),
					static_cast<int>(HliYearBoundary::LunarNewYear),
					static_cast<int>(HliMonthBoundary::LunarFirstDay),
					static_cast<int>(HliLeapMonthMode::InheritPrevious),
					static_cast<int>(HliDayBoundary::Hour23)};
	}
}

}

HliRuleSet make_hli_rule_set(HliProfileCode profile){
	return rule_set_from_profile(profile);
}

HliRuleSet normalize_hli_rule_set(const HliRuleSet&rules){
	HliRuleSet out=rules;
	const std::array<HliProfileCode,4> presets={
		HliProfileCode::Folk,
		HliProfileCode::ZiPing,
		HliProfileCode::PurpleStar,
		HliProfileCode::XieJi
	};
	for(HliProfileCode profile : presets){
		HliRuleSet base=rule_set_from_profile(profile);
		if(out.year_boundary==base.year_boundary&&
		   out.month_boundary==base.month_boundary&&
		   out.leap_month_mode==base.leap_month_mode&&
		   out.day_boundary==base.day_boundary){
			out.profile_code=static_cast<int>(profile);
			return out;
		}
	}
	out.profile_code=static_cast<int>(HliProfileCode::Custom);
	return out;
}

bool parse_hli_profile(const std::string&text,HliProfileCode*out){
	if(out==nullptr){
		return false;
	}
	const std::string key=to_low_ascii(text);
	if(key=="folk"||key=="huangli"||key=="huang_li"||key=="almanac"||
	   key=="folk_almanac"||key=="laohuangli"||key=="old_huangli"||
	   key=="old_almanac"||key=="tongshu"||key=="tong_shu"||
	   key=="tongsheng"||key=="tong_sheng"||text=="民俗"||text=="黄历"||
	   text=="老黄历"||text=="民俗黄历"||text=="通书"||text=="通胜"){
		*out=HliProfileCode::Folk;
		return true;
	}
	if(key=="ziping"||key=="zipingbazi"||key=="bazi"||key=="zi_ping"||
	   key=="zi_ping_ba_zi"||key=="zi_ping_bazi"||
	   key=="fourpillars"||key=="four_pillars"||key=="fengshui"||
	   text=="子平"||text=="八字"||text=="四柱"||text=="风水"||
	   text=="子平八字"||text=="四柱八字"){
		*out=HliProfileCode::ZiPing;
		return true;
	}
	if(key=="purple"||key=="purplestar"||key=="ziwei"||
	   key=="ziweidoushu"||key=="zi_wei_dou_shu"||key=="purple_star"||
	   key=="zhongzhou"||key=="zhong_zhou"||text=="紫微"||text=="斗数"||
	   text=="紫微斗数"||text=="中州派"){
		*out=HliProfileCode::PurpleStar;
		return true;
	}
	if(key=="xieji"||key=="xie_ji"||key=="xieji_bianfang"||
	   key=="xie_ji_bian_fang"||key=="xiejibianfangshu"||
	   key=="xie_ji_bian_fang_shu"||key=="auspicious_selection"||
	   text=="协纪"||text=="择吉"||text=="择日"||text=="协纪择吉"||
	   text=="协纪辨方"||text=="协纪辨方书"){
		*out=HliProfileCode::XieJi;
		return true;
	}
	if(key=="custom"||text=="自定义"){
		*out=HliProfileCode::Custom;
		return true;
	}
	return false;
}

std::string hli_profile_key(HliProfileCode profile){
	switch(profile){
		case HliProfileCode::ZiPing:
			return "ziping";
		case HliProfileCode::PurpleStar:
			return "purple";
		case HliProfileCode::XieJi:
			return "xieji";
		case HliProfileCode::Custom:
			return "custom";
		case HliProfileCode::Folk:
		default:
			return "folk";
	}
}

bool parse_hli_year_boundary(const std::string&text,HliYearBoundary*out){
	if(out==nullptr){
		return false;
	}
	const std::string key=to_low_ascii(text);
	if(key=="lichun"||key=="li_chun"||key=="spring_begin"||
	   key=="li_chun_boundary"||text=="立春"||text=="立春换年"||
	   text=="立春交节"){
		*out=HliYearBoundary::LiChun;
		return true;
	}
	if(key=="lunarnewyear"||key=="lunar_new_year"||key=="newyear"||
	   key=="new_year"||key=="springfestival"||key=="spring_festival"||
	   key=="chunjie"||text=="正月初一"||text=="春节"||
	   text=="农历新年"||text=="正月初一换年"){
		*out=HliYearBoundary::LunarNewYear;
		return true;
	}
	if(key=="dongzhi"||key=="wintersolstice"||key=="winter_solstice"||
	   key=="winter_solstice_boundary"||text=="冬至"||text=="冬至换年"||
	   text=="冬至交节"){
		*out=HliYearBoundary::WinterSolstice;
		return true;
	}
	return false;
}

bool parse_hli_month_boundary(const std::string&text,HliMonthBoundary*out){
	if(out==nullptr){
		return false;
	}
	const std::string key=to_low_ascii(text);
	if(key=="solarterm"||key=="solar_term"||key=="solar_terms"||
	   key=="jieqi"||key=="jie"||text=="节气"||text=="节气换月"||
	   text=="交节"){
		*out=HliMonthBoundary::SolarTerm;
		return true;
	}
	if(key=="lunarfirstday"||key=="lunar_first_day"||key=="firstday"||
	   key=="first_day"||key=="month_start"||text=="初一"||
	   text=="初一换月"||text=="农历初一"||text=="朔日"){
		*out=HliMonthBoundary::LunarFirstDay;
		return true;
	}
	return false;
}

bool parse_hli_leap_month_mode(const std::string&text,HliLeapMonthMode*out){
	if(out==nullptr){
		return false;
	}
	const std::string key=to_low_ascii(text);
	if(key=="ignore"||key=="disabled"||text=="忽略闰月"){
		*out=HliLeapMonthMode::Ignore;
		return true;
	}
	if(key=="inherit"||key=="inheritprevious"||
	   key=="inherit_previous"||key=="follow_prev"||
	   key=="follow_previous"||text=="闰月随前月"||text=="闰月沿前月"){
		*out=HliLeapMonthMode::InheritPrevious;
		return true;
	}
	if(key=="split"||key=="splitmidway"||key=="split_midway"||
	   key=="half_split"||text=="闰月前半随前月后半作下月"||
	   text=="闰月中分"){
		*out=HliLeapMonthMode::SplitMidway;
		return true;
	}
	if(key=="shift"||key=="shifttonext"||key=="shift_to_next"||
	   key=="next_month"||text=="闰月作下月"||text=="闰月算下月"){
		*out=HliLeapMonthMode::ShiftToNext;
		return true;
	}
	return false;
}

bool parse_hli_day_boundary(const std::string&text,HliDayBoundary*out){
	if(out==nullptr){
		return false;
	}
	const std::string key=to_low_ascii(text);
	if(key=="hour23"||key=="23"||key=="23h"||key=="2300"||key=="zichu"||
	   key=="zi_chu"||text=="子初"||text=="子初换日"||text=="23点"||
	   text=="23时"){
		*out=HliDayBoundary::Hour23;
		return true;
	}
	if(key=="hour0"||key=="0"||key=="00h"||key=="0000"||key=="midnight"||
	   key=="zizheng"||key=="zi_zheng"||text=="子正"||text=="子正换日"||
	   text=="0点"||text=="0时"||text=="午夜"){
		*out=HliDayBoundary::Hour0;
		return true;
	}
	return false;
}

std::string hli_year_boundary_key(HliYearBoundary boundary){
	switch(boundary){
		case HliYearBoundary::LiChun:
			return "lichun";
		case HliYearBoundary::WinterSolstice:
			return "dongzhi";
		case HliYearBoundary::LunarNewYear:
		default:
			return "lunar_new_year";
	}
}

std::string hli_month_boundary_key(HliMonthBoundary boundary){
	switch(boundary){
		case HliMonthBoundary::SolarTerm:
			return "solar_term";
		case HliMonthBoundary::LunarFirstDay:
		default:
			return "lunar_first_day";
	}
}

std::string hli_leap_month_mode_key(HliLeapMonthMode mode){
	switch(mode){
		case HliLeapMonthMode::Ignore:
			return "ignore";
		case HliLeapMonthMode::SplitMidway:
			return "split_midway";
		case HliLeapMonthMode::ShiftToNext:
			return "shift_to_next";
		case HliLeapMonthMode::InheritPrevious:
		default:
			return "inherit_previous";
	}
}

std::string hli_day_boundary_key(HliDayBoundary boundary){
	switch(boundary){
		case HliDayBoundary::Hour0:
			return "hour0";
		case HliDayBoundary::Hour23:
		default:
			return "hour23";
	}
}

HliData calc_hli(EphRead&eph,LunCal6&lc,SolLunCal&solver,AppLon&app,
				 const HliInput&in){
	(void)eph;
	(void)solver;

	HliData out;
	GodSeq good;
	GodSeq bad;
	ActSeq yi;
	ActSeq ji;

	out.lon_deg=in.lon_deg;
	out.y_lun=mk_gz_from_year(in.lun_year);

	int y_lc=y_for_lchun(lc,in.jd_utc,in.gy);
	out.y_lchun=mk_gz_from_year(y_lc);

	out.rule_profile_code=in.rules.profile_code;
	out.year_boundary_code=in.rules.year_boundary;
	out.month_boundary_code=in.rules.month_boundary;
	out.leap_month_mode_code=in.rules.leap_month_mode;
	out.day_boundary_code=in.rules.day_boundary;

	HliYearBoundary year_boundary=
		static_cast<HliYearBoundary>(out.year_boundary_code);
	HliMonthBoundary month_boundary=
		static_cast<HliMonthBoundary>(out.month_boundary_code);
	HliLeapMonthMode leap_mode=
		static_cast<HliLeapMonthMode>(out.leap_month_mode_code);
	HliDayBoundary day_boundary=
		static_cast<HliDayBoundary>(out.day_boundary_code);
	const bool traditional_profile=
		out.rule_profile_code==static_cast<int>(HliProfileCode::Folk)||
		out.rule_profile_code==static_cast<int>(HliProfileCode::XieJi);

	out.y_rule=select_year_gz(out,year_boundary,lc,in.jd_utc,in.gy);

	int solar_mon_b=month_branch(lc,in.jd_utc,in.gy);
	int next_solar_num=-1;
	if(traditional_profile){
		out.y_rule=out.y_lun;
		next_solar_num=civil_term_cursor(
			lc,in.gy,in.gm,in.gd,in.tz_off);
		out.m_gz=month_gz_by_term_cursor(in.gy,next_solar_num,in.gm);
		solar_mon_b=out.m_gz.branch;
	}else if(month_boundary==HliMonthBoundary::SolarTerm){
		out.m_gz=month_gz_solar(out.y_rule,solar_mon_b);
	}else{
		out.m_gz=
			month_gz_lunar(out.y_rule,in.lun_month,in.lun_leap,in.lun_day,leap_mode);
	}
	out.god_month=out.m_gz;
	out.god_month_basis=
		month_boundary==HliMonthBoundary::SolarTerm
			?"solar_term_instant"
			:"lunar_month";
	if(traditional_profile&&
	   out.rule_profile_code==static_cast<int>(HliProfileCode::Folk)){
		out.god_month=month_gz_lunar(
			out.y_rule,in.lun_month,in.lun_leap,in.lun_day,
			HliLeapMonthMode::InheritPrevious);
		out.god_month_basis="lunar_month";
	}else if(traditional_profile){
		out.god_month_basis="solar_term_civil_day";
	}
	int mon_b=out.god_month.branch;
	int season_mon_b=traditional_profile?solar_mon_b:mon_b;
	int sn=((season_mon_b-2+12)%12)/3;

	int day_number=effective_day_number(in.jd_utc,in.tz_off,day_boundary);
	int day60=pos_mod(static_cast<long long>(day_number)-11,60);
	out.d_gz=mk_gz_from_idx(day60);

	double clock_min=
		static_cast<double>(in.hh)*60.0+static_cast<double>(in.mm)+in.ss/60.0;
	int hb=hour_branch_by_min(clock_min);
	int day_stem_for_h=out.d_gz.stem;
	if(day_boundary==HliDayBoundary::Hour23&&clock_min>=1380.0){
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
	if(day_boundary==HliDayBoundary::Hour23&&tst>=1380.0){
		day_stem_for_tst=(day_stem_for_tst+1)%10;
	}
	int hs_tst=(day_stem_for_tst*2+hb_tst)%10;
	out.h_gz_true.stem=hs_tst;
	out.h_gz_true.branch=hb_tst;
	out.h_gz_true.text=gz_text(hs_tst,hb_tst);
	out.bazi_clock=out.y_rule.text+" "+out.m_gz.text+" "+out.d_gz.text+" "+
				   out.h_gz.text;
	out.bazi_true=out.y_rule.text+" "+out.m_gz.text+" "+out.d_gz.text+" "+
				  out.h_gz_true.text;

	int jc_idx=pos_mod(static_cast<long long>(out.d_gz.branch)-mon_b,12);
	out.jianchu_code=jc_idx;

	int dstart=duty_start(mon_b);
	int dg_idx=pos_mod(static_cast<long long>(out.d_gz.branch)-dstart,12);
	out.duty_god_code=dg_idx;
	out.duty_is_yellow=
		(dg_idx==0||dg_idx==1||dg_idx==4||dg_idx==5||dg_idx==7||dg_idx==10);

	double jd_tdb=TimeScale::utc_to_tdb(in.jd_utc);
	double moon_lam=app.moon_calc(jd_tdb).first;
	out.xiu28_code=xiu_exact_code(in.moon_xg.region,moon_lam);
	out.xiu28_mod28_code=traditional_profile
		?xiu_cycle_by_civil_day(in.gy,in.gm,in.gd,in.tz_off)
		:xiu_mod28_code(moon_lam);
	out.xiu_id=in.moon_xg.star_name;

	if(traditional_profile){
		RuleDayContext ctx;
		ctx.lunar_month=in.lun_month;
		ctx.lunar_day=in.lun_day;
		ctx.year_stem=out.y_rule.stem;
		ctx.year_branch=out.y_rule.branch;
		ctx.solar_month_branch=solar_mon_b;
		ctx.god_month_branch=mon_b;
		ctx.season=sn;
		ctx.month_type=solar_mon_b%3;
		ctx.day_stem=out.d_gz.stem;
		ctx.day_branch=out.d_gz.branch;
		ctx.day60=day60;
		ctx.next_solar_num=next_solar_num;
		ctx.xiu28_cycle=out.xiu28_mod28_code;
		ctx.day_gz=out.d_gz.text;
		ctx.four_separations=
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z2")||
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z5")||
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z8")||
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z11");
		ctx.four_extinctions=
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J1")||
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J4")||
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J7")||
			is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J10");
		int earth_days=days_to_next_season_term(
			lc,in.gy,in.gm,in.gd,in.tz_off);
		ctx.earth_king=earth_days>=1&&earth_days<=18;

		evaluate_traditional_rules(
			ctx,out.jianchu_code,
			is_long_lunar_month(lc,in.gy,in.gm,in.gd,in.tz_off),
			good,bad,yi,ji,out);
		sync_codes(out,good,bad,yi,ji);
		materialize_zh(out,day60,hb);
		if(out.duty_god_code==4){
			out.duty_god="金贵";
		}
	}else{
	if(out.duty_is_yellow){
		god_add(good,static_cast<HliGodCode>(dg_idx));
	}else{
		god_add(bad,static_cast<HliGodCode>(dg_idx));
	}

	if(out.d_gz.branch==((mon_b+6)%12)){
		god_add(bad,HliGodCode::MonthBreak);
	}
	if(out.d_gz.branch==((out.y_rule.branch+6)%12)){
		god_add(bad,HliGodCode::YearBreak);
	}
	if(is_yang_gong(in.lun_month,in.lun_day)){
		god_add(bad,HliGodCode::YangGongTaboo);
	}
	if(is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z2")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z5")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z8")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"Z11")){
		god_add(bad,HliGodCode::FourSeparations);
	}
	if(is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J1")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J4")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J7")||
	   is_next_day_term(lc,in.gy,in.gm,in.gd,in.tz_off,"J10")){
		god_add(bad,HliGodCode::FourExtinctions);
	}

	static const std::array<int,12> kLiuHe={1,0,11,10,9,8,7,6,5,4,3,2};
	if(pos_mod(static_cast<long long>(out.d_gz.branch)-mon_b,4)==0){
		god_add(good,HliGodCode::ThreeHarmony);
	}
	if(out.d_gz.branch==kLiuHe[static_cast<std::size_t>(mon_b)]){
		god_add(good,HliGodCode::SixHarmony);
	}

	static const std::array<int,12> kMonDeStem={8,6,2,0,8,6,2,0,8,6,2,0};
	static const std::array<int,12> kMonDeHeStem={3,1,7,5,3,1,7,5,3,1,7,5};
	if(out.d_gz.stem==kMonDeStem[static_cast<std::size_t>(mon_b)]){
		god_add(good,HliGodCode::MonthVirtue);
	}
	if(out.d_gz.stem==kMonDeHeStem[static_cast<std::size_t>(mon_b)]){
		god_add(good,HliGodCode::MonthVirtueCombine);
	}

	static const std::array<int,12> kTdeStem={-1,6,3,-1,8,7,-1,0,9,-1,2,1};
	static const std::array<std::array<int,2>,12> kTdeBranch={{
		std::array<int,2>{5,4},std::array<int,2>{-1,-1},std::array<int,2>{-1,-1},std::array<int,2>{8,7},
		std::array<int,2>{-1,-1},std::array<int,2>{-1,-1},std::array<int,2>{11,10},std::array<int,2>{-1,-1},
		std::array<int,2>{-1,-1},std::array<int,2>{2,1},std::array<int,2>{-1,-1},std::array<int,2>{-1,-1}
	}};
	bool is_tde=false;
	if(mon_b%3==0){
		const auto&tg=kTdeBranch[static_cast<std::size_t>(mon_b)];
		is_tde=(out.d_gz.branch==tg[0]||out.d_gz.branch==tg[1]);
	}else{
		is_tde=(out.d_gz.stem==kTdeStem[static_cast<std::size_t>(mon_b)]);
	}
	if(is_tde){
		god_add(good,HliGodCode::HeavenlyVirtue);
	}
	static const std::array<int,12> kTdeHeStem={-1,1,8,-1,3,2,-1,5,4,-1,7,6};
	if(kTdeHeStem[static_cast<std::size_t>(mon_b)]>=0&&
	   out.d_gz.stem==kTdeHeStem[static_cast<std::size_t>(mon_b)]){
		god_add(good,HliGodCode::HeavenlyVirtueCombine);
	}

	static const std::array<int,10> kYearDeStem={0,6,2,8,4,0,6,2,8,4};
	static const std::array<int,10> kYearDeHeStem={5,1,7,3,9,5,1,7,3,9};
	if(out.d_gz.stem==kYearDeStem[static_cast<std::size_t>(out.y_rule.stem)]){
		god_add(good,HliGodCode::YearlyVirtue);
	}
	if(out.d_gz.stem==
	   kYearDeHeStem[static_cast<std::size_t>(out.y_rule.stem)]){
		god_add(good,HliGodCode::YearlyVirtueCombine);
	}

	static const std::array<int,4> kWang={2,5,8,11};
	static const std::array<int,4> kGuan={3,6,9,0};
	static const std::array<int,4> kShou={9,0,3,6};
	static const std::array<int,4> kXiang={5,8,11,2};
	static const std::array<int,4> kMin={6,9,0,3};
	static const std::array<int,4> kShiDe={6,4,0,2};
	if(out.d_gz.branch==kWang[static_cast<std::size_t>(sn)]){
		god_add(good,HliGodCode::RoyalDay);
	}
	if(out.d_gz.branch==kGuan[static_cast<std::size_t>(sn)]){
		god_add(good,HliGodCode::OfficialDay);
	}
	if(out.d_gz.branch==kShou[static_cast<std::size_t>(sn)]){
		god_add(good,HliGodCode::GuardDay);
	}
	if(out.d_gz.branch==kXiang[static_cast<std::size_t>(sn)]){
		god_add(good,HliGodCode::MutualDay);
	}
	if(out.d_gz.branch==kMin[static_cast<std::size_t>(sn)]){
		god_add(good,HliGodCode::PeopleDay);
	}
	if(out.d_gz.branch==kShiDe[static_cast<std::size_t>(sn)]){
		god_add(good,HliGodCode::TimeVirtue);
	}

	static const std::array<std::array<int,2>,4> kTianGuiStem={{
		std::array<int,2>{0,1},std::array<int,2>{2,3},std::array<int,2>{6,7},
		std::array<int,2>{8,9}
	}};
	const auto&tgs=kTianGuiStem[static_cast<std::size_t>(sn)];
	if(out.d_gz.stem==tgs[0]||out.d_gz.stem==tgs[1]){
		god_add(good,HliGodCode::HeavenlyNoble);
	}

	static const std::array<int,12> kTianXi={
		8,9,10,11,0,1,2,3,4,5,6,7
	};
	if(out.d_gz.branch==kTianXi[static_cast<std::size_t>(mon_b)]){
		god_add(good,HliGodCode::HeavenlyJoy);
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
		god_add(good,HliGodCode::MonthlyGrace);
	}

	static const std::array<std::pair<int,int>,4> kTianSheGz={
		std::pair<int,int>{4,2},std::pair<int,int>{0,6},
		std::pair<int,int>{4,8},std::pair<int,int>{0,0}
	};
	const auto&ts_gz=kTianSheGz[static_cast<std::size_t>(sn)];
	if(out.d_gz.stem==ts_gz.first&&out.d_gz.branch==ts_gz.second){
		god_add(good,HliGodCode::HeavenlyPardon);
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
		god_add(bad,HliGodCode::GreatLoss);
	}
	if(out.d_gz.branch==kWuGui[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::FiveGhosts);
	}
	if(out.d_gz.branch==kXianChi[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::PeachBlossom);
	}
	if(out.d_gz.branch==kXueZhi[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::BloodOmen);
	}
	if(out.d_gz.branch==kTianGou[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::HeavenlyDog);
	}
	if(out.d_gz.branch==kWangWang[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::GoingLoss);
	}
	if(out.d_gz.branch==kSiJi[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::FourStrikes);
	}
	if(out.d_gz.branch==kYueXing[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::MonthlyPunishment);
	}
	if(out.d_gz.branch==kYueHai[static_cast<std::size_t>(mon_b)]){
		god_add(bad,HliGodCode::MonthlyHarm);
	}

	apply_base_act(static_cast<HliJianchuCode>(out.jianchu_code),yi,ji);
	add_stem_branch_acts(out.d_gz.stem,out.d_gz.branch,yi,ji);

	if(in.lun_day==1||in.lun_day==6||in.lun_day==15||in.lun_day==19||
	   in.lun_day==21||in.lun_day==23){
		act_add(ji,HliActCode::TrimNails);
	}
	if(in.lun_day==12||in.lun_day==15){
		act_add_many(ji,{HliActCode::CosmeticGrooming,HliActCode::Haircut});
	}
	if(in.lun_day==15||!in.phase_name.empty()){
		act_add(ji,HliActCode::SeekMedical);
	}
	if(god_has(good,HliGodCode::MonthVirtue)||
	   god_has(good,HliGodCode::MonthVirtueCombine)||
	   god_has(good,HliGodCode::HeavenlyVirtue)||
	   god_has(good,HliGodCode::HeavenlyVirtueCombine)){
		act_add_many(yi,{HliActCode::Sacrifice,HliActCode::PrayBlessing,
						 HliActCode::PrayOffspring,HliActCode::Banquet,
						 HliActCode::ArrangeMarriage,HliActCode::Wedding,
						 HliActCode::Travel,HliActCode::TakeOffice,
						 HliActCode::HandleGovernance,HliActCode::Construction,
						 HliActCode::OpenMarket,HliActCode::ReceiveWealth});
	}
	if(god_has(good,HliGodCode::YearlyVirtue)||
	   god_has(good,HliGodCode::YearlyVirtueCombine)){
		act_add_many(yi,{HliActCode::Construction,HliActCode::Wedding,
						 HliActCode::Betrothal,HliActCode::MoveResidence});
	}
	if(god_has(good,HliGodCode::HeavenlyPardon)){
		act_add_many(yi,{HliActCode::Sacrifice,HliActCode::PrayBlessing,
						 HliActCode::PrayOffspring,HliActCode::TakeOffice,
						 HliActCode::HandleGovernance,HliActCode::Wedding,
						 HliActCode::MoveResidence,HliActCode::Construction,
						 HliActCode::OpenMarket,HliActCode::ReceiveWealth,
						 HliActCode::Burial});
	}
	if(god_has(good,HliGodCode::MonthlyGrace)){
		act_add_many(yi,{HliActCode::Sacrifice,HliActCode::Banquet,
						 HliActCode::Travel,HliActCode::ArrangeMarriage,
						 HliActCode::Wedding});
	}
	if(god_has(good,HliGodCode::HeavenlyJoy)){
		act_add_many(yi,{HliActCode::Banquet,HliActCode::ArrangeMarriage,
						 HliActCode::Wedding,HliActCode::Betrothal});
	}
	if(god_has(good,HliGodCode::RoyalDay)||
	   god_has(good,HliGodCode::OfficialDay)||
	   god_has(good,HliGodCode::GuardDay)||
	   god_has(good,HliGodCode::MutualDay)||
	   god_has(good,HliGodCode::PeopleDay)||
	   god_has(good,HliGodCode::TimeVirtue)){
		act_add_many(yi,{HliActCode::TakeOffice,HliActCode::HandleGovernance,
						 HliActCode::BestowFavor,HliActCode::Banquet,
						 HliActCode::ReceiveWealth,HliActCode::OpenMarket});
	}
	if(god_has(bad,HliGodCode::MonthBreak)){
		act_add_many(ji,{HliActCode::Wedding,HliActCode::MoveResidence,
						 HliActCode::OpenMarket,HliActCode::SignContractTrade,
						 HliActCode::ReceiveWealth,HliActCode::BreakGround,
						 HliActCode::Burial});
	}
	if(god_has(bad,HliGodCode::YearBreak)){
		act_add_many(ji,{HliActCode::Construction,HliActCode::MoveResidence,
						 HliActCode::Wedding,HliActCode::OpenMarket,
						 HliActCode::Burial});
	}
	if(god_has(bad,HliGodCode::YangGongTaboo)){
		act_add_many(ji,{HliActCode::OpenMarket,HliActCode::Construction,
						 HliActCode::Wedding,HliActCode::SignContractTrade});
	}
	if(god_has(bad,HliGodCode::GreatLoss)){
		act_add_many(ji,{HliActCode::RepairStorehouse,HliActCode::OpenMarket,
						 HliActCode::SignContractTrade,HliActCode::ReceiveWealth,
						 HliActCode::OpenGranary});
	}
	if(god_has(bad,HliGodCode::FiveGhosts)){
		act_add(ji,HliActCode::Travel);
	}
	if(god_has(bad,HliGodCode::PeachBlossom)){
		act_add_many(ji,{HliActCode::Wedding,HliActCode::Fishing,
						 HliActCode::BoatCrossing});
	}
	if(god_has(bad,HliGodCode::BloodOmen)){
		act_add(ji,HliActCode::Needling);
	}
	if(god_has(bad,HliGodCode::HeavenlyDog)){
		act_add(ji,HliActCode::Sacrifice);
	}
	if(god_has(bad,HliGodCode::GoingLoss)){
		act_add_many(ji,{HliActCode::Travel,HliActCode::TakeOffice,
						 HliActCode::Wedding,HliActCode::MoveResidence,
						 HliActCode::SeekMedical});
	}
	if(god_has(bad,HliGodCode::FourStrikes)){
		act_add_many(ji,{HliActCode::PacifyBorder,HliActCode::SelectGenerals,
						 HliActCode::DeployTroops});
	}
	if(god_has(bad,HliGodCode::MonthlyPunishment)){
		act_add_many(ji,{HliActCode::Travel,HliActCode::Wedding,
						 HliActCode::Construction,HliActCode::OpenMarket,
						 HliActCode::SignContractTrade,
						 HliActCode::ReceiveWealth});
	}
	if(god_has(bad,HliGodCode::MonthlyHarm)){
		act_add_many(ji,{HliActCode::Wedding,HliActCode::OpenMarket,
						 HliActCode::SignContractTrade,HliActCode::ReceiveWealth,
						 HliActCode::Burial});
	}

	if(out.d_gz.branch==3){
		act_add(ji,HliActCode::DigWell);
		act_remove(yi,HliActCode::OpenCanal);
	}
	if(out.d_gz.stem==8){
		act_add(ji,HliActCode::OpenCanal);
		act_remove(yi,HliActCode::DigWell);
	}
	if(out.d_gz.branch==5){
		act_add(ji,HliActCode::Travel);
		act_remove(yi,HliActCode::DeployTroops);
		act_remove(yi,HliActCode::Travel);
	}
	if(out.d_gz.branch==9){
		act_add(ji,HliActCode::Banquet);
		act_remove(yi,HliActCode::CelebrationGrant);
		act_remove(yi,HliActCode::Banquet);
	}
	if(out.d_gz.stem==3){
		act_add(ji,HliActCode::Haircut);
		act_remove(yi,HliActCode::CosmeticGrooming);
	}
	if(out.d_gz.branch==11){
		act_add(ji,HliActCode::Wedding);
	}
	if(god_has(bad,HliGodCode::HeavenlyDog)||out.d_gz.branch==2){
		act_add(ji,HliActCode::Sacrifice);
		act_remove(yi,HliActCode::Sacrifice);
		act_remove(yi,HliActCode::PrayBlessing);
		act_remove(yi,HliActCode::PrayOffspring);
	}
	if(god_has(bad,HliGodCode::FourSeparations)||
	   god_has(bad,HliGodCode::FourExtinctions)){
		yi=ActSeq{};
		ji=ActSeq{};
		act_add(yi,HliActCode::AvoidAll);
		act_add(ji,HliActCode::AvoidAll);
	}

	int score=out.duty_is_yellow?1:-1;
	for(int code : good.order){
		score+=good_score(static_cast<HliGodCode>(code));
	}
	for(int code : bad.order){
		score+=bad_score(static_cast<HliGodCode>(code));
	}

	if(score>=2){
		out.yi_ji_level=0;
		out.yi_ji_rule_code=static_cast<int>(HliRuleCode::FollowYiIgnoreJi);
	}else if(score>=0){
		out.yi_ji_level=1;
		out.yi_ji_rule_code=static_cast<int>(HliRuleCode::FollowBoth);
	}else if(score>=-2){
		out.yi_ji_level=2;
		out.yi_ji_rule_code=static_cast<int>(HliRuleCode::FollowJiIgnoreYi);
	}else{
		out.yi_ji_level=3;
		out.yi_ji_rule_code=static_cast<int>(HliRuleCode::AvoidEverything);
	}

	if(out.yi_ji_level==3){
		yi=ActSeq{};
		ji=ActSeq{};
		act_add(yi,HliActCode::AvoidAll);
		act_add(ji,HliActCode::AvoidAll);
	}else{
		std::vector<int> inter;
		for(int code : yi.order){
			if(act_has(ji,static_cast<HliActCode>(code))){
				inter.push_back(code);
			}
		}
		if(out.yi_ji_level==0){
			for(int code : inter){
				act_remove(ji,static_cast<HliActCode>(code));
			}
		}else if(out.yi_ji_level==1){
			for(int code : inter){
				act_remove(yi,static_cast<HliActCode>(code));
				act_remove(ji,static_cast<HliActCode>(code));
			}
		}else{
			for(int code : inter){
				act_remove(yi,static_cast<HliActCode>(code));
			}
		}
		if(yi.order.empty()){
			act_add(yi,HliActCode::AvoidAll);
		}
		if(ji.order.empty()){
			act_add(ji,HliActCode::NoMajorTaboo);
		}
	}

	sort_act_codes(yi.order);
	sort_act_codes(ji.order);
	sync_codes(out,good,bad,yi,ji);
	materialize_zh(out,day60,hb);
	}

	int day_slot_start=(day60*12)%60;
	int mask_today=kHourMask[static_cast<std::size_t>(day60)];
	int mask_next=kHourMask[static_cast<std::size_t>((day60+1)%60)];
	static const std::array<const char*,12> kHourSpan={{
		"子(23:00-00:59)","丑(01:00-02:59)","寅(03:00-04:59)","卯(05:00-06:59)",
		"辰(07:00-08:59)","巳(09:00-10:59)","午(11:00-12:59)","未(13:00-14:59)",
		"申(15:00-16:59)","酉(17:00-18:59)","戌(19:00-20:59)","亥(21:00-22:59)"
	}};
	out.hour_jx.reserve(13);
	for(int i=0;i<13;++i){
		int idx60=(day_slot_start+i)%60;
		bool is_bad=false;
		std::string slot_name;
		if(i<12){
			is_bad=bad_slot(mask_today,i);
			slot_name=kHourSpan[static_cast<std::size_t>(i)];
		}else{
			is_bad=bad_slot(mask_next,0);
			slot_name="子(次日23:00-00:59)";
		}
		HliHour rec;
		rec.slot_index=i;
		rec.gz_index=idx60;
		rec.is_bad=is_bad;
		rec.slot=slot_name;
		rec.gz=mk_gz_from_idx(idx60).text;
		rec.luck=is_bad?"凶":"吉";
		out.hour_jx.push_back(std::move(rec));
	}

	return out;
}
