#include "lunar/cli.hpp"

#include<algorithm>
#include<array>
#include<cctype>
#include<cmath>
#include<cstdio>
#include<filesystem>
#include<fstream>
#include<iomanip>
#include<iostream>
#include<limits>
#include<map>
#include<set>
#include<sstream>
#include<stdexcept>
#include<string>
#include<tuple>
#include<utility>
#include<vector>

#include "lunar/app_long.hpp"
#include "lunar/calendar.hpp"
#include "lunar/events.hpp"
#include "lunar/format.hpp"
#include "lunar/ics.hpp"
#include "lunar/interact.hpp"
#include "lunar/js_writer.hpp"
#include "lunar/math.hpp"
#include "lunar/time_scale.hpp"

extern "C"{
#include "SpiceUsr.h"
}

namespace{

struct OutTgt{
	std::ofstream file;
	std::ostream*stream=nullptr;
};

struct LunDate{
	int lunar_year=0;
	int lun_mno=0;
	bool is_leap=false;
	std::string lun_mlab;
	int lunar_day=0;
	std::string lun_label;

	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	double cstday_jd=0.0;
};

struct GregDate{
	int year=0;
	int month=0;
	int day=0;
	double cstday_jd=0.0;
};

struct NearEvt{
	bool has=false;
	EventRec event;
};

struct NearEvents{
	NearEvt solar_prev;
	NearEvt solar_next;
	NearEvt phase_prev;
	NearEvt phase_next;
};

bool is_opt(const std::string&s){ return !s.empty()&&s[0]=='-'; }

std::string to_low(std::string s){
	for(char&c : s){
		c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return s;
}

int parse_int(const std::string&text,const std::string&label){
	std::size_t pos=0;
	int v=0;
	try{
		v=std::stoi(text,&pos);
	}catch(...){
		throw std::invalid_argument("invalid "+label+": "+text);
	}
	if(pos!=text.size()){
		throw std::invalid_argument("invalid "+label+": "+text);
	}
	return v;
}

bool parse_bool01(const std::string&text,const std::string&label){
	if(text=="0"){
		return false;
	}
	if(text=="1"){
		return true;
	}
	throw std::invalid_argument(label+" must be 0 or 1");
}

std::string req_val(const std::vector<std::string>&args,std::size_t&idx,
					const std::string&opt){
	if(idx+1>=args.size()){
		throw std::invalid_argument("missing value for option: "+opt);
	}
	++idx;
	return args[idx];
}

OutTgt open_out(const std::string&path){
	OutTgt out;
	if(path.empty()){
		out.stream=&std::cout;
		return out;
	}
	out.file.open(path,std::ios::binary);
	if(!out.file){
		throw std::runtime_error("failed to open output file: "+path);
	}
	out.stream=&out.file;
	return out;
}

void note_out(const std::string&path,bool quiet){
	if(!path.empty()&&!quiet){
		std::cerr<<"written: "<<path<<std::endl;
	}
}

void chk_fmt(const std::string&format,const std::set<std::string>&allowed,
			 const std::string&ctx){
	if(allowed.find(format)==allowed.end()){
		throw std::invalid_argument("invalid --format for "+ctx+": "+format);
	}
}

double norm2pi(double angle){
	double v=std::fmod(angle,TWO_PI);
	if(v<0.0){
		v+=TWO_PI;
	}
	return v;
}

std::string ymd_str(int y,int m,int d){
	std::ostringstream oss;
	oss<<std::setfill('0')<<std::setw(4)<<y<<"-"<<std::setw(2)<<m<<"-"
	   <<std::setw(2)<<d;
	return oss.str();
}

double cst_midjd(int y,int m,int d){ return greg2jd(y,m,d,0,0,0.0)-UTC8DAY; }

void utc2cst(double jd_utc,int&y,int&m,int&d){
	int hour=0;
	int minute=0;
	double second=0.0;
	jd2greg(jd_utc+UTC8DAY,y,m,d,hour,minute,second);
}

std::string lun_dlab(int day){
	static const std::array<const char*,10> units={
		"一","二","三","四","五","六","七","八","九","十"};
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

std::string phase_elo(double elong){
	static const std::array<const char*,8> names={
		"new_moon", "wax_cres","fst_qtr","wax_gibb",
		"full_moon","wan_gibb","lst_qtr","wan_cres",
	};
	const double step=TWO_PI/8.0;
	int idx=static_cast<int>(std::floor((norm2pi(elong)+0.5*step)/step));
	idx%=8;
	if(idx<0){
		idx+=8;
	}
	return names[static_cast<std::size_t>(idx)];
}

EventRec mk_erec(const std::string&kind,const std::string&code,
				 const std::string&name,int year,double jd_utc,int tz_off){
	EventRec ev;
	ev.kind=kind;
	ev.code=code;
	ev.name=name;
	ev.year=year;
	ev.jd_tdb=std::numeric_limits<double>::quiet_NaN();
	ev.jd_utc=jd_utc;
	ev.utc_iso=fmt_iso(jd_utc,0,true);
	ev.loc_iso=fmt_iso(jd_utc,tz_off,true);
	return ev;
}

std::vector<EventRec> bld_stev(const YearResult&yr,int tz_off){
	std::vector<EventRec> out;
	out.reserve(yr.sol_terms.size());
	for(const auto&kv : yr.sol_terms){
		out.push_back(mk_erec("solar_term",kv.first,kv.second.name,yr.year,
							  kv.second.datetime.toUtcJD(),tz_off));
	}
	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return out;
}

std::vector<EventRec> bld_lpev(const YearResult&yr,int tz_off){
	static const std::array<std::pair<const char*,const char*>,4> defs={{
		{"new_moon","朔"},
		{"fst_qtr","上弦"},
		{"full_moon","望"},
		{"lst_qtr","下弦"},
	}};
	std::vector<EventRec> out;
	out.reserve(yr.lun_phase.size()*defs.size());
	for(const auto&ph : yr.lun_phase){
		const LocalDT dts[]={ph.new_moon,ph.fst_qtr,ph.full_moon,ph.lst_qtr};
		for(std::size_t i=0;i<defs.size();++i){
			out.push_back(mk_erec("lunar_phase",defs[i].first,defs[i].second,
								  yr.year,dts[i].toUtcJD(),tz_off));
		}
	}
	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return out;
}

std::pair<NearEvt,NearEvt> find_pnev(const std::vector<EventRec>&events,
									 double jd_utc){
	NearEvt prev;
	NearEvt next;
	for(const auto&ev : events){
		if(ev.jd_utc<=jd_utc){
			prev.has=true;
			prev.event=ev;
		}else{
			next.has=true;
			next.event=ev;
			break;
		}
	}
	return {prev,next};
}

NearEvents comp_near(EphRead&eph,double jd_utc,int tz_off){
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2cst(jd_utc,cst_year,cst_month,cst_day);

	std::set<int> years={cst_year-1,cst_year,cst_year+1};
	SolLunCal solver(eph);

	std::vector<EventRec> sol_evts;
	std::vector<EventRec> ph_evts;
	for(int y : years){
		YearResult yr=solver.compute_year(y,nullptr);
		std::vector<EventRec> se=bld_stev(yr,tz_off);
		std::vector<EventRec> pe=bld_lpev(yr,tz_off);
		sol_evts.insert(sol_evts.end(),se.begin(),se.end());
		ph_evts.insert(ph_evts.end(),pe.begin(),pe.end());
	}

	std::sort(
		sol_evts.begin(),sol_evts.end(),
		[](const EventRec&a,const EventRec&b){ return a.jd_utc<b.jd_utc; });
	std::sort(
		ph_evts.begin(),ph_evts.end(),
		[](const EventRec&a,const EventRec&b){ return a.jd_utc<b.jd_utc; });

	NearEvents out;
	std::tie(out.solar_prev,out.solar_next)=find_pnev(sol_evts,jd_utc);
	std::tie(out.phase_prev,out.phase_next)=find_pnev(ph_evts,jd_utc);
	return out;
}

LunDate res_lun(EphRead&eph,double jd_utc){
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2cst(jd_utc,cst_year,cst_month,cst_day);
	double qry_dutc=cst_midjd(cst_year,cst_month,cst_day);

	LunCal6 calc(eph);

	bool found=false;
	LunarMonth selected;
	double sel_sday=0.0;
	double sel_eday=0.0;
	for(int y : {cst_year,cst_year-1,cst_year+1}){
		std::vector<LunarMonth> months=comp_sym(calc,y);
		for(const auto&m : months){
			double start_day=
				cst_midjd(m.start_dt.year,m.start_dt.month,m.start_dt.day);
			double end_day=cst_midjd(m.end_dt.year,m.end_dt.month,m.end_dt.day);
			if(qry_dutc>=start_day&&qry_dutc<end_day){
				selected=m;
				sel_sday=start_day;
				sel_eday=end_day;
				found=true;
				break;
			}
		}
		if(found){
			break;
		}
	}
	if(!found){
		throw std::runtime_error("failed to map civil day to lunar month");
	}

	std::vector<LunarMonth> mths_year=comp_sym(calc,cst_year);
	double cny_sday=std::numeric_limits<double>::quiet_NaN();
	for(const auto&m : mths_year){
		if(m.month_no==1&&!m.is_leap){
			cny_sday=cst_midjd(m.start_dt.year,m.start_dt.month,m.start_dt.day);
			break;
		}
	}
	if(std::isnan(cny_sday)){
		throw std::runtime_error("failed to locate CNY boundary");
	}
	int lunar_year=(qry_dutc<cny_sday)?(cst_year-1):cst_year;

	int lunar_day=static_cast<int>(std::floor(qry_dutc-sel_sday+1e-9))+1;
	int month_days=static_cast<int>(std::llround(sel_eday-sel_sday));
	if(lunar_day<1){
		lunar_day=1;
	}
	if(month_days>0&&lunar_day>month_days){
		lunar_day=month_days;
	}

	LunDate info;
	info.lunar_year=lunar_year;
	info.lun_mno=selected.month_no;
	info.is_leap=selected.is_leap;
	info.lun_mlab=selected.label;
	info.lunar_day=lunar_day;
	info.cst_year=cst_year;
	info.cst_month=cst_month;
	info.cst_day=cst_day;
	info.cstday_jd=qry_dutc;

	std::ostringstream label;
	label<<"农历"<<lunar_year<<"年"<<selected.label<<lun_dlab(lunar_day);
	info.lun_label=label.str();
	return info;
}

GregDate res_greg(EphRead&eph,int lunar_year,int month_no,int day,bool leap){
	LunCal6 calc(eph);
	auto find_cny=[&](int greg_year) -> double{
		std::vector<LunarMonth> months=comp_sym(calc,greg_year);
		for(const auto&m : months){
			if(m.month_no==1&&!m.is_leap){
				return cst_midjd(m.start_dt.year,m.start_dt.month,
								 m.start_dt.day);
			}
		}
		throw std::runtime_error("failed to locate CNY boundary");
	};

	double cny_this=find_cny(lunar_year);
	double cny_next=find_cny(lunar_year+1);

	std::vector<LunarMonth> candidates=comp_sym(calc,lunar_year);
	std::vector<LunarMonth> next=comp_sym(calc,lunar_year+1);
	candidates.insert(candidates.end(),next.begin(),next.end());

	bool found=false;
	double start_day=0.0;
	double end_day=0.0;
	for(const auto&m : candidates){
		double s=cst_midjd(m.start_dt.year,m.start_dt.month,m.start_dt.day);
		if(s<cny_this||s>=cny_next){
			continue;
		}
		if(m.month_no==month_no&&m.is_leap==leap){
			start_day=s;
			end_day=cst_midjd(m.end_dt.year,m.end_dt.month,m.end_dt.day);
			found=true;
			break;
		}
	}
	if(!found){
		throw std::invalid_argument(
			"lunar month not found in target lunar year interval");
	}

	int month_days=static_cast<int>(std::llround(end_day-start_day));
	if(month_days<=0){
		throw std::runtime_error("invalid lunar month span");
	}
	if(day<1||day>month_days){
		throw std::invalid_argument("lunar day out of range for the month");
	}

	double tgt_dutc=start_day+static_cast<double>(day-1);
	int gy=0;
	int gm=0;
	int gd=0;
	utc2cst(tgt_dutc,gy,gm,gd);

	GregDate out;
	out.year=gy;
	out.month=gm;
	out.day=gd;
	out.cstday_jd=tgt_dutc;
	return out;
}

void write_meta(JsonWriter&w,const std::string&ephem,
				const std::string&tz_display,
				const std::vector<std::string>&x_notes={}){
	w.key("meta");
	w.obj_begin();
	w.key("tool");
	w.value("lunar");
	w.key("version");
	w.value(tool_ver());
	w.key("schema");
	w.value("lunar.v1");
	w.key("ephem");
	w.value(ephem);
	w.key("tz_display");
	w.value(tz_display);
	w.key("notes");
	w.arr_begin();
	w.value("算法不变，仅新增工具层查询/输出能力");
	w.value("--tz仅影响显示，不改变算法或规则");
	for(const auto&n : x_notes){
		w.value(n);
	}
	w.arr_end();
	w.obj_end();
}

void wr_ejson0(JsonWriter&w,const NearEvt&ne){
	if(!ne.has){
		w.null_val();
		return;
	}
	const EventRec&ev=ne.event;
	w.obj_begin();
	w.key("kind");
	w.value(ev.kind);
	w.key("code");
	w.value(ev.code);
	w.key("name");
	w.value(ev.name);
	w.key("year");
	w.value(ev.year);
	w.key("jd_utc");
	w.value(ev.jd_utc);
	w.key("utc_iso");
	w.value(ev.utc_iso);
	w.key("loc_iso");
	w.value(ev.loc_iso);
	w.obj_end();
}

void wr_ljson(JsonWriter&w,const LunDate&ld){
	w.obj_begin();
	w.key("lunar_year");
	w.value(ld.lunar_year);
	w.key("lun_mno");
	w.value(ld.lun_mno);
	w.key("is_leap");
	w.value(ld.is_leap);
	w.key("lun_mlab");
	w.value(ld.lun_mlab);
	w.key("lunar_day");
	w.value(ld.lunar_day);
	w.key("lun_label");
	w.value(ld.lun_label);
	w.obj_end();
}

std::string format_num(double v){
	std::ostringstream oss;
	oss<<std::setprecision(17)<<v;
	return oss.str();
}

struct AtData{
	std::string time_raw;
	std::string tz_in;
	std::string display_tz;
	double jd_utc=0.0;
	double jd_tdb=0.0;
	std::string utc_iso;
	std::string local_iso;

	double lam_s=0.0;
	double lam_s_dot=0.0;
	double lam_m=0.0;
	double lam_m_dot=0.0;
	double elong=0.0;
	double elong_deg=0.0;
	double ill_frac=0.0;
	double ill_pct=0.0;
	bool waxing=false;
	std::string phase_name;

	LunDate lunar_date;
	bool inc_ev=false;
	NearEvents near_ev;
};

struct BatchLine{
	int line_no=0;
	std::string raw;
};

struct BatchIssue{
	int line_no=0;
	std::string raw;
	std::string message;
};

struct EvtFilt{
	bool inc_st=true;
	bool inc_lph=true;
};

std::tuple<int,int,int> parse_ymd(const std::string&s){
	if(s.size()!=10||s[4]!='-'||s[7]!='-'){
		throw std::invalid_argument("invalid date, expected YYYY-MM-DD: "+s);
	}
	int y=parse_int(s.substr(0,4),"year");
	int m=parse_int(s.substr(5,2),"month");
	int d=parse_int(s.substr(8,2),"day");
	if(m<1||m>12||d<1||d>31){
		throw std::invalid_argument("invalid date value: "+s);
	}
	return {y,m,d};
}

std::pair<int,int> parse_ym(const std::string&s){
	if(s.size()!=7||s[4]!='-'){
		throw std::invalid_argument("invalid year-month, expected YYYY-MM: "+s);
	}
	int y=parse_int(s.substr(0,4),"year");
	int m=parse_int(s.substr(5,2),"month");
	if(m<1||m>12){
		throw std::invalid_argument("invalid month in YYYY-MM: "+s);
	}
	return {y,m};
}

bool is_lyear(int y){ return (y%400==0)||(y%4==0&&y%100!=0); }

int days_gm(int y,int m){
	static const int kDays[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	if(m<1||m>12){
		throw std::invalid_argument("month out of range");
	}
	if(m==2&&is_lyear(y)){
		return 29;
	}
	return kDays[m-1];
}

void parse_hms(const std::string&s,int&hh,int&mm,double&ss){
	hh=12;
	mm=0;
	ss=0.0;
	if(s.empty()){
		return;
	}
	int t_h=0;
	int t_m=0;
	int t_s=0;
	if(std::sscanf(s.c_str(),"%d:%d:%d",&t_h,&t_m,&t_s)==3){
		hh=t_h;
		mm=t_m;
		ss=static_cast<double>(t_s);
	}else if(std::sscanf(s.c_str(),"%d:%d",&t_h,&t_m)==2){
		hh=t_h;
		mm=t_m;
		ss=0.0;
	}else{
		throw std::invalid_argument("invalid time, expected HH:MM[:SS]");
	}
	if(hh<0||hh>23||mm<0||mm>59||ss<0.0||ss>=60.0){
		throw std::invalid_argument("time out of range");
	}
}

InterCfg load_def(){
	InterCfg cfg;
	load_cfg(cfg);
	if(cfg.default_tz.empty()){
		cfg.default_tz="+08:00";
	}
	if(cfg.def_fmt.empty()){
		cfg.def_fmt="txt";
	}
	return cfg;
}

std::vector<BatchLine> read_bat(bool from_stdin,const std::string&input_file){
	std::vector<BatchLine> lines;
	if(!from_stdin&&input_file.empty()){
		return lines;
	}
	std::istream*in=nullptr;
	std::ifstream ifs;
	if(from_stdin){
		in=&std::cin;
	}else{
		ifs.open(input_file,std::ios::binary);
		if(!ifs){
			throw std::runtime_error("failed to open input file: "+input_file);
		}
		in=&ifs;
	}
	std::string raw;
	int line_no=0;
	while(std::getline(*in,raw)){
		++line_no;
		std::string trimmed=raw;
		while(!trimmed.empty()&&(trimmed.back()=='\r'||trimmed.back()=='\n')){
			trimmed.pop_back();
		}
		if(trimmed.empty()){
			continue;
		}
		lines.push_back({line_no,trimmed});
	}
	return lines;
}

AtData at_fromjd(EphRead&eph,double jd_utc,int tz_disp,
				 const std::string&display_tz,const std::string&time_raw,
				 const std::string&tz_in,bool inc_ev){
	AtData out;
	out.time_raw=time_raw;
	out.tz_in=tz_in;
	out.display_tz=display_tz;
	out.jd_utc=jd_utc;
	out.jd_tdb=TimeScale::utc_to_tdb(jd_utc);

	AppLon app(eph);
	auto sun=app.sun_calc(out.jd_tdb);
	auto moon=app.moon_calc(out.jd_tdb);
	out.lam_s=sun.first;
	out.lam_s_dot=sun.second;
	out.lam_m=moon.first;
	out.lam_m_dot=moon.second;

	out.elong=norm2pi(out.lam_m-out.lam_s);
	out.elong_deg=out.elong*180.0/PI;
	out.ill_frac=(1.0-std::cos(out.elong))*0.5;
	out.ill_pct=out.ill_frac*100.0;
	out.waxing=(out.lam_m_dot-out.lam_s_dot)>0.0;
	out.phase_name=phase_elo(out.elong);
	out.lunar_date=res_lun(eph,jd_utc);
	out.inc_ev=inc_ev;
	if(inc_ev){
		out.near_ev=comp_near(eph,jd_utc,tz_disp);
	}

	out.utc_iso=fmt_iso(jd_utc,0,true);
	out.local_iso=fmt_iso(jd_utc,tz_disp,true);
	return out;
}

AtData at_ftxt(EphRead&eph,const std::string&time_raw,
			   const std::string&input_tz,int tz_disp,
			   const std::string&display_tz,bool inc_ev){
	IsoTime parsed=parse_iso(time_raw,input_tz);
	std::string tz_in=
		parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(input_tz));
	return at_fromjd(eph,parsed.jd_utc,tz_disp,display_tz,time_raw,tz_in,
					 inc_ev);
}

void wr_ejson(JsonWriter&w,const EventRec&ev){
	w.obj_begin();
	w.key("kind");
	w.value(ev.kind);
	w.key("code");
	w.value(ev.code);
	w.key("name");
	w.value(ev.name);
	w.key("year");
	w.value(ev.year);
	w.key("jd_utc");
	w.value(ev.jd_utc);
	w.key("utc_iso");
	w.value(ev.utc_iso);
	w.key("loc_iso");
	w.value(ev.loc_iso);
	w.obj_end();
}

void wr_nslot(JsonWriter&w,const NearEvt&ev){
	if(!ev.has){
		w.null_val();
		return;
	}
	wr_ejson(w,ev.event);
}

void wr_adjs(JsonWriter&w,const AtData&d){
	w.obj_begin();
	w.key("sun_lam");
	w.value(d.lam_s);
	w.key("moon_lam");
	w.value(d.lam_m);
	w.key("elongation_rad");
	w.value(d.elong);
	w.key("elongation_deg");
	w.value(d.elong_deg);
	w.key("ill_frac");
	w.value(d.ill_frac);
	w.key("ill_pct");
	w.value(d.ill_pct);
	w.key("waxing");
	w.value(d.waxing);
	w.key("phase_name");
	w.value(d.phase_name);
	w.key("lunar_date");
	wr_ljson(w,d.lunar_date);
	w.key("near_ev");
	if(!d.inc_ev){
		w.null_val();
	}else{
		w.obj_begin();
		w.key("st_prev");
		wr_nslot(w,d.near_ev.solar_prev);
		w.key("st_next");
		wr_nslot(w,d.near_ev.solar_next);
		w.key("lp_prev");
		wr_nslot(w,d.near_ev.phase_prev);
		w.key("lp_next");
		wr_nslot(w,d.near_ev.phase_next);
		w.obj_end();
	}
	w.obj_end();
}

void wr_aijs(JsonWriter&w,const AtData&d){
	w.obj_begin();
	w.key("time_raw");
	w.value(d.time_raw);
	w.key("input_tz");
	w.value(d.tz_in);
	w.key("display_tz");
	w.value(d.display_tz);
	w.key("jd_utc");
	w.value(d.jd_utc);
	w.key("jd_tdb");
	w.value(d.jd_tdb);
	w.key("utc_iso");
	w.value(d.utc_iso);
	w.key("loc_iso");
	w.value(d.local_iso);
	w.obj_end();
}

void wr_etln(std::ostream&os,const std::string&slot,const NearEvt&ne){
	os<<slot<<"\t";
	if(!ne.has){
		os<<"null\tnull\tnull\tnull\tnull\tnull\n";
		return;
	}
	const EventRec&ev=ne.event;
	os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<format_num(ev.jd_utc)<<"\t"
	  <<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
}

void wr_atxt(std::ostream&os,const AtData&d,bool hdr_on){
	if(hdr_on){
		os<<"tool=lunar format=txt type=at tz_display="<<d.display_tz<<"\n";
	}
	os<<"input.time_raw="<<d.time_raw<<"\n";
	os<<"input.input_tz="<<d.tz_in<<"\n";
	os<<"input.display_tz="<<d.display_tz<<"\n";
	os<<"input.jd_utc="<<format_num(d.jd_utc)<<"\n";
	os<<"input.jd_tdb="<<format_num(d.jd_tdb)<<"\n";
	os<<"input.utc_iso="<<d.utc_iso<<"\n";
	os<<"input.loc_iso="<<d.local_iso<<"\n";
	os<<"data.sun_lam="<<format_num(d.lam_s)<<"\n";
	os<<"data.moon_lam="<<format_num(d.lam_m)<<"\n";
	os<<"data.elongation_rad="<<format_num(d.elong)<<"\n";
	os<<"data.elongation_deg="<<format_num(d.elong_deg)<<"\n";
	os<<"data.ill_frac="<<format_num(d.ill_frac)<<"\n";
	os<<"data.ill_pct="<<format_num(d.ill_pct)<<"\n";
	os<<"data.waxing="<<(d.waxing?"1":"0")<<"\n";
	os<<"data.phase_name="<<d.phase_name<<"\n";
	os<<"data.lunar_year="<<d.lunar_date.lunar_year<<"\n";
	os<<"data.lun_mno="<<d.lunar_date.lun_mno<<"\n";
	os<<"data.lun_leap="<<(d.lunar_date.is_leap?"1":"0")<<"\n";
	os<<"data.lun_mlab="<<d.lunar_date.lun_mlab<<"\n";
	os<<"data.lunar_day="<<d.lunar_date.lunar_day<<"\n";
	os<<"data.lun_label="<<d.lunar_date.lun_label<<"\n";
	if(d.inc_ev){
		os<<"[near_ev]\n";
		os<<"slot\tkind\tcode\tname\tjd_utc\ttm_uiso\ttm_liso\n";
		wr_etln(os,"st_prev",d.near_ev.solar_prev);
		wr_etln(os,"st_next",d.near_ev.solar_next);
		wr_etln(os,"lp_prev",d.near_ev.phase_prev);
		wr_etln(os,"lp_next",d.near_ev.phase_next);
	}
}

std::vector<EventRec> col_eyrs(EphRead&eph,const std::set<int>&years,int tz_off,
							   std::ostream*log){
	SolLunCal solver(eph);
	std::vector<EventRec> events;
	for(int y : years){
		YearResult yr=solver.compute_year(y,log);
		std::vector<EventRec> solar=bld_stev(yr,tz_off);
		std::vector<EventRec> phase=bld_lpev(yr,tz_off);
		events.insert(events.end(),solar.begin(),solar.end());
		events.insert(events.end(),phase.begin(),phase.end());
	}
	std::sort(events.begin(),events.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return events;
}

EvtFilt parse_ef(const std::string&text){
	EvtFilt f;
	if(text.empty()){
		return f;
	}
	f.inc_st=false;
	f.inc_lph=false;
	std::string token;
	std::istringstream iss(text);
	while(std::getline(iss,token,',')){
		token=to_low(token);
		if(token=="solar_term"||token=="solar-term"){
			f.inc_st=true;
		}else if(token=="lunar_phase"||token=="lunar-phase"){
			f.inc_lph=true;
		}else if(!token.empty()){
			throw std::invalid_argument("invalid kind: "+token);
		}
	}
	if(!f.inc_st&&!f.inc_lph){
		throw std::invalid_argument("kinds filter cannot be empty");
	}
	return f;
}

bool pass_flt(const EventRec&ev,const EvtFilt&f){
	if(ev.kind=="solar_term"){
		return f.inc_st;
	}
	if(ev.kind=="lunar_phase"){
		return f.inc_lph;
	}
	return false;
}

IcsEvent toic_evt(const EventRec&ev){
	IcsEvent out;
	std::ostringstream uid;
	uid<<"lunar-"<<ev.kind<<"-"<<ev.code<<"-"<<std::setprecision(12)<<ev.jd_utc;
	out.uid=uid.str();
	out.summary=ev.name;
	std::ostringstream desc;
	desc<<"kind="<<ev.kind<<"; code="<<ev.code
		<<"; jd_utc="<<std::setprecision(17)<<ev.jd_utc;
	out.desc=desc.str();
	out.jd_utc=ev.jd_utc;
	return out;
}

void wr_elics(std::ostream&os,const std::string&ephem,
			  const std::string&cal_name,const std::vector<EventRec>&events){
	std::vector<IcsEvent> out;
	out.reserve(events.size());
	for(const auto&ev : events){
		out.push_back(toic_evt(ev));
	}
	write_ics(os,"lunar-cli//"+tool_ver(),cal_name,out,
			  {"schema=lunar.v1","ephem="+ephem,"--tz仅影响显示"});
}

bool parse_spk(const std::string&ephem,double&jd_start,double&jd_end);

}

int cmd_info(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_info();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument("info requires: <bsp>");
	}
	std::string ephem=args[0];
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for info: "+opt);
		}
	}
	chk_fmt(format,{"json","txt"},"info");

