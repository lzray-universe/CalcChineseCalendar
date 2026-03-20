#pragma once

#include<array>
#include<cstdint>
#include<string>
#include<vector>

#include "lunar/app_long.hpp"
#include "lunar/calendar.hpp"
#include "lunar/star.hpp"

enum class HliJianchuCode : int{
	Build=0,
	Remove,
	Full,
	Balance,
	Settle,
	Hold,
	Break,
	Danger,
	Success,
	Receive,
	Open,
	Close,
};

enum class HliGodCode : int{
	AzureDragon=0,
	BrightHall,
	HeavenlyPunishment,
	VermilionBird,
	GoldenCoffer,
	HeavenlyVirtue,
	WhiteTiger,
	JadeHall,
	HeavenlyPrison,
	BlackTortoise,
	ControllerOfFate,
	Gouchen,
	MonthBreak,
	YearBreak,
	YangGongTaboo,
	FourSeparations,
	FourExtinctions,
	ThreeHarmony,
	SixHarmony,
	MonthVirtue,
	MonthVirtueCombine,
	HeavenlyVirtueCombine,
	YearlyVirtue,
	YearlyVirtueCombine,
	RoyalDay,
	OfficialDay,
	GuardDay,
	MutualDay,
	PeopleDay,
	TimeVirtue,
	HeavenlyNoble,
	HeavenlyJoy,
	MonthlyGrace,
	HeavenlyPardon,
	GreatLoss,
	FiveGhosts,
	PeachBlossom,
	BloodOmen,
	HeavenlyDog,
	GoingLoss,
	FourStrikes,
	MonthlyPunishment,
	MonthlyHarm,
};

enum class HliActCode : int{
	Sacrifice=0,
	Travel,
	Relocation,
	ArrangeMarriage,
	Banquet,
	Wedding,
	SetBed,
	Bathing,
	Haircut,
	Construction,
	SeekMedical,
	SubmitMemorial,
	TakeOffice,
	EnterSchool,
	Crowning,
	AddPeople,
	CutClothes,
	RaisePillarsBeam,
	LoomWork,
	OpenMarket,
	SignContractTrade,
	ReceiveWealth,
	PrepareBirthRoom,
	OpenCanal,
	DigWell,
	SetMillstone,
	CleanHouse,
	LevelRoads,
	DemolishHouseWall,
	Logging,
	Capture,
	Hunting,
	Planting,
	Herding,
	BreakGround,
	Burial,
	OpenTomb,
	BestowFavor,
	RecruitTalent,
	PromoteUprightness,
	HandleGovernance,
	RemoveRelieve,
	CosmeticGrooming,
	TrimNails,
	Tailoring,
	OpenGranary,
	SealHoles,
	RepairWall,
	DecorateWalls,
	PrayBlessing,
	PrayOffspring,
	Betrothal,
	MoveResidence,
	BuildProject,
	BuildEmbankment,
	PacifyBorder,
	SelectGenerals,
	Litigation,
	AcquireLivestock,
	BrewFermentation,
	RepairStorehouse,
	Fishing,
	BoatCrossing,
	Needling,
	DeployTroops,
	CelebrationGrant,
	Sailing,
	ClimbHeights,
	ThatchCovering,
	AvoidAll,
	NoMajorTaboo,
};

enum class HliRuleCode : int{
	FollowYiIgnoreJi=0,
	FollowBoth,
	FollowJiIgnoreYi,
	AvoidEverything,
};

enum class HliXiuCode : int{
	Jiao=0,
	Kang,
	Di,
	Fang,
	Xin,
	Wei,
	Ji,
	Dou,
	Niu,
	Nu,
	Xu,
	WeiYan,
	Shi,
	Bi,
	Kui,
	Lou,
	WeiZhi,
	Mao,
	BiWu,
	Zui,
	Shen,
	Jing,
	Gui,
	Liu,
	Xing,
	Zhang,
	Yi,
	Zhen,
};

enum class HliYearBoundary : int{
	LiChun=0,
	LunarNewYear,
	WinterSolstice,
};

enum class HliMonthBoundary : int{
	SolarTerm=0,
	LunarFirstDay,
};

enum class HliLeapMonthMode : int{
	Ignore=0,
	InheritPrevious,
	SplitMidway,
	ShiftToNext,
};

enum class HliDayBoundary : int{
	Hour23=0,
	Hour0,
};

enum class HliProfileCode : int{
	Folk=0,
	ZiPing,
	PurpleStar,
	XieJi,
	Custom,
};

