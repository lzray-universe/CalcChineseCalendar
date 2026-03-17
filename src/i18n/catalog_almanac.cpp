#include "lunar/i18n.hpp"

#include<array>
#include<cstddef>
#include<stdexcept>
#include<string>
#include<utility>
#include<vector>

#include "lunar/almanac.hpp"

namespace lunar::i18n{

namespace{

struct Text4{
	const char*zh;
	const char*en;
	const char*ja;
	const char*ko;
};

struct DictItem{
	const char*key;
	Text4 text;
};

std::string pick_text(const Text4&text){
	return pick(text.zh,text.en,text.ja,text.ko);
}

template<std::size_t N>
std::string pick_index(const std::array<Text4,N>&items,int idx,
					  const std::string&fallback){
	if(idx<0||idx>=static_cast<int>(N)){
		return fallback;
	}
	return pick_text(items[static_cast<std::size_t>(idx)]);
}

const std::array<Text4,10> kStem={{
	{"甲","Jia","甲","갑"},
	{"乙","Yi","乙","을"},
	{"丙","Bing","丙","병"},
	{"丁","Ding","丁","정"},
	{"戊","Wu","戊","무"},
	{"己","Ji","己","기"},
	{"庚","Geng","庚","경"},
	{"辛","Xin","辛","신"},
	{"壬","Ren","壬","임"},
	{"癸","Gui","癸","계"},
}};

const std::array<Text4,12> kBranch={{
	{"子","Zi","子","자"},
	{"丑","Chou","丑","축"},
	{"寅","Yin","寅","인"},
	{"卯","Mao","卯","묘"},
	{"辰","Chen","辰","진"},
	{"巳","Si","巳","사"},
	{"午","Wu","午","오"},
	{"未","Wei","未","미"},
	{"申","Shen","申","신"},
	{"酉","You","酉","유"},
	{"戌","Xu","戌","술"},
	{"亥","Hai","亥","해"},
}};

const std::array<Text4,12> kZodiac={{
	{"鼠","Rat","鼠","쥐"},
	{"牛","Ox","牛","소"},
	{"虎","Tiger","虎","호랑이"},
	{"兔","Rabbit","兎","토끼"},
	{"龙","Dragon","龍","용"},
	{"蛇","Snake","蛇","뱀"},
	{"马","Horse","馬","말"},
	{"羊","Goat","羊","양"},
	{"猴","Monkey","猿","원숭이"},
	{"鸡","Rooster","鶏","닭"},
	{"狗","Dog","犬","개"},
	{"猪","Pig","猪","돼지"},
}};

const std::array<Text4,5> kWuxing={{
	{"木","Wood","木","목"},
	{"火","Fire","火","화"},
	{"土","Earth","土","토"},
	{"金","Metal","金","금"},
	{"水","Water","水","수"},
}};

const std::array<Text4,30> kNayin={{
	{"海中金","Gold in the Sea","海中金","해중금"},
	{"炉中火","Fire in the Furnace","炉中火","노중화"},
	{"大林木","Wood of the Great Forest","大林木","대림목"},
	{"路旁土","Roadside Earth","路旁土","노방토"},
	{"剑锋金","Sword-Edge Gold","剣鋒金","검봉금"},
	{"山头火","Mountain-Top Fire","山頭火","산두화"},
	{"涧下水","Ravine Water","澗下水","간하수"},
	{"城头土","City-Wall Earth","城頭土","성두토"},
	{"白蜡金","White Wax Gold","白蝋金","백랍금"},
	{"杨柳木","Willow Wood","楊柳木","양류목"},
	{"井泉水","Well-Spring Water","井泉水","정천수"},
	{"屋上土","Roof-Top Earth","屋上土","옥상토"},
	{"霹雳火","Thunderbolt Fire","霹靂火","벽력화"},
	{"松柏木","Pine-Cypress Wood","松柏木","송백목"},
	{"长流水","Long-Flowing Water","長流水","장류수"},
	{"砂中金","Gold in Sand","砂中金","사중금"},
	{"山下火","Mountain-Foot Fire","山下火","산하화"},
	{"平地木","Flatland Wood","平地木","평지목"},
	{"壁上土","Wall Earth","壁上土","벽상토"},
	{"金箔金","Foil Gold","金箔金","금박금"},
	{"覆灯火","Covered-Lamp Fire","覆灯火","복등화"},
	{"天河水","Milky-Way Water","天河水","천하수"},
	{"大驿土","Post-Road Earth","大駅土","대역토"},
	{"钗钏金","Hairpin Gold","釵釧金","차천금"},
	{"桑柘木","Mulberry Wood","桑柘木","상자목"},
	{"大溪水","Great Creek Water","大渓水","대계수"},
	{"砂中土","Earth in Sand","砂中土","사중토"},
	{"天上火","Sky Fire","天上火","천상화"},
	{"石榴木","Pomegranate Wood","石榴木","석류목"},
	{"大海水","Great Sea Water","大海水","대해수"},
}};

const std::array<Text4,10> kPengStem={{
	{"甲不开仓 财物耗散","Jia: avoid opening granaries; wealth may scatter.","甲は倉を開かず、財物耗散。","갑일에는 창고를 열지 말라, 재물이 흩어진다."},
	{"乙不栽植 千株不长","Yi: avoid planting; growth is hindered.","乙は植栽せず、千株育たず。","을일에는 심기를 피하라, 잘 자라지 않는다."},
	{"丙不修灶 必见灾殃","Bing: avoid repairing stoves; misfortune may follow.","丙は竈を修めず、災いを招く。","병일에는 부엌 수리를 피하라."},
	{"丁不剃头 头必生疮","Ding: avoid haircuts; scalp trouble may follow.","丁は剃髪せず、頭に瘡を生ず。","정일에는 이발을 피하라."},
	{"戊不受田 田主不祥","Wu: avoid taking fields; land luck declines.","戊は田を受けず、田主不祥。","무일에는 토지 인수를 피하라."},
	{"己不破券 二比并亡","Ji: avoid contract canceling; both parties may lose.","己は券を破らず、双方に損あり。","기일에는 계약 파기를 피하라."},
	{"庚不经络 织机虚张","Geng: avoid loom setup; effort may be wasted.","庚は経絡を作らず、機織り空張り。","경일에는 경락·직조 일을 피하라."},
	{"辛不合酱 主人不尝","Xin: avoid sauce making; host may not taste it.","辛は醤を合わせず、主人味わわず。","신일에는 장 담그기를 피하라."},
	{"壬不泱水 更难提防","Ren: avoid large waterworks; hard to guard.","壬は大水を扱わず、防ぎ難し。","임일에는 큰 물일을 피하라."},
	{"癸不词讼 理弱敌强","Gui: avoid lawsuits; your side may be weaker.","癸は訴訟せず、理弱く敵強し。","계일에는 소송을 피하라."},
}};

const std::array<Text4,12> kPengBranch={{
	{"子不问卜 自惹祸殃","Zi: avoid divination; self-invited trouble.","子は占わず、自ら禍を招く。","자일에는 점을 피하라."},
	{"丑不冠带 主不还乡","Chou: avoid crowning/attire rites; return is delayed.","丑は冠帯せず、主は還郷し難し。","축일에는 관대 예식을 피하라."},
	{"寅不祭祀 神鬼不尝","Yin: avoid sacrifice rites; offerings are not accepted.","寅は祭祀せず、神鬼味わわず。","인일에는 제사를 피하라."},
	{"卯不穿井 水泉不香","Mao: avoid digging wells; water quality suffers.","卯は井を穿たず、水泉香らず。","묘일에는 우물 파기를 피하라."},
	{"辰不哭泣 必主重丧","Chen: avoid wailing; mourning omen is heavy.","辰は哭泣せず、重喪の兆し。","진일에는 곡을 삼가라."},
	{"巳不远行 财物伏藏","Si: avoid long trips; wealth may be hidden/lost.","巳は遠行せず、財物伏蔵。","사일에는 원행을 피하라."},
	{"午不苫盖 屋主更张","Wu: avoid roofing; household may face upheaval.","午は苫蓋せず、屋主更張。","오일에는 지붕 덮기를 피하라."},
	{"未不服药 毒气入肠","Wei: avoid taking medicine; adverse effects likely.","未は服薬せず、毒気入腸。","미일에는 복약을 삼가라."},
	{"申不安床 鬼祟入房","Shen: avoid bed placement; uneasy influences enter.","申は安床せず、鬼祟入房。","신일에는 침상 설치를 피하라."},
	{"酉不会客 醉坐颠狂","You: avoid receiving guests; disorder may follow.","酉は会客せず、酔って顛狂。","유일에는 손님맞이를 피하라."},
	{"戌不吃犬 作怪上床","Xu: avoid eating dog meat; strange unrest follows.","戌は犬を食さず、怪異が起こる。","술일에는 개고기를 피하라."},
	{"亥不嫁娶 不利新郎","Hai: avoid weddings; unfavorable for the groom.","亥は嫁娶せず、新郎に不利。","해일에는 혼례를 피하라."},
}};

const std::array<int,10> kStemWxIdx={0,0,1,1,2,2,3,3,4,4};
const std::array<int,12> kBranchWxIdx={4,2,0,0,2,1,1,2,3,3,2,4};
const std::array<int,12> kLiuHe={1,0,11,10,9,8,7,6,5,4,3,2};

const std::array<DictItem,184> kDict={{
	{"建",{"建","Build","建","건"}},
	{"除",{"除","Remove","除","제"}},
	{"满",{"满","Full","満","만"}},
	{"平",{"平","Balance","平","평"}},
	{"定",{"定","Fix","定","정"}},
	{"执",{"执","Hold","執","집"}},
	{"破",{"破","Break","破","파"}},
	{"危",{"危","Danger","危","위"}},
	{"成",{"成","Success","成","성"}},
	{"收",{"收","Receive","収","수"}},
	{"开",{"开","Open","開","개"}},
	{"闭",{"闭","Close","閉","폐"}},
	{"青龙",{"青龙","Azure Dragon","青龍","청룡"}},
	{"明堂",{"明堂","Bright Hall","明堂","명당"}},
	{"天刑",{"天刑","Heavenly Punishment","天刑","천형"}},
	{"朱雀",{"朱雀","Vermilion Bird","朱雀","주작"}},
	{"金匮",{"金匮","Golden Coffer","金匱","금궤"}},
	{"天德",{"天德","Heavenly Virtue","天徳","천덕"}},
	{"白虎",{"白虎","White Tiger","白虎","백호"}},
	{"玉堂",{"玉堂","Jade Hall","玉堂","옥당"}},
	{"天牢",{"天牢","Heavenly Prison","天牢","천뢰"}},
	{"玄武",{"玄武","Black Tortoise","玄武","현무"}},
	{"司命",{"司命","Controller of Fate","司命","사명"}},
	{"勾陈",{"勾陈","Gouchen","勾陳","구진"}},
	{"黄道日",{"黄道日","Auspicious Day","黄道日","황도일"}},
	{"黑道日",{"黑道日","Inauspicious Day","黒道日","흑도일"}},
	{"从宜不从忌",{"从宜不从忌","Follow Yi, ignore Ji","宜を優先し忌を従わず","의를 따르고 기를 따르지 않음"}},
	{"从宜亦从忌",{"从宜亦从忌","Observe both Yi and Ji","宜も忌も参照","의와 기를 함께 따름"}},
	{"从忌不从宜",{"从忌不从宜","Follow Ji, ignore Yi","忌を優先し宜を従わず","기를 따르고 의를 따르지 않음"}},
	{"诸事皆忌",{"诸事皆忌","Avoid all activities","諸事とも忌","모든 일을 피함"}},
	{"诸事不宜",{"诸事不宜","Avoid all activities","諸事不宜","모든 일이 부적합"}},
	{"诸事不忌",{"诸事不忌","No major taboos","諸事不忌","큰 금기 없음"}},
	{"月破",{"月破","Month Break","月破","월파"}},
	{"岁破",{"岁破","Year Break","歳破","세파"}},
	{"杨公忌",{"杨公忌","Yanggong Taboo","楊公忌","양공기"}},
	{"四离",{"四离","Four Separations","四離","사리"}},
	{"四绝",{"四绝","Four Extinctions","四絶","사절"}},
	{"三合",{"三合","Three Harmonies","三合","삼합"}},
	{"六合",{"六合","Six Harmony","六合","육합"}},
	{"月德",{"月德","Monthly Virtue","月徳","월덕"}},
	{"月德合",{"月德合","Monthly Virtue Combine","月徳合","월덕합"}},
	{"天德合",{"天德合","Heavenly Virtue Combine","天徳合","천덕합"}},
	{"岁德",{"岁德","Yearly Virtue","歳徳","세덕"}},
	{"岁德合",{"岁德合","Yearly Virtue Combine","歳徳合","세덕합"}},
	{"王日",{"王日","Royal Day","王日","왕일"}},
	{"官日",{"官日","Official Day","官日","관일"}},
	{"守日",{"守日","Guard Day","守日","수일"}},
	{"相日",{"相日","Mutual Day","相日","상일"}},
	{"民日",{"民日","People Day","民日","민일"}},
	{"时德",{"时德","Time Virtue","時徳","시덕"}},
	{"天贵",{"天贵","Heavenly Noble","天貴","천귀"}},
	{"天喜",{"天喜","Heavenly Joy","天喜","천희"}},
	{"月恩",{"月恩","Monthly Grace","月恩","월은"}},
	{"天赦",{"天赦","Heavenly Pardon","天赦","천사"}},
	{"大耗",{"大耗","Great Loss","大耗","대모"}},
	{"五鬼",{"五鬼","Five Ghosts","五鬼","오귀"}},
	{"咸池",{"咸池","Peach Blossom","咸池","함지"}},
	{"血支",{"血支","Blood Omen","血支","혈지"}},
	{"天狗",{"天狗","Heavenly Dog","天狗","천구"}},
	{"往亡",{"往亡","Going-Loss","往亡","왕망"}},
	{"四击",{"四击","Four Strikes","四撃","사격"}},
	{"月刑",{"月刑","Monthly Punishment","月刑","월형"}},
	{"月害",{"月害","Monthly Harm","月害","월해"}},
	{"祭祀",{"祭祀","Sacrifice","祭祀","제사"}},
	{"出行",{"出行","Travel","出行","출행"}},
	{"移徙",{"移徙","Relocation","移徙","이사"}},
	{"结婚姻",{"结婚姻","Arrange Marriage","婚姻を結ぶ","혼인 성사"}},
	{"宴会",{"宴会","Banquet","宴会","연회"}},
	{"嫁娶",{"嫁娶","Wedding","嫁娶","혼례"}},
	{"安床",{"安床","Set Bed","安床","안상"}},
	{"沐浴",{"沐浴","Bathing","沐浴","목욕"}},
	{"剃头",{"剃头","Haircut","剃頭","이발"}},
	{"修造",{"修造","Construction","修造","수조"}},
	{"求医疗病",{"求医疗病","Seek Medical Treatment","治療を求む","치료 구함"}},
	{"上表章",{"上表章","Submit Memorial","上表章","상표장"}},
	{"上官",{"上官","Take Office","上官","출사"}},
	{"入学",{"入学","Enter School","入学","입학"}},
	{"冠带",{"冠带","Crowning Rite","冠帯","관대"}},
	{"进人口",{"进人口","Add Family Members","人口を増やす","식구 늘림"}},
	{"裁衣",{"裁衣","Cut Clothes","裁衣","의복 재단"}},
	{"竖柱上梁",{"竖柱上梁","Raise Pillars/Beam","柱梁を立てる","기둥·들보 세움"}},
	{"经络",{"经络","Loom/Thread Work","経絡","경락/직조"}},
	{"开市",{"开市","Open Market","開市","개시"}},
	{"立券交易",{"立券交易","Sign Contract Trade","契約取引","계약 거래"}},
	{"纳财",{"纳财","Receive Wealth","納財","재물 들임"}},
	{"修置产室",{"修置产室","Prepare Birth Room","産室を整える","산실 정비"}},
	{"开渠",{"开渠","Open Canal","開渠","개거"}},
	{"穿井",{"穿井","Dig Well","井を掘る","우물 파기"}},
	{"安碓硙",{"安碓硙","Set Millstone","碓硙を据える","방아 설치"}},
	{"扫舍宇",{"扫舍宇","Clean House","家屋清掃","집 청소"}},
	{"平治道涂",{"平治道涂","Level Roads","道を整える","도로 정비"}},
	{"破屋坏垣",{"破屋坏垣","Demolish House/Wall","屋壁を壊す","가옥·담 철거"}},
	{"伐木",{"伐木","Logging","伐木","벌목"}},
	{"捕捉",{"捕捉","Capture","捕捉","포획"}},
	{"畋猎",{"畋猎","Hunting","狩猟","사냥"}},
	{"栽种",{"栽种","Planting","栽種","재식"}},
	{"牧养",{"牧养","Herding","牧養","목양"}},
	{"破土",{"破土","Break Ground","破土","기공"}},
	{"安葬",{"安葬","Burial","安葬","안장"}},
	{"启攒",{"启攒","Open Tomb Cache","啓攢","계찬"}},
	{"施恩",{"施恩","Bestow Favor","恩を施す","은혜 베풂"}},
	{"招贤",{"招贤","Recruit Talent","賢者を招く","현자 초빙"}},
	{"举正直",{"举正直","Promote Uprightness","正直を挙す","정직한 이 천거"}},
	{"临政",{"临政","Handle Governance","政務に臨む","정무 처리"}},
	{"解除",{"解除","Remove/Relieve","解除","해제"}},
	{"整容",{"整容","Cosmetic Grooming","整容","정용"}},
	{"整手足甲",{"整手足甲","Trim Nails","手足の爪を整える","손발톱 정리"}},
	{"裁制",{"裁制","Tailoring","裁製","재제"}},
	{"开仓",{"开仓","Open Granary","開倉","개창"}},
	{"塞穴",{"塞穴","Seal Holes","穴を塞ぐ","구멍 메움"}},
	{"补垣",{"补垣","Repair Wall","垣を補う","담 보수"}},
	{"修饰垣墙",{"修饰垣墙","Repair/Decorate Walls","垣牆を修飾","담장 수식"}},
	{"祈福",{"祈福","Pray for Blessing","祈福","기복"}},
	{"求嗣",{"求嗣","Pray for Offspring","嗣を求む","자손 기원"}},
	{"纳采",{"纳采","Betrothal","納采","납채"}},
	{"搬移",{"搬移","Move Residence","搬移","반이"}},
	{"营建",{"营建","Build Project","営建","영건"}},
	{"筑堤防",{"筑堤防","Build Embankment","堤防を築く","제방 축조"}},
	{"安抚边境",{"安抚边境","Pacify Border","辺境を安撫","변경 안정"}},
	{"选将",{"选将","Select Generals","将を選ぶ","장수 선발"}},
	{"诉讼",{"诉讼","Litigation","訴訟","소송"}},
	{"纳畜",{"纳畜","Acquire Livestock","納畜","가축 들임"}},
	{"酝酿",{"酝酿","Brew Fermentation","醸造","양조"}},
	{"修仓库",{"修仓库","Repair Storehouse","倉庫修繕","창고 수리"}},
	{"取鱼",{"取鱼","Fishing","魚取り","어획"}},
	{"乘船渡水",{"乘船渡水","Boat Crossing","舟で渡る","배로 건넘"}},
	{"针刺",{"针刺","Needling","鍼刺","침자"}},
	{"出师",{"出师","Deploy Troops","出師","출사"}},
	{"庆赐",{"庆赐","Celebration/Grant","慶賜","경사 하사"}},
	{"行船",{"行船","Sailing","行船","행선"}},
	{"登高",{"登高","Climb Heights","登高","등고"}},
	{"苫盖",{"苫盖","Thatch Covering","苫蓋","초개"}},
	{"正北",{"正北","North","北","북"}},
	{"东北",{"东北","Northeast","北東","북동"}},
	{"正东",{"正东","East","東","동"}},
	{"东南",{"东南","Southeast","南東","남동"}},
	{"正南",{"正南","South","南","남"}},
	{"西南",{"西南","Southwest","南西","남서"}},
	{"正西",{"正西","West","西","서"}},
	{"西北",{"西北","Northwest","北西","북서"}},
	{"东",{"东","East","東","동"}},
	{"南",{"南","South","南","남"}},
	{"西",{"西","West","西","서"}},
	{"北",{"北","North","北","북"}},
	{"胆",{"胆","Gallbladder","胆","담"}},
	{"肝",{"肝","Liver","肝","간"}},
	{"肺",{"肺","Lung","肺","폐"}},
	{"大肠",{"大肠","Large Intestine","大腸","대장"}},
	{"胃",{"胃","Stomach","胃","위"}},
	{"脾",{"脾","Spleen","脾","비"}},
	{"心",{"心","Heart","心","심"}},
	{"小肠",{"小肠","Small Intestine","小腸","소장"}},
	{"膀胱",{"膀胱","Bladder","膀胱","방광"}},
	{"肾",{"肾","Kidney","腎","신장"}},
	{"心包",{"心包","Pericardium","心包","심포"}},
	{"三焦",{"三焦","Triple Burner","三焦","삼초"}},
	{"角木蛟",{"角木蛟","Jiao Wood Dragon","角木蛟","각목교"}},
	{"亢金龙",{"亢金龙","Kang Metal Dragon","亢金龍","항금룡"}},
	{"氐土貉",{"氐土貉","Di Earth Badger","氐土貉","저토학"}},
	{"房日兔",{"房日兔","Fang Sun Rabbit","房日兎","방일토"}},
	{"心月狐",{"心月狐","Xin Moon Fox","心月狐","심월호"}},
	{"尾火虎",{"尾火虎","Wei Fire Tiger","尾火虎","미화호"}},
	{"箕水豹",{"箕水豹","Ji Water Leopard","箕水豹","기수표"}},
	{"斗木獬",{"斗木獬","Dou Wood Xie","斗木獬","두목해"}},
	{"牛金牛",{"牛金牛","Niu Metal Ox","牛金牛","우금우"}},
	{"女土蝠",{"女土蝠","Nu Earth Bat","女土蝠","녀토복"}},
	{"虚日鼠",{"虚日鼠","Xu Sun Rat","虚日鼠","허일서"}},
	{"危月燕",{"危月燕","Wei Moon Swallow","危月燕","위월연"}},
	{"室火猪",{"室火猪","Shi Fire Pig","室火猪","실화저"}},
	{"壁水貐",{"壁水貐","Bi Water Beast","壁水貐","벽수유"}},
	{"奎木狼",{"奎木狼","Kui Wood Wolf","奎木狼","규목랑"}},
	{"娄金狗",{"娄金狗","Lou Metal Dog","婁金狗","루금구"}},
	{"胃土雉",{"胃土雉","Wei Earth Pheasant","胃土雉","위토치"}},
	{"昴日鸡",{"昴日鸡","Mao Sun Rooster","昴日鶏","묘일계"}},
	{"毕月乌",{"毕月乌","Bi Moon Crow","畢月烏","필월오"}},
	{"觜火猴",{"觜火猴","Zui Fire Monkey","觜火猴","자화후"}},
	{"参水猿",{"参水猿","Shen Water Ape","参水猿","삼수원"}},
	{"井木犴",{"井木犴","Jing Wood Beast","井木犴","정목한"}},
	{"鬼金羊",{"鬼金羊","Gui Metal Goat","鬼金羊","귀금양"}},
	{"柳土獐",{"柳土獐","Liu Earth Deer","柳土獐","류토장"}},
	{"星日马",{"星日马","Xing Sun Horse","星日馬","성일마"}},
	{"张月鹿",{"张月鹿","Zhang Moon Deer","張月鹿","장월록"}},
	{"翼火蛇",{"翼火蛇","Yi Fire Snake","翼火蛇","익화사"}},
	{"轸水蚓",{"轸水蚓","Zhen Water Worm","軫水蚓","진수인"}},
}};

std::string tr_dict(const std::string&key){
	for(const auto&item : kDict){
		if(key==item.key){
			return pick_text(item.text);
		}
	}
	return key;
}

void tr_vec(std::vector<std::string>&items){
	for(auto&item : items){
		item=tr_dict(item);
	}
}

std::string tr_stem(int stem){
	return pick_index(kStem,stem,std::to_string(stem));
}

std::string tr_branch(int branch){
	return pick_index(kBranch,branch,std::to_string(branch));
}

std::string tr_zodiac(int branch){
	return pick_index(kZodiac,branch,std::to_string(branch));
}

std::string tr_wuxing(int idx){
	return pick_index(kWuxing,idx,std::to_string(idx));
}

std::string tr_gz(int stem,int branch){
	return tr_stem(stem)+tr_branch(branch);
}

int gz_idx60(int stem,int branch){
	for(int i=0;i<60;++i){
		if(i%10==stem&&i%12==branch){
			return i;
		}
	}
	return 0;
}

std::string tr_nayin(int day60){
	int idx=day60/2;
	return pick_index(kNayin,idx,std::to_string(idx));
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

std::string tr_fetal_god(const std::string&text){
	if(current_lang()==Lang::ZhHant){
		return to_zh_hant(text);
	}
	if(current_lang()!=Lang::En){
		return text;
	}
	std::string out=text;
	static const std::array<std::pair<const char*,const char*>,29> kRepl={{
		{"占大门","Main Door"},
		{"占门碓","Door-Mortar"},
		{"占门床","Door-Bed"},
		{"占碓磨","Mortar"},
		{"占房床","Bed"},
		{"门鸡栖","Door-Roost"},
		{"碓磨","Mortar"},
		{"厨灶","Kitchen"},
		{"仓库","Storehouse"},
		{"房床","Bed"},
		{"房内","Indoor"},
		{"正北","North"},
		{"东北","Northeast"},
		{"正东","East"},
		{"东南","Southeast"},
		{"正南","South"},
		{"西南","Southwest"},
		{"正西","West"},
		{"西北","Northwest"},
		{"门","Door"},
		{"厕","Toilet"},
		{"炉","Furnace"},
		{"栖","Roost"},
		{"外","Outside"},
		{"北","North"},
		{"南","South"},
		{"东","East"},
		{"西","West"},
		{"中","Center"},
	}};
	for(const auto&it : kRepl){
		replace_all(out,it.first,it.second);
	}
	return out;
}

std::string tr_sha_dir_zh(int b){
	if(b==8||b==0||b==4){
		return "南";
	}
	if(b==2||b==6||b==10){
		return "北";
	}
	if(b==11||b==3||b==7){
		return "西";
	}
	return "东";
}

std::array<int,3> tr_three_he_group(int b){
	if(b==8||b==0||b==4){
		return {8,0,4};
	}
	if(b==11||b==3||b==7){
		return {11,3,7};
	}
	if(b==2||b==6||b==10){
		return {2,6,10};
	}
	return {5,9,1};
}

std::string tr_clash(int day_branch){
	int clash_b=(day_branch+6)%12;
	switch(current_lang()){
		case Lang::Zh:
			return tr_zodiac(day_branch)+"日冲"+tr_zodiac(clash_b);
		case Lang::ZhHant:
			return to_zh_hant(tr_zodiac(day_branch)+"日冲"+tr_zodiac(clash_b));
		case Lang::En:
			return tr_zodiac(day_branch)+" day clashes with "+tr_zodiac(clash_b);
		case Lang::Ja:
			return tr_zodiac(day_branch)+"日冲"+tr_zodiac(clash_b);
		case Lang::Ko:
			return tr_zodiac(day_branch)+"일 충 "+tr_zodiac(clash_b);
	}
	return tr_zodiac(day_branch)+"日冲"+tr_zodiac(clash_b);
}

std::string tr_chong_sha(int day_branch){
	int clash_b=(day_branch+6)%12;
	std::string sha_dir=tr_dict(tr_sha_dir_zh(day_branch));
	switch(current_lang()){
		case Lang::Zh:
			return std::string("冲")+tr_zodiac(clash_b)+"煞"+sha_dir;
		case Lang::ZhHant:
			return to_zh_hant(std::string("冲")+tr_zodiac(clash_b)+"煞"+sha_dir);
		case Lang::En:
			return "Clash "+tr_zodiac(clash_b)+", Sha "+sha_dir;
		case Lang::Ja:
			return std::string("冲")+tr_zodiac(clash_b)+"煞"+sha_dir;
		case Lang::Ko:
			return std::string("충")+tr_zodiac(clash_b)+" 살 "+sha_dir;
	}
	return std::string("冲")+tr_zodiac(clash_b)+"煞"+sha_dir;
}

std::string tr_six_he(int day_branch){
	int other=kLiuHe[static_cast<std::size_t>(day_branch)];
	switch(current_lang()){
		case Lang::Zh:
		case Lang::ZhHant:
		case Lang::Ja:
			return tr_branch(day_branch)+"合"+tr_branch(other);
		case Lang::En:
			return tr_branch(day_branch)+" combines with "+tr_branch(other);
		case Lang::Ko:
			return tr_branch(day_branch)+"합"+tr_branch(other);
	}
	return tr_branch(day_branch)+"合"+tr_branch(other);
}

std::string tr_three_he(int day_branch){
	auto g=tr_three_he_group(day_branch);
	switch(current_lang()){
		case Lang::Zh:
		case Lang::ZhHant:
		case Lang::Ja:
			return tr_branch(g[0])+tr_branch(g[1])+tr_branch(g[2]);
		case Lang::En:
			return tr_branch(g[0])+"-"+tr_branch(g[1])+"-"+tr_branch(g[2]);
		case Lang::Ko:
			return tr_branch(g[0])+tr_branch(g[1])+tr_branch(g[2]);
	}
	return tr_branch(g[0])+tr_branch(g[1])+tr_branch(g[2]);
}

std::string tr_pengzu(int stem,int branch){
	std::string stem_txt=pick_index(kPengStem,stem,std::to_string(stem));
	std::string branch_txt=pick_index(kPengBranch,branch,std::to_string(branch));
	if(current_lang()==Lang::En){
		return stem_txt+"; "+branch_txt;
	}
	return stem_txt+" "+branch_txt;
}

std::string tr_wx_day(const HliData&h,int day60){
	std::string stem_txt=tr_stem(h.d_gz.stem);
	std::string branch_txt=tr_branch(h.d_gz.branch);
	std::string stem_wx=tr_wuxing(kStemWxIdx[static_cast<std::size_t>(h.d_gz.stem)]);
	std::string branch_wx=
		tr_wuxing(kBranchWxIdx[static_cast<std::size_t>(h.d_gz.branch)]);
	std::string nayin=tr_nayin(day60);
	switch(current_lang()){
		case Lang::Zh:
			return "天干"+stem_txt+"属"+stem_wx+" 地支"+branch_txt+"属"+
				   branch_wx+" 纳音"+nayin;
		case Lang::ZhHant:
			return "天干"+stem_txt+"屬"+stem_wx+" 地支"+branch_txt+"屬"+
				   branch_wx+" 納音"+nayin;
		case Lang::En:
			return "Stem "+stem_txt+" is "+stem_wx+", Branch "+branch_txt+
				   " is "+branch_wx+", Nayin "+nayin;
		case Lang::Ja:
			return "天干"+stem_txt+"は"+stem_wx+"、地支"+branch_txt+"は"+
				   branch_wx+"、納音"+nayin;
		case Lang::Ko:
			return "천간 "+stem_txt+"은 "+stem_wx+", 지지 "+branch_txt+"는 "+
				   branch_wx+", 납음 "+nayin;
	}
	return "天干"+stem_txt+"属"+stem_wx+" 地支"+branch_txt+"属"+branch_wx+
		   " 纳音"+nayin;
}

std::string tr_gz_text(const std::string&text){
	for(int i=0;i<60;++i){
		int stem=i%10;
		int branch=i%12;
		std::string zh=std::string(kStem[static_cast<std::size_t>(stem)].zh)+
					   kBranch[static_cast<std::size_t>(branch)].zh;
		if(text==zh){
			return tr_gz(stem,branch);
		}
	}
	return text;
}

std::string tr_hour_luck(const std::string&text){
	if(text=="吉"){
		return pick("吉","Auspicious","吉","길");
	}
	if(text=="凶"){
		return pick("凶","Inauspicious","凶","흉");
	}
	if(current_lang()==Lang::ZhHant){
		return to_zh_hant(text);
	}
	return text;
}

std::string tr_hour_slot(std::size_t idx){
	static const std::array<const char*,12> kSpan={{
		"23:00-00:59","01:00-02:59","03:00-04:59","05:00-06:59",
		"07:00-08:59","09:00-10:59","11:00-12:59","13:00-14:59",
		"15:00-16:59","17:00-18:59","19:00-20:59","21:00-22:59",
	}};
	int b=static_cast<int>(idx%12);
	if(idx<12){
		switch(current_lang()){
			case Lang::Zh:
			case Lang::ZhHant:
			case Lang::Ja:
			case Lang::Ko:
				return tr_branch(b)+"("+kSpan[idx]+")";
			case Lang::En:
				return tr_branch(b)+"("+kSpan[idx]+")";
		}
		return tr_branch(b)+"("+kSpan[idx]+")";
	}
	switch(current_lang()){
		case Lang::Zh:
			return tr_branch(0)+"(次日23:00-00:59)";
		case Lang::ZhHant:
			return to_zh_hant(tr_branch(0)+"(次日23:00-00:59)");
		case Lang::En:
			return tr_branch(0)+"(next day 23:00-00:59)";
		case Lang::Ja:
			return tr_branch(0)+"(翌日23:00-00:59)";
		case Lang::Ko:
			return tr_branch(0)+"(익일23:00-00:59)";
	}
	return tr_branch(0)+"(次日23:00-00:59)";
}

}

void localize_hli(::HliData*data){
	if(data==nullptr){
		throw std::invalid_argument("localize_hli requires non-null data");
	}
	if(current_lang()==Lang::Zh){
		return;
	}

	auto tr_gz_node=[](GzNode&g){ g.text=tr_gz(g.stem,g.branch); };

	tr_gz_node(data->y_lun);
	tr_gz_node(data->y_lchun);
	tr_gz_node(data->m_gz);
	tr_gz_node(data->d_gz);
	tr_gz_node(data->h_gz);
	tr_gz_node(data->h_gz_true);

	data->bazi_clock=
		data->y_lchun.text+" "+data->m_gz.text+" "+data->d_gz.text+" "+
		data->h_gz.text;
	data->bazi_true=
		data->y_lchun.text+" "+data->m_gz.text+" "+data->d_gz.text+" "+
		data->h_gz_true.text;

	data->jianchu=tr_dict(data->jianchu);
	data->duty_god=tr_dict(data->duty_god);
	data->duty_tag=tr_dict(data->duty_tag);
	data->clash=tr_clash(data->d_gz.branch);
	data->chong_sha=tr_chong_sha(data->d_gz.branch);
	data->six_he=tr_six_he(data->d_gz.branch);
	data->three_he=tr_three_he(data->d_gz.branch);
	data->zodiac_day=tr_zodiac(data->d_gz.branch);

	int day60=gz_idx60(data->d_gz.stem,data->d_gz.branch);
	data->pengzu=tr_pengzu(data->d_gz.stem,data->d_gz.branch);
	data->nayin=tr_nayin(day60);
	data->wx_day=tr_wx_day(*data,day60);
	data->fetal_god=tr_fetal_god(data->fetal_god);

	data->meridian=tr_dict(data->meridian);
	data->lucky_dir=tr_dict(data->lucky_dir);
	data->wealth_dir=tr_dict(data->wealth_dir);
	data->mascot_dir=tr_dict(data->mascot_dir);
	data->sun_noble_dir=tr_dict(data->sun_noble_dir);
	data->moon_noble_dir=tr_dict(data->moon_noble_dir);
	data->xiu28=tr_dict(data->xiu28);

	tr_vec(data->good_gods);
	tr_vec(data->bad_gods);
	tr_vec(data->yi);
	tr_vec(data->ji);
	data->yi_ji_rule=tr_dict(data->yi_ji_rule);

	for(std::size_t i=0;i<data->hour_jx.size();++i){
		data->hour_jx[i].slot=tr_hour_slot(i);
		data->hour_jx[i].gz=tr_gz_text(data->hour_jx[i].gz);
		data->hour_jx[i].luck=tr_hour_luck(data->hour_jx[i].luck);
	}
}

}