	std::error_code ec;
	bool exists=std::filesystem::exists(ephem,ec);
	std::uintmax_t size=exists?std::filesystem::file_size(ephem,ec):0;

	double jd_start=std::numeric_limits<double>::quiet_NaN();
	double jd_end=std::numeric_limits<double>::quiet_NaN();
	bool has_cov=false;
	if(exists){
		try{
			has_cov=parse_spk(ephem,jd_start,jd_end);
		}catch(...){
			has_cov=false;
		}
	}

	OutTgt out=open_out(out_path);
	if(format=="json"){
		JsonWriter w(*out.stream,pretty);
		w.obj_begin();
		write_meta(w,ephem,"Z",{"type=info"});
		w.key("data");
		w.obj_begin();
		w.key("exists");
		w.value(exists);
		w.key("fsize_b");
		w.value(static_cast<int>(
			size>static_cast<std::uintmax_t>(std::numeric_limits<int>::max())
				?std::numeric_limits<int>::max()
				:size));
		w.key("leap_last");
		w.value("2017-01-01");
		w.key("delt_str");
		w.value("year<1970 or year>2026: em53; [1972,2026]: leap table; "
				"[1970,1972): legacy");
		if(has_cov){
			w.key("spk_cov");
			w.obj_begin();
			w.key("jd_tstart");
			w.value(jd_start);
			w.key("jd_tdb_end");
			w.value(jd_end);
			w.key("u_sisoap");
			w.value(fmt_iso(TimeScale::tdb_to_utc(jd_start),0,true));
			w.key("u_eisoap");
			w.value(fmt_iso(TimeScale::tdb_to_utc(jd_end),0,true));
			w.obj_end();
		}else{
			w.key("spk_cov");
			w.null_val();
		}
		w.key("tool_ver");
		w.value(tool_ver());
		w.key("build_time");
		w.value(std::string(__DATE__)+" "+std::string(__TIME__));
		w.obj_end();
		w.obj_end();
		*out.stream<<"\n";
	}else{
		std::ostream&os=*out.stream;
		os<<"tool=lunar format=txt type=info\n";
		os<<"ephem.path="<<ephem<<"\n";
		os<<"ephem.exists="<<(exists?"1":"0")<<"\n";
		os<<"ephem.fsize_b="<<size<<"\n";
		os<<"timescale.leap_last=2017-01-01\n";
		os<<"timescale.delt_str=year<1970 or year>2026: em53; "
			"[1972,2026]: leap table; [1970,1972): legacy\n";
		if(has_cov){
			os<<"spk.jd_tstart="<<format_num(jd_start)<<"\n";
			os<<"spk.jd_tdb_end="<<format_num(jd_end)<<"\n";
			os<<"spk.u_sisoap="<<fmt_iso(TimeScale::tdb_to_utc(jd_start),0,true)
			  <<"\n";
			os<<"spk.u_eisoap="<<fmt_iso(TimeScale::tdb_to_utc(jd_end),0,true)
			  <<"\n";
		}else{
			os<<"spk.coverage=not_avail\n";
		}
		os<<"tool.version="<<tool_ver()<<"\n";
		os<<"tool.build_time="<<__DATE__<<" "<<__TIME__<<"\n";
	}
	note_out(out_path,quiet);
	return 0;
}