struct HliRuleSet{
	int profile_code=static_cast<int>(HliProfileCode::Folk);
	int year_boundary=static_cast<int>(HliYearBoundary::LunarNewYear);
	int month_boundary=static_cast<int>(HliMonthBoundary::LunarFirstDay);
	int leap_month_mode=static_cast<int>(HliLeapMonthMode::InheritPrevious);
	int day_boundary=static_cast<int>(HliDayBoundary::Hour23);
};

struct GzNode{
	int stem=0;
	int branch=0;
	std::string text;
};

struct HliHour{
	int slot_index=-1;
	int gz_index=-1;
	bool is_bad=false;
	std::string slot;
	std::string gz;
	std::string luck;
};

struct HliData{
	GzNode y_lun;
	GzNode y_lchun;
	GzNode y_rule;
	GzNode m_gz;
	GzNode d_gz;
	GzNode h_gz;
	GzNode h_gz_true;
	std::string bazi_clock;
	std::string bazi_true;

	int rule_profile_code=0;
	int year_boundary_code=0;
	int month_boundary_code=0;
	int leap_month_mode_code=0;
	int day_boundary_code=0;
	std::string rule_profile;
	std::string year_boundary_text;
	std::string month_boundary_text;
	std::string leap_month_mode_text;
	std::string day_boundary_text;
	int jianchu_code=-1;
	int duty_god_code=-1;
	int duty_tag_code=0;
	int clash_branch_code=-1;
	int sha_dir_code=-1;
	int zodiac_day_code=-1;
	int six_he_branch_code=-1;
	int three_he_group_code=-1;
	int nayin_code=-1;
	int fetal_god_code=-1;
	int meridian_code=-1;
	int lucky_dir_code=-1;
	int wealth_dir_code=-1;
	int mascot_dir_code=-1;
	int sun_noble_dir_code=-1;
	int moon_noble_dir_code=-1;
	bool duty_is_yellow=false;
	std::string jianchu;
	std::string duty_god;
	std::string duty_tag;
	std::string clash;
	std::string chong_sha;
	std::string six_he;
	std::string three_he;
	std::string zodiac_day;
	std::string pengzu;
	std::string nayin;
	std::string wx_day;
	std::string fetal_god;
	std::string meridian;
	std::string lucky_dir;
	std::string wealth_dir;
	std::string mascot_dir;
	std::string sun_noble_dir;
	std::string moon_noble_dir;
	int xiu28_code=-1;
	int xiu28_mod28_code=-1;
	std::string xiu28;
	std::string xiu28_mod28;
	std::string xiu_id;

	std::uint64_t good_god_mask=0;
	std::uint64_t bad_god_mask=0;
	std::array<std::uint64_t,2> yi_mask{{0,0}};
	std::array<std::uint64_t,2> ji_mask{{0,0}};
	std::vector<int> good_god_codes;
	std::vector<int> bad_god_codes;
	std::vector<int> yi_codes;
	std::vector<int> ji_codes;
	std::vector<std::string> good_gods;
	std::vector<std::string> bad_gods;
	std::vector<std::string> yi;
	std::vector<std::string> ji;

	int yi_ji_level=0;
	int yi_ji_rule_code=0;
	std::string yi_ji_rule;

	double lon_deg=120.0;
	double eot_min=0.0;
	double tst_min=0.0;

	std::vector<HliHour> hour_jx;
};

struct HliInput{
	double jd_utc=0.0;
	int gy=0;
	int gm=0;
	int gd=0;
	int hh=0;
	int mm=0;
	double ss=0.0;
	int tz_off=480;
	double lon_deg=120.0;

	int lun_year=0;
	int lun_month=0;
	int lun_day=0;
	bool lun_leap=false;

	std::string phase_name;
	lunar::MoonXg moon_xg;
	HliRuleSet rules;
};

HliRuleSet make_hli_rule_set(HliProfileCode profile);

HliRuleSet normalize_hli_rule_set(const HliRuleSet&rules);

bool parse_hli_profile(const std::string&text,HliProfileCode*out);

std::string hli_profile_key(HliProfileCode profile);

bool parse_hli_year_boundary(const std::string&text,HliYearBoundary*out);

bool parse_hli_month_boundary(const std::string&text,HliMonthBoundary*out);

bool parse_hli_leap_month_mode(const std::string&text,HliLeapMonthMode*out);

bool parse_hli_day_boundary(const std::string&text,HliDayBoundary*out);

std::string hli_year_boundary_key(HliYearBoundary boundary);

std::string hli_month_boundary_key(HliMonthBoundary boundary);

std::string hli_leap_month_mode_key(HliLeapMonthMode mode);

std::string hli_day_boundary_key(HliDayBoundary boundary);

HliData calc_hli(EphRead&eph,LunCal6&lc,SolLunCal&solver,AppLon&app,
				 const HliInput&in);
