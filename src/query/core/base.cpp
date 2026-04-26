// Shared query primitives, parsing, and common serializers.
namespace{

struct YearEventsCache{
	std::vector<EventRec> solar;
	std::vector<EventRec> phase;
};

struct QueryCache{
	LunCal6 calc;
	SolLunCal solver;
	AppLon app;
	std::map<std::pair<int,int>,YearEventsCache> year_events;

	explicit QueryCache(EphRead&eph)
		: calc(eph),solver(eph),app(eph){}
};

using cli_util::OutTgt;
using cli_util::bld_lpev;
using cli_util::bld_stev;
using cli_util::chk_fmt;
using cli_util::current_jd_utc;
using cli_util::is_opt;
using cli_util::log_year_progress;
using cli_util::mk_erec;
using cli_util::note_out;
using cli_util::open_out;
using cli_util::parse_bool01;
using cli_util::parse_int;
using cli_util::parse_ymd_fixed;
using cli_util::req_val;
using cli_util::to_low;
using lunar::AstroObs;
using lunar::AstroEvt;
using lunar::MoonXg;
using lunar::SkyMode;
using lunar::SkyPick;
using lunar::SkyPos;
using lunar::StarMode;
using lunar::StarPick;
using lunar::calc_astro_evt;
using lunar::calc_sky_pos;
using lunar::calc_moon_xg;
using lunar::make_sky_pick;
using lunar::make_star_pick;
using lunar::parse_sky_mode;
using lunar::parse_star_mode;

using FmtHandler=std::function<void()>;
using FmtMap=std::unordered_map<std::string,FmtHandler>;

void run_fmt(const FmtMap&handlers,const std::string&format,
			 const std::string&ctx){
	auto it=handlers.find(format);
	if(it==handlers.end()){
		throw std::invalid_argument("invalid --format for "+ctx+": "+format);
	}
	it->second();
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

std::string hex_mask64(std::uint64_t value){
	std::ostringstream oss;
	oss<<"0x"<<std::hex<<std::nouppercase<<std::setw(16)<<std::setfill('0')
	   <<value;
	return oss.str();
}

std::string join_code_pipe(const std::vector<int>&codes){
	std::string out;
	for(std::size_t i=0;i<codes.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=std::to_string(codes[i]);
	}
	return out;
}

std::string join_mask_hex_pipe(const std::array<std::uint64_t,2>&masks){
	std::string out;
	for(std::size_t i=0;i<masks.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=hex_mask64(masks[i]);
	}
	return out;
}

template<class Fn>
std::string join_hour_pipe(const std::vector<HliHour>&hours,Fn&&fn){
	std::string out;
	for(std::size_t i=0;i<hours.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=fn(hours[i]);
	}
	return out;
}

int gz_index60(int stem,int branch){
	for(int idx=0;idx<60;++idx){
		if(idx%10==stem&&idx%12==branch){
			return idx;
		}
	}
	return -1;
}

int gz_index_of(const GzNode&g){ return gz_index60(g.stem,g.branch); }

double civil_day_off(int tz_off){
	return static_cast<double>(tz_off)/1440.0;
}

double civil_midjd(int y,int m,int d,int tz_off){
	return greg2jd(y,m,d,0,0,0.0)-civil_day_off(tz_off);
}

void utc2civil(double jd_utc,int tz_off,int&y,int&m,int&d){
	int hour=0;
	int minute=0;
	double second=0.0;
	jd2greg(jd_utc+civil_day_off(tz_off),y,m,d,hour,minute,second);
}

double cst_midjd(int y,int m,int d){ return greg2jd(y,m,d,0,0,0.0)-UTC8DAY; }

void utc2cst(double jd_utc,int&y,int&m,int&d){
	utc2civil(jd_utc,8*60,y,m,d);
}

std::string canonical_tz_text(const std::string&tz){
	return fmt_tz(parse_tz(tz));
}

std::string lunar_day_rule_note(const std::string&lunar_day_tz){
	return lunar::i18n::day_rule_note(canonical_tz_text(lunar_day_tz));
}

std::string lun_dlab(int day){
	return lunar::i18n::tr_lunar_day(day,std::to_string(day));
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
	w.value(tool_ver());
	w.key("ephem");
	w.value(ephem);
	w.key("tz_display");
	w.value(tz_display);
	w.key("notes");
	w.arr_begin();
	w.value(lunar::i18n::tz_note());
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

double parse_double(const std::string&text,const std::string&name){
	try{
		std::size_t idx=0;
		double v=std::stod(text,&idx);
		if(idx!=text.size()){
			throw std::invalid_argument("");
		}
		if(!std::isfinite(v)){
			throw std::invalid_argument("");
		}
		return v;
	}catch(const std::exception&){
		throw std::invalid_argument("invalid "+name+": "+text);
	}
}

HliProfileCode parse_hli_profile_arg(const std::string&text,
									 const std::string&opt){
	HliProfileCode parsed=HliProfileCode::Folk;
	if(!parse_hli_profile(text,&parsed)){
		throw std::invalid_argument(
			"invalid "+opt+": "+text+" (expected folk|ziping|purple|xieji)");
	}
	return parsed;
}

HliYearBoundary parse_hli_year_boundary_arg(const std::string&text){
	HliYearBoundary parsed=HliYearBoundary::LunarNewYear;
	if(!parse_hli_year_boundary(text,&parsed)){
		throw std::invalid_argument(
			"invalid --year-boundary: "+text+
			" (expected lichun|lunar_new_year|dongzhi)");
	}
	return parsed;
}

HliMonthBoundary parse_hli_month_boundary_arg(const std::string&text){
	HliMonthBoundary parsed=HliMonthBoundary::LunarFirstDay;
	if(!parse_hli_month_boundary(text,&parsed)){
		throw std::invalid_argument(
			"invalid --month-boundary: "+text+
			" (expected solar_term|lunar_first_day)");
	}
	return parsed;
}

HliLeapMonthMode parse_hli_leap_month_mode_arg(const std::string&text){
	HliLeapMonthMode parsed=HliLeapMonthMode::InheritPrevious;
	if(!parse_hli_leap_month_mode(text,&parsed)){
		throw std::invalid_argument(
			"invalid --leap-month-mode: "+text+
			" (expected ignore|inherit_previous|split_midway|shift_to_next)");
	}
	return parsed;
}

HliDayBoundary parse_hli_day_boundary_arg(const std::string&text){
	HliDayBoundary parsed=HliDayBoundary::Hour23;
	if(!parse_hli_day_boundary(text,&parsed)){
		throw std::invalid_argument(
			"invalid --day-boundary: "+text+" (expected hour23|hour0)");
	}
	return parsed;
}

void set_hli_year_boundary(HliRuleSet&rules,const std::string&text){
	rules.year_boundary=static_cast<int>(parse_hli_year_boundary_arg(text));
}

void set_hli_month_boundary(HliRuleSet&rules,const std::string&text){
	rules.month_boundary=static_cast<int>(parse_hli_month_boundary_arg(text));
}

void set_hli_leap_month_mode(HliRuleSet&rules,const std::string&text){
	rules.leap_month_mode=static_cast<int>(parse_hli_leap_month_mode_arg(text));
}

void set_hli_day_boundary(HliRuleSet&rules,const std::string&text){
	rules.day_boundary=static_cast<int>(parse_hli_day_boundary_arg(text));
}

std::string join_pipe(const std::vector<std::string>&items){
	std::string out;
	for(std::size_t i=0;i<items.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=items[i];
	}
	return out;
}

bool all_digits(const std::string&s){
	if(s.empty()){
		return false;
	}
	for(char c : s){
		if(!std::isdigit(static_cast<unsigned char>(c))){
			return false;
		}
	}
	return true;
}

void strip_utf8_bom(std::string&line){
	if(line.size()>=3&&
	   static_cast<unsigned char>(line[0])==0xEF&&
	   static_cast<unsigned char>(line[1])==0xBB&&
	   static_cast<unsigned char>(line[2])==0xBF){
		line.erase(0,3);
	}
}

int days_gm(int y,int m);

EventRec mk_astro_rec(const AstroEvt&src,int tz_off){
	EventRec out;
	out.kind=src.kind;
	out.code=src.code;
	out.name=src.name;
	int y=0;
	int m=0;
	int d=0;
	utc2cst(src.jd_utc,y,m,d);
	out.year=y;
	out.jd_utc=src.jd_utc;
	out.jd_tdb=TimeScale::utc_to_tdb(src.jd_utc);
	out.utc_iso=fmt_iso(src.jd_utc,0,true);
	out.loc_iso=fmt_iso(src.jd_utc,tz_off,true);
	return out;
}

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
	bool inc_lecl=false;
	bool inc_secl=false;
	bool inc_ecl=false;
};

struct AstroSiteOpt{
	double lat=0.0;
	double lon=0.0;
	double height=0.0;
	bool has_lat=false;
	bool has_lon=false;
	bool has_height=false;
};

void add_astro_site_options(lunar::ArgParser&parser,AstroSiteOpt&site){
	parser.add_value("--astro-lat",[&](const std::string&v){
		site.lat=parse_double(v,"--astro-lat");
		site.has_lat=true;
	});
	parser.add_value("--astro-lon",[&](const std::string&v){
		site.lon=parse_double(v,"--astro-lon");
		site.has_lon=true;
	});
	parser.add_value("--astro-height",[&](const std::string&v){
		site.height=parse_double(v,"--astro-height");
		site.has_height=true;
	});
}

void validate_astro_site(const AstroSiteOpt&site){
	if(site.has_lat!=site.has_lon){
		throw std::invalid_argument(
			"astro site requires both --astro-lat and --astro-lon");
	}
	if(site.has_height&&!site.has_lat){
		throw std::invalid_argument(
			"--astro-height requires --astro-lat and --astro-lon");
	}
}

std::tuple<int,int,int> parse_ymd(const std::string&s){
	auto [y,m,d]=parse_ymd_fixed(s,"date");
	if(m<1||m>12||d<1||d>days_gm(y,m)){
		throw std::invalid_argument("invalid date value: "+s);
	}
	return {y,m,d};
}

std::pair<int,int> parse_ym(const std::string&s){
	if(s.empty()){
		throw std::invalid_argument("invalid year-month, expected YEAR-MM: "+s);
	}
	std::size_t year_sep=s.find('-',((s[0]=='+'||s[0]=='-')?1u:0u));
	if(year_sep==std::string::npos){
		throw std::invalid_argument("invalid year-month, expected YEAR-MM: "+s);
	}
	std::string ytxt=s.substr(0,year_sep);
	std::string mtxt=s.substr(year_sep+1);
	if(mtxt.size()!=2||!all_digits(mtxt)){
		throw std::invalid_argument("invalid year-month, expected YEAR-MM: "+s);
	}
	int y=parse_int(ytxt,"year");
	int m=parse_int(mtxt,"month");
	if(m<1||m>12){
		throw std::invalid_argument("invalid month in YEAR-MM: "+s);
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
	int consumed=0;
	bool parsed=false;
	if(std::sscanf(s.c_str(),"%d:%d:%d%n",&t_h,&t_m,&t_s,&consumed)==3&&
	   consumed==static_cast<int>(s.size())){
		hh=t_h;
		mm=t_m;
		ss=static_cast<double>(t_s);
		parsed=true;
	}
	consumed=0;
	if(!parsed&&
	   std::sscanf(s.c_str(),"%d:%d%n",&t_h,&t_m,&consumed)==2&&
	   consumed==static_cast<int>(s.size())){
		hh=t_h;
		mm=t_m;
		ss=0.0;
		parsed=true;
	}
	if(!parsed){
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
	if(cfg.default_lang.empty()){
		cfg.default_lang="zh";
	}
	if(cfg.def_fmt.empty()){
		cfg.def_fmt="txt";
	}
	if(cfg.hli_trad.empty()){
		cfg.hli_trad="folk";
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
	bool first_line=true;
	while(std::getline(*in,raw)){
		++line_no;
		std::string trimmed=raw;
		if(first_line){
			strip_utf8_bom(trimmed);
			first_line=false;
		}
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

bool parse_spk(const std::string&ephem,double&jd_start,double&jd_end);

}