int cmd_test(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_test();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument("selftest requires: <bsp>");
	}
	std::string ephem=args[0];
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;
	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for selftest: "+opt);
		}
	}
	chk_fmt(format,{"json","txt"},"selftest");

	struct Case{
		std::string id;
		bool pass=false;
		std::string message;
	};
	std::vector<Case> cases;
	bool all_pass=true;
	try{
		EphRead eph(ephem);

		Case c1;
		c1.id="at_illum";
		try{
			AtData atd=at_ftxt(eph,"2025-06-01T00:00:00+08:00","+08:00",480,
							   "+08:00",true);
			c1.pass=(atd.ill_pct>=0.0&&atd.ill_pct<=100.0);
			c1.message=c1.pass?"ok":"illumination out of [0,100]";
		}catch(const std::exception&ex){
			c1.pass=false;
			c1.message=ex.what();
		}
		cases.push_back(c1);
		all_pass=all_pass&&c1.pass;

		Case c2;
		c2.id="conv_rt";
		try{
			IsoTime p=parse_iso("2026-02-18","+08:00");
			LunDate ld=res_lun(eph,p.jd_utc);
			GregDate g=
				res_greg(eph,ld.lunar_year,ld.lun_mno,ld.lunar_day,ld.is_leap);
			int gy=0,gm=0,gd=0;
			std::tie(gy,gm,gd)=parse_ymd("2026-02-18");
			int ry=0,rm=0,rd=0;
			utc2cst(g.cstday_jd,ry,rm,rd);
			c2.pass=(gy==ry&&gm==rm&&gd==rd);
			c2.message=c2.pass?"ok":"roundtrip mismatch";
		}catch(const std::exception&ex){
			c2.pass=false;
			c2.message=ex.what();
		}
		cases.push_back(c2);
		all_pass=all_pass&&c2.pass;

		Case c3;
		c3.id="y25_cnt";
		try{
			SolLunCal solver(eph);
			YearResult yr=solver.compute_year(2025,quiet?nullptr:&std::cerr);
			std::size_t solar_n=yr.sol_terms.size();
			std::size_t phase_n=yr.lun_phase.size()*4;
			c3.pass=(solar_n==24&&phase_n>=48);
			std::ostringstream msg;
			msg<<"sol_terms="<<solar_n<<", lp_events="<<phase_n;
			c3.message=msg.str();
		}catch(const std::exception&ex){
			c3.pass=false;
			c3.message=ex.what();
		}
		cases.push_back(c3);
		all_pass=all_pass&&c3.pass;
	}catch(const std::exception&ex){
		all_pass=false;
		cases.push_back(Case{"bootstrap",false,ex.what()});
	}

	OutTgt out=open_out(out_path);
	if(format=="json"){
		JsonWriter w(*out.stream,pretty);
		w.obj_begin();
		write_meta(w,ephem,"Z",{"type=selftest"});
		w.key("data");
		w.obj_begin();
		w.key("pass");
		w.value(all_pass);
		w.key("cases");
		w.arr_begin();
		for(const auto&c : cases){
			w.obj_begin();
			w.key("id");
			w.value(c.id);
			w.key("pass");
			w.value(c.pass);
			w.key("message");
			w.value(c.message);
			w.obj_end();
		}
		w.arr_end();
		w.obj_end();
		w.obj_end();
		*out.stream<<"\n";
	}else{
		std::ostream&os=*out.stream;
		os<<"tool=lunar format=txt type=selftest\n";
		os<<"result.pass="<<(all_pass?"1":"0")<<"\n";
		os<<"id\tpass\tmessage\n";
		for(const auto&c : cases){
			os<<c.id<<"\t"<<(c.pass?"1":"0")<<"\t"<<c.message<<"\n";
		}
	}
	note_out(out_path,quiet);
	return all_pass?0:1;
}

int cmd_cfg(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_cfg();
		return 0;
	}
	std::string action=to_low(args[0]);
	std::string format="txt";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;

	auto parse_opt=[&](std::size_t start){
		for(std::size_t i=start;i<args.size();++i){
			const std::string&opt=args[i];
			if(opt=="--format"){
				format=to_low(req_val(args,i,opt));
			}else if(opt=="--out"){
				out_path=req_val(args,i,opt);
			}else if(opt=="--pretty"){
				pretty=parse_bool01(req_val(args,i,opt),"--pretty");
			}else if(opt=="--quiet"){
				quiet=true;
			}else{
				throw std::invalid_argument("unknown option for config: "+opt);
			}
		}
	};

	InterCfg cfg=load_def();
	if(action=="show"){
		parse_opt(1);
		chk_fmt(format,{"json","txt"},"config show");
		OutTgt out=open_out(out_path);
		if(format=="json"){
			JsonWriter w(*out.stream,pretty);
			w.obj_begin();
			w.key("meta");
			w.obj_begin();
			w.key("tool");
			w.value("lunar");
			w.key("schema");
			w.value("lunar.v1");
			w.obj_end();
			w.key("data");
			w.obj_begin();
			w.key("def_bsp");
			w.value(cfg.def_bsp);
			w.key("bsp_dir");
			w.value(cfg.bsp_dir);
			w.key("default_tz");
			w.value(cfg.default_tz);
			w.key("def_fmt");
			w.value(cfg.def_fmt);
			w.key("def_prety");
			w.value(cfg.def_prety);
			w.obj_end();
			w.obj_end();
			*out.stream<<"\n";
		}else{
			*out.stream<<"tool=lunar format=txt type=config\n";
			*out.stream<<"def_bsp="<<cfg.def_bsp<<"\n";
			*out.stream<<"bsp_dir="<<cfg.bsp_dir<<"\n";
			*out.stream<<"default_tz="<<cfg.default_tz<<"\n";
			*out.stream<<"def_fmt="<<cfg.def_fmt<<"\n";
			*out.stream<<"def_prety="<<(cfg.def_prety?"1":"0")<<"\n";
		}
		note_out(out_path,quiet);
		return 0;
	}

	if(action=="set"){
		if(args.size()<3){
			throw std::invalid_argument("config set requires: <key> <value>");
		}
		std::string key=to_low(args[1]);
		std::string value=args[2];
		parse_opt(3);
		if(key=="def_bsp"){
			cfg.def_bsp=value;
		}else if(key=="bsp_dir"){
			cfg.bsp_dir=value;
		}else if(key=="default_tz"){
			parse_tz(value);
			cfg.default_tz=value;
		}else if(key=="def_fmt"){
			std::string v=to_low(value);
			chk_fmt(v,{"txt","json","csv","jsonl","ics"},"config def_fmt");
			cfg.def_fmt=v;
		}else if(key=="def_prety"){
			cfg.def_prety=parse_bool01(value,"def_prety");
		}else{
			throw std::invalid_argument("unknown config key: "+key);
		}
		if(!save_cfg(cfg)){
			throw std::runtime_error("failed to save config: "+CFG_FILE);
		}
		if(!quiet){
			std::cerr<<"written: "<<CFG_FILE<<"\n";
		}
		return 0;
	}

	throw std::invalid_argument("config action must be show or set");
}

int cmd_comp(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_comp();
		return 0;
	}
	std::string shell=to_low(args[0]);
	if(shell=="bash"||shell=="zsh"){
		std::cout<<"_lunar_complete() {\n"
				 <<"  local cur\n"
				 <<"  COMPREPLY=()\n"
				 <<"  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
				 <<"  local cmds=\"months calendar year event download at "
				   "convert day monthview next range search festival almanac "
				   "info selftest config completion\"\n"
				 <<"  if [[ ${COMP_CWORD} -eq 1 ]]; then\n"
				 <<"    COMPREPLY=( $(compgen -W \"${cmds}\" -- \"${cur}\") )\n"
				 <<"    return 0\n"
				 <<"  fi\n"
				 <<"  local opts=\"--help --format --out --tz --pretty --quiet "
				   "--stdin --file --jobs --meta-once --from --to --count "
				   "--kinds\"\n"
				 <<"  COMPREPLY=( $(compgen -W \"${opts}\" -- \"${cur}\") )\n"
				 <<"}\n"
				 <<"complete -F _lunar_complete lunar\n";
		return 0;
	}
	if(shell=="fish"){
		std::cout<<"complete -c lunar -f\n"
				 <<"complete -c lunar -n '__fish_use_subcommand' -a 'months "
				   "calendar year event download at convert day monthview next "
				   "range search festival almanac info selftest config "
				   "completion'\n";
		return 0;
	}
	if(shell=="powershell"){
		std::cout
			<<"Register-ArgumentCompleter -Native -CommandName lunar "
			  "-ScriptBlock {\n"
			<<"  param($wordToComplete, $commandAst, $cursorPosition)\n"
			<<"  $cmds = "
			  "'months','calendar','year','event','download','at','convert','"
			  "day','monthview','next','range','search','festival','almanac','"
			  "info','selftest','config','completion'\n"
			<<"  $cmds | Where-Object { $_ -like \"$wordToComplete*\" } | "
			  "ForEach-Object {\n"
			<<"    "
			  "[System.Management.Automation.CompletionResult]::new($_,$_,'"
			  "ParameterValue',$_)\n"
			<<"  }\n"
			<<"}\n";
		return 0;
	}
	throw std::invalid_argument(
		"completion shell must be bash|zsh|fish|powershell");
}

void use_day(){
	std::cout
		<<"Usage:\n"
		<<"  lunar day <bsp> <YYYY-MM-DD>\n"
		<<"    [--tz ...] [--format json|txt|csv|jsonl] [--out ...] [--pretty "
		  "0|1] [--quiet]\n"
		<<"    [--at HH:MM[:SS]] [--events 0|1]\n"
		<<"Examples:\n"
		<<"  lunar day D:\\de442.bsp 2025-06-01\n"
		<<"  lunar day D:\\de442.bsp 2025-06-01 --format json --out day.json\n";
}

void use_mview(){
	std::cout<<"Usage:\n"
			 <<"  lunar monthview <bsp> <YYYY-MM>\n"
			 <<"    [--tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar monthview D:\\de442.bsp 2025-09 --format txt\n"
			 <<"  lunar monthview D:\\de442.bsp 2025-09 --format csv --out "
			   "month.csv\n";
}

void use_next(){
	std::cout<<"Usage:\n"
			 <<"  lunar next <bsp> --from <time> --count N\n"
			 <<"    [--kinds solar_term,lunar_phase] [--tz ...]\n"
			 <<"    [--format json|txt|csv|ics|jsonl] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar next D:\\de442.bsp --from 2025-06-01T00:00:00+08:00 "
			   "--count 5\n"
			 <<"  lunar next D:\\de442.bsp --from 2025-06-01 --count 10 "
			   "--format ics --out next.ics\n";
}

void use_range(){
	std::cout<<"Usage:\n"
			 <<"  lunar range <bsp> --from <time> --to <time>\n"
			 <<"    [--kinds solar_term,lunar_phase] [--tz ...]\n"
			 <<"    [--format json|txt|csv|ics|jsonl] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar range D:\\de442.bsp --from 2025-01-01 --to 2025-12-31\n"
			 <<"  lunar range D:\\de442.bsp --from 2025-02-01 --to 2025-03-01 "
			   "--format csv\n";
}

void use_search(){
	std::cout
		<<"Usage:\n"
		<<"  lunar search <bsp> <query> [--from <time>] [--count N]\n"
		<<"    [--tz ...] [--format json|txt|csv|ics|jsonl] [--out ...] "
		  "[--pretty 0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar search D:\\de442.bsp \"next full_moon\" --from 2025-06-01\n"
		<<"  lunar search D:\\de442.bsp \"next solar_term 立春\" --from "
		  "2025-01-01 --format json\n";
}

void use_fest(){
	std::cout<<"Usage:\n"
			 <<"  lunar festival <bsp> <year>\n"
			 <<"    [--tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar festival D:\\de442.bsp 2025\n"
			 <<"  lunar festival D:\\de442.bsp 2025 --format csv --out "
			   "festival.csv\n";
}

void use_alm(){
	std::cout<<"Usage:\n"
			 <<"  lunar almanac <bsp> <YYYY-MM-DD>\n"
			 <<"    [--tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17 --format json --out "
			   "almanac.json\n";
}

void use_info(){
	std::cout<<"Usage:\n"
			 <<"  lunar info <bsp> [--format json|txt] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar info D:\\de442.bsp\n"
			 <<"  lunar info D:\\de442.bsp --format json --out info.json\n";
}

void use_test(){
	std::cout
		<<"Usage:\n"
		<<"  lunar selftest <bsp> [--format json|txt] [--out ...] [--pretty "
		  "0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar selftest D:\\de442.bsp\n"
		<<"  lunar selftest D:\\de442.bsp --format json --out selftest.json\n";
}

void use_cfg(){
	std::cout<<"Usage:\n"
			 <<"  lunar config show [--format json|txt] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"  lunar config set <key> <value>\n"
			 <<"Keys:\n"
			 <<"  def_bsp | bsp_dir | default_tz | def_fmt | "
			   "def_prety\n"
			 <<"Examples:\n"
			 <<"  lunar config show\n"
			 <<"  lunar config set default_tz +08:00\n";
}

void use_comp(){
	std::cout<<"Usage:\n"
			 <<"  lunar completion bash|zsh|fish|powershell\n"
			 <<"Examples:\n"
			 <<"  lunar completion powershell > lunar-completion.ps1\n"
			 <<"  lunar completion bash > lunar-completion.bash\n";
}

void cli_at(const AtArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt"},"at");

	int tz_disp=parse_tz(args.tz);
	EphRead eph(args.ephem);
	AtData result=
		at_ftxt(eph,args.time_raw,args.input_tz,tz_disp,args.tz,args.events);

	OutTgt out=open_out(args.out);
	if(format=="json"){
		JsonWriter w(*out.stream,args.pretty);
		w.obj_begin();
		write_meta(
			w,args.ephem,args.tz,
			{"农历判日固定按UTC+8民用日执行；--input-tz仅用于解析无时区输入"});

		w.key("input");
		wr_aijs(w,result);

		w.key("data");
		wr_adjs(w,result);

		w.obj_end();
		*out.stream<<"\n";
	}else{
		wr_atxt(*out.stream,result,true);
	}
	note_out(args.out,args.quiet);
}

void cli_conv(const ConvArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt"},"convert");

	int tz_disp=parse_tz(args.tz);

	EphRead eph(args.ephem);

	bool forward=!args.from_lunar;
	std::string note=
		"农历判日固定按UTC+8民用日执行；--input-tz仅用于解析无时区输入";

	OutTgt out=open_out(args.out);
	if(forward){
		IsoTime parsed=parse_iso(args.in_value,args.input_tz);
		std::string tz_in=
			parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(args.input_tz));

		LunDate lunar_date=res_lun(eph,parsed.jd_utc);
		std::string utc_iso=fmt_iso(parsed.jd_utc,0,true);
		std::string local_iso=fmt_iso(parsed.jd_utc,tz_disp,true);

		if(format=="json"){
			JsonWriter w(*out.stream,args.pretty);
			w.obj_begin();
			write_meta(w,args.ephem,args.tz,{note});

			w.key("input");
			w.obj_begin();
			w.key("direction");
			w.value("greg2lun");
			w.key("value_raw");
			w.value(args.in_value);
			w.key("input_tz");
			w.value(tz_in);
			w.key("display_tz");
			w.value(args.tz);
			w.key("jd_utc");
			w.value(parsed.jd_utc);
			w.key("utc_iso");
			w.value(utc_iso);
			w.key("loc_iso");
			w.value(local_iso);
			w.obj_end();

			w.key("data");
			w.obj_begin();
			w.key("lunar_date");
			wr_ljson(w,lunar_date);

			w.key("greg_date");
			w.obj_begin();
			w.key("cst_date");
			w.value(ymd_str(lunar_date.cst_year,lunar_date.cst_month,
							lunar_date.cst_day));
			w.key("cstday_jd");
			w.value(lunar_date.cstday_jd);
			w.key("cst_uiso");
			w.value(fmt_iso(lunar_date.cstday_jd,0,true));
			w.key("cst_liso");
			w.value(fmt_iso(lunar_date.cstday_jd,tz_disp,true));
			w.obj_end();
			w.obj_end();

			w.obj_end();
			*out.stream<<"\n";
		}else{
			std::ostream&os=*out.stream;
			os<<"tool=lunar format=txt type=convert tz_display="<<args.tz<<"\n";
			os<<"input.direction=greg2lun\n";
			os<<"input.value_raw="<<args.in_value<<"\n";
			os<<"input.input_tz="<<tz_in<<"\n";
			os<<"input.jd_utc="<<format_num(parsed.jd_utc)<<"\n";
			os<<"input.utc_iso="<<utc_iso<<"\n";
			os<<"input.loc_iso="<<local_iso<<"\n";
			os<<"data.lunar_year="<<lunar_date.lunar_year<<"\n";
			os<<"data.lun_mno="<<lunar_date.lun_mno<<"\n";
			os<<"data.lun_leap="<<(lunar_date.is_leap?"1":"0")<<"\n";
			os<<"data.lun_mlab="<<lunar_date.lun_mlab<<"\n";
			os<<"data.lunar_day="<<lunar_date.lunar_day<<"\n";
			os<<"data.lun_label="<<lunar_date.lun_label<<"\n";
			os<<"data.gcst_date="
			  <<ymd_str(lunar_date.cst_year,lunar_date.cst_month,
						lunar_date.cst_day)
			  <<"\n";
			os<<"data.gcst_jd="<<format_num(lunar_date.cstday_jd)<<"\n";
		}
	}else{
		GregDate g=
			res_greg(eph,args.lunar_year,args.lun_mno,args.lunar_day,args.leap);
		LunDate l_check=res_lun(eph,g.cstday_jd);

		if(format=="json"){
			JsonWriter w(*out.stream,args.pretty);
			w.obj_begin();
			write_meta(w,args.ephem,args.tz,{note});

			w.key("input");
			w.obj_begin();
			w.key("direction");
			w.value("lun2greg");
			w.key("lunar_year");
			w.value(args.lunar_year);
			w.key("lun_mno");
			w.value(args.lun_mno);
			w.key("lunar_day");
			w.value(args.lunar_day);
			w.key("is_leap");
			w.value(args.leap);
			w.obj_end();

			w.key("data");
			w.obj_begin();
			w.key("greg_date");
			w.obj_begin();
			w.key("cst_date");
			w.value(ymd_str(g.year,g.month,g.day));
			w.key("cstday_jd");
			w.value(g.cstday_jd);
			w.key("cst_uiso");
			w.value(fmt_iso(g.cstday_jd,0,true));
			w.key("cst_liso");
			w.value(fmt_iso(g.cstday_jd,tz_disp,true));
			w.obj_end();
			w.key("lunar_date");
			wr_ljson(w,l_check);
			w.obj_end();

			w.obj_end();
			*out.stream<<"\n";
		}else{
			std::ostream&os=*out.stream;
			os<<"tool=lunar format=txt type=convert tz_display="<<args.tz<<"\n";
			os<<"input.direction=lun2greg\n";
			os<<"input.lunar_year="<<args.lunar_year<<"\n";
			os<<"input.lun_mno="<<args.lun_mno<<"\n";
			os<<"input.lunar_day="<<args.lunar_day<<"\n";
			os<<"input.lun_leap="<<(args.leap?"1":"0")<<"\n";
			os<<"data.gcst_date="<<ymd_str(g.year,g.month,g.day)<<"\n";
			os<<"data.gcst_jd="<<format_num(g.cstday_jd)<<"\n";
			os<<"data.gcst_uiso="<<fmt_iso(g.cstday_jd,0,true)<<"\n";
			os<<"data.gcst_liso="<<fmt_iso(g.cstday_jd,tz_disp,true)<<"\n";
			os<<"data.lun_label="<<l_check.lun_label<<"\n";
		}
	}

	note_out(args.out,args.quiet);
}

namespace{

int run_abcli(const AtArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"jsonl","json","txt"},"at");
	if(args.from_stdin&&!args.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	std::vector<BatchLine> lines=read_bat(args.from_stdin,args.input_file);
	if(lines.empty()){
		throw std::invalid_argument("batch input is empty");
	}

	if(args.jobs>1&&!args.quiet){
		std::cerr<<"note: --jobs is accepted; current batch executor runs "
				   "seq_run for det_mode behavior.\n";
	}

	const int tz_disp=parse_tz(args.tz);
	EphRead eph(args.ephem);

	struct Row{
		int line_no=0;
		std::string raw;
		bool ok=false;
		std::string error;
		AtData data;
	};
	std::vector<Row> rows;
	rows.reserve(lines.size());
	int err_cnt=0;
	for(const auto&line : lines){
		Row row;
		row.line_no=line.line_no;
		row.raw=line.raw;
		try{
			row.data=
				at_ftxt(eph,line.raw,args.input_tz,tz_disp,args.tz,args.events);
			row.ok=true;
		}catch(const std::exception&ex){
			row.ok=false;
			row.error=ex.what();
			++err_cnt;
		}
		rows.push_back(std::move(row));
	}

	OutTgt out=open_out(args.out);
	if(format=="jsonl"){
		if(args.meta_once){
			JsonWriter wm(*out.stream,false);
			wm.obj_begin();
			wm.key("meta");
			write_meta(wm,args.ephem,args.tz,{"batch=true","schema=lunar.v1"});
			wm.obj_end();
			*out.stream<<"\n";
		}
		for(const auto&row : rows){
			JsonWriter w(*out.stream,false);
			w.obj_begin();
			if(!args.meta_once){
				write_meta(w,args.ephem,args.tz,
						   {"batch=true","schema=lunar.v1"});
			}
			w.key("line_no");
			w.value(row.line_no);
			w.key("raw");
			w.value(row.raw);
			if(row.ok){
				w.key("input");
				wr_aijs(w,row.data);
				w.key("data");
				wr_adjs(w,row.data);
			}else{
				w.key("error");
				w.obj_begin();
				w.key("message");
				w.value(row.error);
				w.obj_end();
			}
			w.obj_end();
			*out.stream<<"\n";
		}
	}else if(format=="json"){
		JsonWriter w(*out.stream,args.pretty);
		w.obj_begin();
		write_meta(w,args.ephem,args.tz,{"batch=true","schema=lunar.v1"});
		w.key("data");
		w.arr_begin();
		for(const auto&row : rows){
			w.obj_begin();
			w.key("line_no");
			w.value(row.line_no);
			w.key("raw");
			w.value(row.raw);
			if(row.ok){
				w.key("input");
				wr_aijs(w,row.data);
				w.key("data");
				wr_adjs(w,row.data);
			}else{
				w.key("error");
				w.obj_begin();
				w.key("message");
				w.value(row.error);
				w.obj_end();
			}
			w.obj_end();
		}
		w.arr_end();
		w.obj_end();
		*out.stream<<"\n";
	}else{
		std::ostream&os=*out.stream;
		os<<"tool=lunar format=txt type=at-batch tz_display="<<args.tz<<"\n";
		os<<"line_no\tstatus\traw\till_pct\tphase_name\tlunar_"
			"date\tmessage\n";
		for(const auto&row : rows){
			os<<row.line_no<<"\t";
			if(row.ok){
				os<<"ok\t"<<row.raw<<"\t"<<format_num(row.data.ill_pct)<<"\t"
				  <<row.data.phase_name<<"\t"<<row.data.lunar_date.lun_label
				  <<"\t"
				  <<"\n";
			}else{
				os<<"error\t"<<row.raw<<"\t\t\t\t"<<row.error<<"\n";
			}
		}
	}
	note_out(args.out,args.quiet);
	return (err_cnt==0)?0:1;
}

int run_cbcli(const ConvArgs&args){
	const std::string format=to_low(args.format);
	chk_fmt(format,{"jsonl","json","txt"},"convert");
	if(args.from_stdin&&!args.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	std::vector<BatchLine> lines=read_bat(args.from_stdin,args.input_file);
	if(lines.empty()){
		throw std::invalid_argument("batch input is empty");
	}
	if(args.jobs>1&&!args.quiet){
		std::cerr<<"note: --jobs is accepted; current batch executor runs "
				   "seq_run for det_mode behavior.\n";
	}

	const int tz_disp=parse_tz(args.tz);
	EphRead eph(args.ephem);

	struct Row{
		int line_no=0;
		std::string raw;
		bool ok=false;
		std::string error;
		std::string direction;
		double jd_utc=std::numeric_limits<double>::quiet_NaN();
		std::string tz_in;
		LunDate lunar_date;
		GregDate greg_date;
	};

	auto parse_lun=[](const std::string&raw,int&y,int&m,int&d,bool&leap){
		std::istringstream iss(raw);
		std::string a,b,c,d4;
		if(!(iss>>a>>b>>c)){
			throw std::invalid_argument(
				"expected: <lunar_year> <month_no> <day> [leap]");
		}
		y=parse_int(a,"lunar_year");
		m=parse_int(b,"lun_mno");
		d=parse_int(c,"lunar_day");
		leap=false;
		if(iss>>d4){
			if(d4=="1"||to_low(d4)=="true"||to_low(d4)=="leap"){
				leap=true;
			}else if(d4=="0"||to_low(d4)=="false"||to_low(d4)=="normal"){
				leap=false;
			}else{
				throw std::invalid_argument(
					"invalid leap flag, expected 0/1/true/false");
			}
		}
	};

	std::vector<Row> rows;
	rows.reserve(lines.size());
	int err_cnt=0;
	for(const auto&line : lines){
		Row row;
		row.line_no=line.line_no;
		row.raw=line.raw;
		try{
			if(!args.from_lunar){
				IsoTime parsed=parse_iso(line.raw,args.input_tz);
				row.direction="greg2lun";
				row.jd_utc=parsed.jd_utc;
				row.tz_in=parsed.has_tz?fmt_tz(parsed.tz_off)
									   :fmt_tz(parse_tz(args.input_tz));
				row.lunar_date=res_lun(eph,parsed.jd_utc);
				row.greg_date.year=row.lunar_date.cst_year;
				row.greg_date.month=row.lunar_date.cst_month;
				row.greg_date.day=row.lunar_date.cst_day;
				row.greg_date.cstday_jd=row.lunar_date.cstday_jd;
			}else{
				int y=0;
				int m=0;
				int d=0;
				bool leap=false;
				parse_lun(line.raw,y,m,d,leap);
				row.direction="lun2greg";
				row.greg_date=res_greg(eph,y,m,d,leap);
				row.jd_utc=row.greg_date.cstday_jd;
				row.tz_in=args.tz;
				row.lunar_date=res_lun(eph,row.greg_date.cstday_jd);
			}
			row.ok=true;
		}catch(const std::exception&ex){
			row.ok=false;
			row.error=ex.what();
			++err_cnt;
		}
		rows.push_back(std::move(row));
	}

	OutTgt out=open_out(args.out);
	const std::string note="农历判日固定按UTC+8民用日执行；--tz仅影响显示";
	if(format=="jsonl"){
		if(args.meta_once){
			JsonWriter wm(*out.stream,false);
			wm.obj_begin();
			wm.key("meta");
			write_meta(wm,args.ephem,args.tz,{note,"batch=true"});
			wm.obj_end();
			*out.stream<<"\n";
		}
		for(const auto&row : rows){
			JsonWriter w(*out.stream,false);
			w.obj_begin();
			if(!args.meta_once){
				write_meta(w,args.ephem,args.tz,{note,"batch=true"});
			}
			w.key("line_no");
			w.value(row.line_no);
			w.key("raw");
			w.value(row.raw);
			if(!row.ok){
				w.key("error");
				w.obj_begin();
				w.key("message");
				w.value(row.error);
				w.obj_end();
			}else{
				w.key("input");
				w.obj_begin();
				w.key("direction");
				w.value(row.direction);
				w.key("input_tz");
				w.value(row.tz_in);
				w.key("jd_utc");
				w.value(row.jd_utc);
				w.obj_end();
				w.key("data");
				w.obj_begin();
				w.key("lunar_date");
				wr_ljson(w,row.lunar_date);
				w.key("gcst_date");
				w.value(ymd_str(row.greg_date.year,row.greg_date.month,
								row.greg_date.day));
				w.key("gcst_jd");
				w.value(row.greg_date.cstday_jd);
				w.obj_end();
			}
			w.obj_end();
			*out.stream<<"\n";
		}
	}else if(format=="json"){
		JsonWriter w(*out.stream,args.pretty);
		w.obj_begin();
		write_meta(w,args.ephem,args.tz,{note,"batch=true"});
		w.key("data");
		w.arr_begin();
		for(const auto&row : rows){
			w.obj_begin();
			w.key("line_no");
			w.value(row.line_no);
			w.key("raw");
			w.value(row.raw);
			if(!row.ok){
				w.key("error");
				w.obj_begin();
				w.key("message");
				w.value(row.error);
				w.obj_end();
			}else{
				w.key("input");
				w.obj_begin();
				w.key("direction");
				w.value(row.direction);
				w.key("input_tz");
				w.value(row.tz_in);
				w.key("jd_utc");
				w.value(row.jd_utc);
				w.obj_end();
				w.key("data");
				w.obj_begin();
				w.key("lunar_date");
				wr_ljson(w,row.lunar_date);
				w.key("gcst_date");
				w.value(ymd_str(row.greg_date.year,row.greg_date.month,
								row.greg_date.day));
				w.key("gcst_jd");
				w.value(row.greg_date.cstday_jd);
				w.obj_end();
			}
			w.obj_end();
		}
		w.arr_end();
		w.obj_end();
		*out.stream<<"\n";
	}else{
		std::ostream&os=*out.stream;
		os<<"tool=lunar format=txt type=convert-batch tz_display="<<args.tz
		  <<"\n";
		os<<"line_no\tstatus\traw\tdirection\tgregorian_cst_date\tlunar_"
			"date\tmessage\n";
		for(const auto&row : rows){
			os<<row.line_no<<"\t";
			if(row.ok){
				os<<"ok\t"<<row.raw<<"\t"<<row.direction<<"\t"
				  <<ymd_str(row.greg_date.year,row.greg_date.month,
							row.greg_date.day)
				  <<"\t"<<row.lunar_date.lun_label<<"\t\n";
			}else{
				os<<"error\t"<<row.raw<<"\t\t\t\t"<<row.error<<"\n";
			}
		}
	}

	note_out(args.out,args.quiet);
	return (err_cnt==0)?0:1;
}

}

int cmd_at(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_at();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"at requires: <bsp> <time> or --time <time>");
	}

	InterCfg cfg=load_def();
	AtArgs a;
	a.ephem=args[0];
	a.input_tz=cfg.default_tz;
	a.tz=cfg.default_tz;
	a.format=to_low(cfg.def_fmt);
	a.pretty=cfg.def_prety;
	if(a.format!="txt"&&a.format!="json"&&a.format!="jsonl"){
		a.format="txt";
	}

	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		a.time_raw=args[i];
		++i;
	}

	for(;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--time"){
			a.time_raw=req_val(args,i,opt);
		}else if(opt=="--stdin"){
			a.from_stdin=true;
		}else if(opt=="--file"){
			a.input_file=req_val(args,i,opt);
		}else if(opt=="--input-tz"){
			a.input_tz=req_val(args,i,opt);
		}else if(opt=="--tz"){
			a.tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			a.format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			a.out=req_val(args,i,opt);
		}else if(opt=="--jobs"){
			a.jobs=parse_int(req_val(args,i,opt),"--jobs");
			if(a.jobs<1){
				throw std::invalid_argument("--jobs must be >= 1");
			}
		}else if(opt=="--meta-once"){
			a.meta_once=parse_bool01(req_val(args,i,opt),"--meta-once");
		}else if(opt=="--pretty"){
			a.pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			a.quiet=true;
		}else if(opt=="--events"){
			a.events=parse_bool01(req_val(args,i,opt),"--events");
		}else if(opt=="-h"||opt=="--help"){
			use_at();
			return 0;
		}else{
			throw std::invalid_argument("unknown option for at: "+opt);
		}
	}

	if(a.from_stdin&&!a.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	bool batch_mode=a.from_stdin||!a.input_file.empty();
	if(batch_mode&&!a.time_raw.empty()){
		if(!a.quiet){
			std::cerr<<"note: positional <time> is ignored in batch mode\n";
		}
		a.time_raw.clear();
	}

	if(a.time_raw.empty()){
		if(!batch_mode){
			throw std::invalid_argument(
				"at requires a <time> or --time <time>");
		}
	}
	if(!batch_mode&&to_low(a.format)=="jsonl"){
		a.format="json";
		a.pretty=false;
	}

	if(batch_mode){
		return run_abcli(a);
	}

	cli_at(a);
	return 0;
}

int cmd_conv(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_conv();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"convert requires: <bsp> <dt_or_tm> or --from-lunar ...");
	}

	InterCfg cfg=load_def();
	ConvArgs c;
	c.ephem=args[0];
	c.input_tz=cfg.default_tz;
	c.tz=cfg.default_tz;
	c.format=to_low(cfg.def_fmt);
	c.pretty=cfg.def_prety;
	if(c.format!="txt"&&c.format!="json"&&c.format!="jsonl"){
		c.format="txt";
	}

	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		c.in_value=args[i];
		c.has_in=true;
		++i;
	}

	for(;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--from-lunar"){
			if(i+3>=args.size()){
				throw std::invalid_argument(
					"--from-lunar requires: <year> <month_no> <day>");
			}
			c.from_lunar=true;
			c.lunar_year=parse_int(args[++i],"lunar_year");
			c.lun_mno=parse_int(args[++i],"lun_mno");
			c.lunar_day=parse_int(args[++i],"lunar_day");
		}else if(opt=="--stdin"){
			c.from_stdin=true;
		}else if(opt=="--file"){
			c.input_file=req_val(args,i,opt);
		}else if(opt=="--leap"){
			c.leap=parse_bool01(req_val(args,i,opt),"--leap");
		}else if(opt=="--input-tz"){
			c.input_tz=req_val(args,i,opt);
		}else if(opt=="--tz"){
			c.tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			c.format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			c.out=req_val(args,i,opt);
		}else if(opt=="--jobs"){
			c.jobs=parse_int(req_val(args,i,opt),"--jobs");
			if(c.jobs<1){
				throw std::invalid_argument("--jobs must be >= 1");
			}
		}else if(opt=="--meta-once"){
			c.meta_once=parse_bool01(req_val(args,i,opt),"--meta-once");
		}else if(opt=="--pretty"){
			c.pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			c.quiet=true;
		}else if(opt=="-h"||opt=="--help"){
			use_conv();
			return 0;
		}else{
			throw std::invalid_argument("unknown option for convert: "+opt);
		}
	}

	if(c.from_lunar&&c.has_in){
		throw std::invalid_argument(
			"do not pass positional <dt_or_tm> when using --from-lunar");
	}
	if(c.from_stdin&&!c.input_file.empty()){
		throw std::invalid_argument(
			"--stdin and --file cannot be used together");
	}
	bool batch_mode=c.from_stdin||!c.input_file.empty();
	if(!c.from_lunar&&!c.has_in&&!batch_mode){
		throw std::invalid_argument(
			"convert requires positional <dt_or_tm> when not using "
			"--from-lunar");
	}
	if(c.from_lunar&&!batch_mode){
		if(c.lun_mno<1||c.lun_mno>12){
			throw std::invalid_argument("lunar month must be 1..12");
		}
	}
	if(!batch_mode&&to_low(c.format)=="jsonl"){
		c.format="json";
		c.pretty=false;
	}

	if(batch_mode){
		return run_cbcli(c);
	}

	cli_conv(c);
	return 0;
}

void use_at(){
	std::cout
		<<"Usage:\n"
		<<"  lunar at <bsp> <time>\n"
		<<"  lunar at <bsp> --time <time>\n"
		<<"  lunar at <bsp> --stdin\n"
		<<"  lunar at <bsp> --file <path>\n"
		<<"    [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]\n"
		<<"    [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] "
		  "[--quiet] [--events 0|1]\n"
		<<"    [--jobs N] [--meta-once 0|1]\n"
		<<"Time formats:\n"
		<<"  YYYY-MM-DD\n"
		<<"  YYYY-MM-DDTHH:MM\n"
		<<"  YYYY-MM-DDTHH:MM:SS[.sss]\n"
		<<"  optional timezone suffix: Z or +HH:MM/-HH:MM\n"
		<<"Examples:\n"
		<<"  lunar at D:\\de442.bsp 2025-06-01T00:00:00+08:00 --format json\n"
		<<"  lunar at D:\\de442.bsp --time 2025-06-01T00:00 --input-tz +08:00 "
		  "--tz Z\n"
		<<"  lunar at D:\\de442.bsp --file times.txt --format jsonl "
		  "--meta-once 1\n"
		<<"Notes:\n"
		<<"  --input-tz only parses input without timezone suffix; --tz only "
		  "affects display.\n";
}

void use_conv(){
	std::cout<<"Usage:\n"
			 <<"  lunar convert <bsp> <dt_or_tm>\n"
			 <<"    [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]\n"
			 <<"    [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] "
			   "[--quiet]\n"
			 <<"    [--stdin|--file <path>] [--jobs N] [--meta-once 0|1]\n"
			 <<"  lunar convert <bsp> --from-lunar <lunar_year> <month_no> "
			   "<day> [--leap 0|1]\n"
			 <<"    [--tz ...] [--format json|txt|jsonl] [--out <path>] "
			   "[--pretty 0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar convert D:\\de442.bsp 2026-02-18 --format txt\n"
			 <<"  lunar convert D:\\de442.bsp 2025-06-01T00:00 --input-tz "
			   "+08:00 --format json\n"
			 <<"  lunar convert D:\\de442.bsp --file dates.txt --format jsonl\n"
			 <<"Notes:\n"
			 <<"  lunar day-boundary mapping is fixed to UTC+8 civil day; --tz "
			   "only affects display.\n";
}

namespace{

std::string csv_quote(const std::string&s){
	bool need_quote=false;
	for(char c : s){
		if(c==','||c=='"'||c=='\n'||c=='\r'){
			need_quote=true;
			break;
		}
	}
	if(!need_quote){
		return s;
	}
	std::string out="\"";
	for(char c : s){
		if(c=='"'){
			out+="\"\"";
		}else{
			out.push_back(c);
		}
	}
	out.push_back('"');
	return out;
}

void wr_eljs(std::ostream&os,const std::string&ephem,const std::string&tz,
			 bool pretty,const std::vector<EventRec>&events,
			 const std::string&type){
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz,{"type="+type});
	w.key("data");
	w.arr_begin();
	for(const auto&ev : events){
		wr_ejson(w,ev);
	}
	w.arr_end();
	w.obj_end();
	os<<"\n";
}

void wr_eltxt(std::ostream&os,const std::string&tz,
			  const std::vector<EventRec>&events,const std::string&type){
	os<<"tool=lunar format=txt type="<<type<<" tz_display="<<tz<<"\n";
	os<<"kind\tcode\tname\tyear\tjd_utc\ttm_uiso\ttm_liso\n";
	for(const auto&ev : events){
		os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t"
		  <<format_num(ev.jd_utc)<<"\t"<<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
	}
}

void wr_elcsv(std::ostream&os,const std::vector<EventRec>&events){
	os<<"kind,code,name,year,jd_utc,utc_iso,loc_iso\n";
	for(const auto&ev : events){
		os<<csv_quote(ev.kind)<<","<<csv_quote(ev.code)<<","<<csv_quote(ev.name)
		  <<","<<ev.year<<","<<format_num(ev.jd_utc)<<","<<csv_quote(ev.utc_iso)
		  <<","<<csv_quote(ev.loc_iso)<<"\n";
	}
}

void wr_eljsl(std::ostream&os,const std::string&ephem,const std::string&tz,
			  const std::vector<EventRec>&events,const std::string&type){
	for(const auto&ev : events){
		JsonWriter w(os,false);
		w.obj_begin();
		write_meta(w,ephem,tz,{"type="+type});
		w.key("data");
		wr_ejson(w,ev);
		w.obj_end();
		os<<"\n";
	}
}

std::vector<EventRec> filt_evs(const std::vector<EventRec>&events,
							   const EvtFilt&filter,double jd_from,double jd_to,
							   bool has_ub,bool gt_from){
	std::vector<EventRec> out;
	for(const auto&ev : events){
		if(!pass_flt(ev,filter)){
			continue;
		}
		if(gt_from){
			if(!(ev.jd_utc>jd_from)){
				continue;
			}
		}else if(ev.jd_utc<jd_from){
			continue;
		}
		if(has_ub&&ev.jd_utc>jd_to){
			continue;
		}
		out.push_back(ev);
	}
	return out;
}

std::vector<EventRec> load_evs(EphRead&eph,double jd_from,double jd_to,
							   const EvtFilt&filter,int tz_off,bool quiet,
							   bool gt_from){
	int y1=0,m1=0,d1=0;
	int y2=0,m2=0,d2=0;
	utc2cst(jd_from,y1,m1,d1);
	utc2cst(jd_to,y2,m2,d2);
	int y_start=std::min(y1,y2)-1;
	int y_end=std::max(y1,y2)+1;
	std::set<int> years;
	for(int y=y_start;y<=y_end;++y){
		years.insert(y);
	}
	std::vector<EventRec> events=
		col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
	std::sort(events.begin(),events.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return filt_evs(events,filter,jd_from,jd_to,true,gt_from);
}

std::vector<EventRec> bld_fest(EphRead&eph,int lunar_year,int tz_off){
	struct FDef{
		const char*name;
		int m;
		int d;
	};
	static const std::array<FDef,7> defs={{
		{"春节",1,1},
		{"元宵",1,15},
		{"端午",5,5},
		{"七夕",7,7},
		{"中秋",8,15},
		{"重阳",9,9},
		{"腊八",12,8},
	}};

	std::vector<EventRec> out;
	out.reserve(defs.size()+1);
	for(const auto&def : defs){
		GregDate g=res_greg(eph,lunar_year,def.m,def.d,false);
		EventRec ev;
		ev.kind="festival";
		ev.code=std::to_string(def.m)+"-"+std::to_string(def.d);
		ev.name=def.name;
		ev.year=lunar_year;
		ev.jd_utc=g.cstday_jd;
		ev.utc_iso=fmt_iso(ev.jd_utc,0,true);
		ev.loc_iso=fmt_iso(ev.jd_utc,tz_off,true);
		out.push_back(std::move(ev));
	}

	GregDate cny_next=res_greg(eph,lunar_year+1,1,1,false);
	EventRec eve;
	eve.kind="festival";
	eve.code="12-last";
	eve.name="除夕";
	eve.year=lunar_year;
	eve.jd_utc=cny_next.cstday_jd-1.0;
	eve.utc_iso=fmt_iso(eve.jd_utc,0,true);
	eve.loc_iso=fmt_iso(eve.jd_utc,tz_off,true);
	out.push_back(std::move(eve));

	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return out;
}

}

int cmd_day(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_day();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("day requires: <bsp> <YYYY-MM-DD>");
	}

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string date_text=args[1];
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"&&format!="jsonl"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	std::string at_time="12:00:00";
	bool inc_ev=true;

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--tz"){
			tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else if(opt=="--at"){
			at_time=req_val(args,i,opt);
		}else if(opt=="--events"){
			inc_ev=parse_bool01(req_val(args,i,opt),"--events");
		}else{
			throw std::invalid_argument("unknown option for day: "+opt);
		}
	}
	chk_fmt(format,{"json","txt","csv","jsonl"},"day");

	int y=0,m=0,d=0;
	std::tie(y,m,d)=parse_ymd(date_text);
	int hh=12;
	int mm=0;
	double ss=0.0;
	parse_hms(at_time,hh,mm,ss);

	int tz_off=parse_tz(tz);
	double smp_jdutc=greg2jd(y,m,d,hh,mm,ss)-UTC8DAY;
	double day_sutc=cst_midjd(y,m,d);
	double day_eutc=day_sutc+1.0;

	EphRead eph(ephem);
	AtData atd=
		at_fromjd(eph,smp_jdutc,tz_off,tz,date_text+"T"+at_time,"+08:00",false);

	std::vector<EventRec> day_events;
	if(inc_ev){
		std::set<int> years={y-1,y,y+1};
		std::vector<EventRec> all=
			col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
		for(const auto&ev : all){
			if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
				day_events.push_back(ev);
			}
		}
		std::sort(
			day_events.begin(),day_events.end(),
			[](const EventRec&a,const EventRec&b){ return a.jd_utc<b.jd_utc; });
	}

	OutTgt out=open_out(out_path);
	if(format=="json"||format=="jsonl"){
		JsonWriter w(*out.stream,(format=="json")?pretty:false);
		w.obj_begin();
		write_meta(w,ephem,tz,{"type=day","农历判日固定UTC+8"});
		w.key("input");
		w.obj_begin();
		w.key("date");
		w.value(date_text);
		w.key("smp_time");
		w.value(at_time);
		w.key("smp_jdutc");
		w.value(smp_jdutc);
		w.obj_end();
		w.key("data");
		w.obj_begin();
		w.key("lunar_date");
		wr_ljson(w,atd.lunar_date);
		w.key("ill_pct");
		w.value(atd.ill_pct);
		w.key("phase_name");
		w.value(atd.phase_name);
		w.key("smp_uiso");
		w.value(atd.utc_iso);
		w.key("smp_liso");
		w.value(atd.local_iso);
		w.key("events");
		w.arr_begin();
		for(const auto&ev : day_events){
			wr_ejson(w,ev);
		}
		w.arr_end();
		w.obj_end();
		w.obj_end();
		*out.stream<<"\n";
	}else if(format=="csv"){
		std::string summary;
		for(std::size_t i=0;i<day_events.size();++i){
			if(i!=0){
				summary+="|";
			}
			summary+=day_events[i].name;
		}
		*out.stream<<"date,lun_label,lun_mlab,is_leap,lunar_"
					 "day,ill_pct,phase_name,smp_tloc"
					 "iso,ev_sum\n";
		*out.stream<<csv_quote(date_text)<<","
				   <<csv_quote(atd.lunar_date.lun_label)<<","
				   <<csv_quote(atd.lunar_date.lun_mlab)<<","
				   <<(atd.lunar_date.is_leap?"1":"0")<<","
				   <<atd.lunar_date.lunar_day<<","<<format_num(atd.ill_pct)<<","
				   <<csv_quote(atd.phase_name)<<","<<csv_quote(atd.local_iso)
				   <<","<<csv_quote(summary)<<"\n";
	}else{
		std::ostream&os=*out.stream;
		os<<"tool=lunar format=txt type=day tz_display="<<tz<<"\n";
		os<<"input.date="<<date_text<<"\n";
		os<<"input.smp_time="<<at_time<<"\n";
		os<<"data.lun_label="<<atd.lunar_date.lun_label<<"\n";
		os<<"data.ill_pct="<<format_num(atd.ill_pct)<<"\n";
		os<<"data.phase_name="<<atd.phase_name<<"\n";
		os<<"data.smp_liso="<<atd.local_iso<<"\n";
		os<<"[events]\n";
		os<<"kind\tcode\tname\tjd_utc\ttm_liso\n";
		for(const auto&ev : day_events){
			os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"
			  <<format_num(ev.jd_utc)<<"\t"<<ev.loc_iso<<"\n";
		}
	}
	note_out(out_path,quiet);
	return 0;
}

int cmd_mview(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_mview();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("monthview requires: <bsp> <YYYY-MM>");
	}

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string ym=args[1];
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--tz"){
			tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for monthview: "+opt);
		}
	}
	chk_fmt(format,{"json","txt","csv"},"monthview");
	int year=0;
	int month=0;
	std::tie(year,month)=parse_ym(ym);

	int tz_off=parse_tz(tz);
	int n_days=days_gm(year,month);
	EphRead eph(ephem);

	std::set<int> years={year-1,year,year+1};
	std::vector<EventRec> events=
		col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
	std::map<int,std::vector<std::string>> day2ev;
	for(const auto&ev : events){
		int ey=0,em=0,ed=0;
		utc2cst(ev.jd_utc,ey,em,ed);
		if(ey==year&&em==month){
			day2ev[ed].push_back(ev.name);
		}
	}

	struct Row{
		std::string greg_date;
		std::string lun_label;
		bool is_leap=false;
		std::string lun_mlab;
		double ill_pct=0.0;
		std::string ev_sum;
	};
	std::vector<Row> rows;
	rows.reserve(static_cast<std::size_t>(n_days));
	for(int d=1;d<=n_days;++d){
		double smp_jdutc=greg2jd(year,month,d,12,0,0.0)-UTC8DAY;
		AtData atd=at_fromjd(eph,smp_jdutc,tz_off,tz,ymd_str(year,month,d),
							 "+08:00",false);
		std::string summary;
		auto it=day2ev.find(d);
		if(it!=day2ev.end()){
			for(std::size_t i=0;i<it->second.size();++i){
				if(i!=0){
					summary+="|";
				}
				summary+=it->second[i];
			}
		}
		rows.push_back(Row{ymd_str(year,month,d),atd.lunar_date.lun_label,
						   atd.lunar_date.is_leap,atd.lunar_date.lun_mlab,
						   atd.ill_pct,summary});
	}

	OutTgt out=open_out(out_path);
	if(format=="json"){
		JsonWriter w(*out.stream,pretty);
		w.obj_begin();
		write_meta(w,ephem,tz,{"type=monthview","农历判日固定UTC+8"});
		w.key("input");
		w.obj_begin();
		w.key("month");
		w.value(ym);
		w.obj_end();
		w.key("data");
		w.arr_begin();
		for(const auto&row : rows){
			w.obj_begin();
			w.key("greg_date");
			w.value(row.greg_date);
			w.key("lun_label");
			w.value(row.lun_label);
			w.key("is_leap");
			w.value(row.is_leap);
			w.key("lun_mlab");
			w.value(row.lun_mlab);
			w.key("ill_pct");
			w.value(row.ill_pct);
			w.key("ev_sum");
			w.value(row.ev_sum);
			w.obj_end();
		}
		w.arr_end();
		w.obj_end();
		*out.stream<<"\n";
	}else if(format=="csv"){
		*out.stream<<"greg_date,lun_label,is_leap,lun_m_"
					 "label,ill_pct,ev_sum\n";
		for(const auto&row : rows){
			*out.stream<<csv_quote(row.greg_date)<<","<<csv_quote(row.lun_label)
					   <<","<<(row.is_leap?"1":"0")<<","
					   <<csv_quote(row.lun_mlab)<<","<<format_num(row.ill_pct)
					   <<","<<csv_quote(row.ev_sum)<<"\n";
		}
	}else{
		std::ostream&os=*out.stream;
		os<<"tool=lunar format=txt type=monthview tz_display="<<tz<<"\n";
		os<<"input.month="<<ym<<"\n";
		os<<"greg_date\tlunar_date_label\tis_leap\tlunar_month_"
			"label\till_pct\tevents_summary\n";
		for(const auto&row : rows){
			os<<row.greg_date<<"\t"<<row.lun_label<<"\t"<<(row.is_leap?"1":"0")
			  <<"\t"<<row.lun_mlab<<"\t"<<format_num(row.ill_pct)<<"\t"
			  <<row.ev_sum<<"\n";
		}
	}
	note_out(out_path,quiet);
	return 0;
}

int cmd_next(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_next();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"next requires: <bsp> --from <time> --count N");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string from_time;
	int count=1;
	std::string kinds="solar_term,lunar_phase";
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"&&format!="ics"&&
	   format!="jsonl"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;

	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--from"){
			from_time=req_val(args,i,opt);
		}else if(opt=="--count"){
			count=parse_int(req_val(args,i,opt),"--count");
			if(count<1){
				throw std::invalid_argument("--count must be >=1");
			}
		}else if(opt=="--kinds"){
			kinds=req_val(args,i,opt);
		}else if(opt=="--tz"){
			tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for next: "+opt);
		}
	}
	if(from_time.empty()){
		throw std::invalid_argument("next requires --from <time>");
	}
	chk_fmt(format,{"json","txt","csv","ics","jsonl"},"next");

	IsoTime parsed=parse_iso(from_time,cfg.default_tz);
	EvtFilt filter=parse_ef(kinds);
	int tz_off=parse_tz(tz);

	EphRead eph(ephem);
	int cst_year=0,cst_month=0,cst_day=0;
	utc2cst(parsed.jd_utc,cst_year,cst_month,cst_day);
	int span=1;
	std::vector<EventRec> picked;
	while(span<=8){
		std::set<int> years;
		for(int y=cst_year-span;y<=cst_year+span;++y){
			years.insert(y);
		}
		std::vector<EventRec> all=
			col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
		std::vector<EventRec> filtered=
			filt_evs(all,filter,parsed.jd_utc,
					 std::numeric_limits<double>::infinity(),false,true);
		std::sort(
			filtered.begin(),filtered.end(),
			[](const EventRec&a,const EventRec&b){ return a.jd_utc<b.jd_utc; });
		if(static_cast<int>(filtered.size())>=count||span==8){
			if(static_cast<int>(filtered.size())>count){
				filtered.resize(static_cast<std::size_t>(count));
			}
			picked=std::move(filtered);
			break;
		}
		++span;
	}

	OutTgt out=open_out(out_path);
	if(format=="json"){
		wr_eljs(*out.stream,ephem,tz,pretty,picked,"next");
	}else if(format=="txt"){
		wr_eltxt(*out.stream,tz,picked,"next");
	}else if(format=="csv"){
		wr_elcsv(*out.stream,picked);
	}else if(format=="jsonl"){
		wr_eljsl(*out.stream,ephem,tz,picked,"next");
	}else{
		wr_elics(*out.stream,ephem,"lunar-next",picked);
	}
	note_out(out_path,quiet);
	return 0;
}

int cmd_range(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_range();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"range requires: <bsp> --from <time> --to <time>");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string from_time;
	std::string to_time;
	std::string kinds="solar_term,lunar_phase";
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"&&format!="ics"&&
	   format!="jsonl"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;

	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--from"){
			from_time=req_val(args,i,opt);
		}else if(opt=="--to"){
			to_time=req_val(args,i,opt);
		}else if(opt=="--kinds"){
			kinds=req_val(args,i,opt);
		}else if(opt=="--tz"){
			tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for range: "+opt);
		}
	}
	if(from_time.empty()||to_time.empty()){
		throw std::invalid_argument(
			"range requires --from <time> and --to <time>");
	}
	chk_fmt(format,{"json","txt","csv","ics","jsonl"},"range");
	IsoTime from_par=parse_iso(from_time,cfg.default_tz);
	IsoTime to_parsed=parse_iso(to_time,cfg.default_tz);
	if(to_parsed.jd_utc<from_par.jd_utc){
		throw std::invalid_argument("--to must be >= --from");
	}

	EvtFilt filter=parse_ef(kinds);
	int tz_off=parse_tz(tz);
	EphRead eph(ephem);
	std::vector<EventRec> picked=load_evs(eph,from_par.jd_utc,to_parsed.jd_utc,
										  filter,tz_off,quiet,false);

	OutTgt out=open_out(out_path);
	if(format=="json"){
		wr_eljs(*out.stream,ephem,tz,pretty,picked,"range");
	}else if(format=="txt"){
		wr_eltxt(*out.stream,tz,picked,"range");
	}else if(format=="csv"){
		wr_elcsv(*out.stream,picked);
	}else if(format=="jsonl"){
		wr_eljsl(*out.stream,ephem,tz,picked,"range");
	}else{
		wr_elics(*out.stream,ephem,"lunar-range",picked);
	}
	note_out(out_path,quiet);
	return 0;
}

int cmd_search(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_search();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("search requires: <bsp> <query>");
	}

	std::string ephem=args[0];
	std::string query=args[1];
	std::string from_time="2025-01-01T00:00:00+08:00";
	int count=1;
	std::string format="txt";
	std::string tz="+08:00";
	std::string out_path;
	bool pretty=true;
	bool quiet=false;

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--from"){
			from_time=req_val(args,i,opt);
		}else if(opt=="--count"){
			count=parse_int(req_val(args,i,opt),"--count");
		}else if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--tz"){
			tz=req_val(args,i,opt);
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for search: "+opt);
		}
	}

	std::istringstream iss(to_low(query));
	std::string a,b,c;
	iss>>a>>b>>c;
	if(a!="next"){
		throw std::invalid_argument(
			"search currently supports query starting with 'next ...'");
	}

	std::vector<std::string> next_args;
	next_args.push_back(ephem);
	next_args.push_back("--from");
	next_args.push_back(from_time);
	next_args.push_back("--count");
	next_args.push_back(std::to_string(count));
	next_args.push_back("--format");
	next_args.push_back(format);
	next_args.push_back("--tz");
	next_args.push_back(tz);
	if(!out_path.empty()){
		next_args.push_back("--out");
		next_args.push_back(out_path);
	}
	next_args.push_back("--pretty");
	next_args.push_back(pretty?"1":"0");
	if(quiet){
		next_args.push_back("--quiet");
	}

	if(b=="full_moon"||b=="new_moon"||b=="fst_qtr"||b=="lst_qtr"){
		next_args.push_back("--kinds");
		next_args.push_back("lunar_phase");
		return cmd_next(next_args);
	}
	if(b=="solar_term"){
		next_args.push_back("--kinds");
		next_args.push_back("solar_term");
		return cmd_next(next_args);
	}
	if(b=="lunar_phase"){
		next_args.push_back("--kinds");
		next_args.push_back("lunar_phase");
		return cmd_next(next_args);
	}
	return cmd_next(next_args);
}

int cmd_fest(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_fest();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("festival requires: <bsp> <year>");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	int year=parse_int(args[1],"year");
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--tz"){
			tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for festival: "+opt);
		}
	}
	chk_fmt(format,{"json","txt","csv"},"festival");

	int tz_off=parse_tz(tz);
	EphRead eph(ephem);
	std::vector<EventRec> festivals=bld_fest(eph,year,tz_off);

	OutTgt out=open_out(out_path);
	if(format=="json"){
		wr_eljs(*out.stream,ephem,tz,pretty,festivals,"festival");
	}else if(format=="csv"){
		wr_elcsv(*out.stream,festivals);
	}else{
		wr_eltxt(*out.stream,tz,festivals,"festival");
	}
	note_out(out_path,quiet);
	return 0;
}

int cmd_alm(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_alm();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("almanac requires: <bsp> <YYYY-MM-DD>");
	}
	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string date_text=args[1];
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;

	for(std::size_t i=2;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="--tz"){
			tz=req_val(args,i,opt);
		}else if(opt=="--format"){
			format=to_low(req_val(args,i,opt));
		}else if(opt=="--out"){
			out_path=req_val(args,i,opt);
		}else if(opt=="--pretty"){
			pretty=parse_bool01(req_val(args,i,opt),"--pretty");
		}else if(opt=="--quiet"){
			quiet=true;
		}else{
			throw std::invalid_argument("unknown option for almanac: "+opt);
		}
	}
	chk_fmt(format,{"json","txt","csv"},"almanac");

	int y=0,m=0,d=0;
	std::tie(y,m,d)=parse_ymd(date_text);
	int tz_off=parse_tz(tz);
	double smp_jdutc=greg2jd(y,m,d,12,0,0.0)-UTC8DAY;
	double day_sutc=cst_midjd(y,m,d);
	double day_eutc=day_sutc+1.0;

	EphRead eph(ephem);
	AtData atd=
		at_fromjd(eph,smp_jdutc,tz_off,tz,date_text+"T12:00:00","+08:00",false);
	std::set<int> years={y-1,y,y+1};
	std::vector<EventRec> all_events=
		col_eyrs(eph,years,tz_off,quiet?nullptr:&std::cerr);
	std::vector<EventRec> day_events;
	for(const auto&ev : all_events){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			day_events.push_back(ev);
		}
	}
	std::vector<EventRec> festivals=
		bld_fest(eph,atd.lunar_date.lunar_year,tz_off);
	std::vector<EventRec> day_fest;
	for(const auto&ev : festivals){
		if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
			day_fest.push_back(ev);
		}
	}

	OutTgt out=open_out(out_path);
	if(format=="json"){
		JsonWriter w(*out.stream,pretty);
		w.obj_begin();
		write_meta(w,ephem,tz,{"type=almanac","农历判日固定UTC+8"});
		w.key("input");
		w.obj_begin();
		w.key("date");
		w.value(date_text);
		w.obj_end();
		w.key("data");
		w.obj_begin();
		w.key("lunar_date");
		wr_ljson(w,atd.lunar_date);
		w.key("ill_pct");
		w.value(atd.ill_pct);
		w.key("phase_name");
		w.value(atd.phase_name);
		w.key("events");
		w.arr_begin();
		for(const auto&ev : day_events){
			wr_ejson(w,ev);
		}
		w.arr_end();
		w.key("festivals");
		w.arr_begin();
		for(const auto&ev : day_fest){
			wr_ejson(w,ev);
		}
		w.arr_end();
		w.obj_end();
		w.obj_end();
		*out.stream<<"\n";
	}else if(format=="csv"){
		std::string ev_sum2;
		for(std::size_t i=0;i<day_events.size();++i){
			if(i!=0){
				ev_sum2+="|";
			}
			ev_sum2+=day_events[i].name;
		}
		std::string fest_sum;
		for(std::size_t i=0;i<day_fest.size();++i){
			if(i!=0){
				fest_sum+="|";
			}
			fest_sum+=day_fest[i].name;
		}
		*out.stream<<"date,lun_label,ill_pct,phase_name,"
					 "events,festivals\n";
		*out.stream<<csv_quote(date_text)<<","
				   <<csv_quote(atd.lunar_date.lun_label)<<","
				   <<format_num(atd.ill_pct)<<","<<csv_quote(atd.phase_name)
				   <<","<<csv_quote(ev_sum2)<<","<<csv_quote(fest_sum)<<"\n";
	}else{
		std::ostream&os=*out.stream;
		os<<"tool=lunar format=txt type=almanac tz_display="<<tz<<"\n";
		os<<"input.date="<<date_text<<"\n";
		os<<"data.lun_label="<<atd.lunar_date.lun_label<<"\n";
		os<<"data.ill_pct="<<format_num(atd.ill_pct)<<"\n";
		os<<"data.phase_name="<<atd.phase_name<<"\n";
		os<<"[events]\n";
		for(const auto&ev : day_events){
			os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.loc_iso<<"\n";
		}
		os<<"[festivals]\n";
		for(const auto&ev : day_fest){
			os<<ev.name<<"\t"<<ev.loc_iso<<"\n";
		}
	}
	note_out(out_path,quiet);
	return 0;
}

namespace{

bool parse_spk(const std::string&ephem,double&jd_start,double&jd_end){
	EphRead reader(ephem);
	reader.load_kern();

	SPICEINT_CELL(ids,10000);
	scard_c(0,&ids);
	spkobj_c(ephem.c_str(),&ids);
	chk_spice("spkobj_c failed");
	SpiceInt nids=card_c(&ids);
	if(nids<=0){
		return false;
	}

	double min_et=std::numeric_limits<double>::infinity();
	double max_et=-std::numeric_limits<double>::infinity();

	for(SpiceInt i=0;i<nids;++i){
		SpiceInt obj=SPICE_CELL_ELEM_I(&ids,i);
		SPICEDOUBLE_CELL(cover,400000);
		scard_c(0,&cover);
		spkcov_c(ephem.c_str(),obj,&cover);
		chk_spice("spkcov_c failed");
		SpiceInt nint=wncard_c(&cover);
		for(SpiceInt k=0;k<nint;++k){
			SpiceDouble b=0.0;
			SpiceDouble e=0.0;
			wnfetd_c(&cover,k,&b,&e);
			if(b<min_et){
				min_et=b;
			}
			if(e>max_et){
				max_et=e;
			}
		}
	}
	if(!std::isfinite(min_et)||!std::isfinite(max_et)||min_et>=max_et){
		return false;
	}
	jd_start=2451545.0+min_et/SEC_DAY;
	jd_end=2451545.0+max_et/SEC_DAY;
	return true;
}

}
