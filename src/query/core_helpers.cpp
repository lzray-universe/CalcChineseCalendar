namespace{

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
using cli_util::is_opt;
using cli_util::mk_erec;
using cli_util::note_out;
using cli_util::open_out;
using cli_util::parse_bool01;
using cli_util::parse_int;
using cli_util::req_val;
using cli_util::to_low;
using lunar::AstroObs;
using lunar::AstroEvt;
using lunar::MoonXg;
using lunar::StarMode;
using lunar::StarPick;
using lunar::calc_astro_evt;
using lunar::calc_moon_xg;
using lunar::make_star_pick;
using lunar::parse_star_mode;

using OptHandler=
	std::function<void(const std::vector<std::string>&,std::size_t&,
					   const std::string&)>;
using OptMap=std::unordered_map<std::string,OptHandler>;

void apply_opt(const OptMap&handlers,const std::vector<std::string>&args,
			   std::size_t&idx,const std::string&opt,const std::string&ctx){
	auto it=handlers.find(opt);
	if(it==handlers.end()){
		throw std::invalid_argument("unknown option for "+ctx+": "+opt);
	}
	it->second(args,idx,opt);
}

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

const YearEventsCache&load_year_events(QueryCache&cache,int year,int tz_off){
	auto key=std::make_pair(year,tz_off);
	auto it=cache.year_events.find(key);
	if(it!=cache.year_events.end()){
		return it->second;
	}
	YearResult yr=cache.solver.compute_year(year,nullptr);
	YearEventsCache rec;
	rec.solar=bld_stev(yr,tz_off);
	rec.phase=bld_lpev(yr,tz_off);
	auto inserted=cache.year_events.emplace(key,std::move(rec));
	return inserted.first->second;
}

NearEvents comp_near(EphRead&eph,double jd_utc,int tz_off,
					 QueryCache*cache=nullptr){
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2cst(jd_utc,cst_year,cst_month,cst_day);

	std::set<int> years={cst_year-1,cst_year,cst_year+1};

	std::vector<EventRec> sol_evts;
	std::vector<EventRec> ph_evts;
	if(cache){
		for(int y : years){
			const auto&cached=load_year_events(*cache,y,tz_off);
			sol_evts.insert(sol_evts.end(),cached.solar.begin(),
							cached.solar.end());
			ph_evts.insert(ph_evts.end(),cached.phase.begin(),
						   cached.phase.end());
		}
	}else{
		SolLunCal solver(eph);
		for(int y : years){
			YearResult yr=solver.compute_year(y,nullptr);
			std::vector<EventRec> se=bld_stev(yr,tz_off);
			std::vector<EventRec> pe=bld_lpev(yr,tz_off);
			sol_evts.insert(sol_evts.end(),se.begin(),se.end());
			ph_evts.insert(ph_evts.end(),pe.begin(),pe.end());
		}
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

LunDate res_lun(EphRead&eph,double jd_utc,QueryCache*cache=nullptr){
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2cst(jd_utc,cst_year,cst_month,cst_day);
	double qry_dutc=cst_midjd(cst_year,cst_month,cst_day);

	LunCal6 local_calc(eph);
	LunCal6&calc=cache?cache->calc:local_calc;

	bool found=false;
	LunarMonth selected;
	double sel_sday=0.0;
	double sel_eday=0.0;
	for(int y : {cst_year,cst_year-1,cst_year+1}){
		const auto&months=calc.get_months(y);
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

	const auto&mths_year=calc.get_months(cst_year);
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

GregDate res_greg(EphRead&eph,int lunar_year,int month_no,int day,bool leap,
				  QueryCache*cache=nullptr){
	LunCal6 local_calc(eph);
	LunCal6&calc=cache?cache->calc:local_calc;
	auto find_cny=[&](int greg_year) -> double{
		const auto&months=calc.get_months(greg_year);
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

	const auto&months_this=calc.get_months(lunar_year);
	const auto&months_next=calc.get_months(lunar_year+1);

	bool found=false;
	double start_day=0.0;
	double end_day=0.0;
	auto find_month=[&](const std::vector<LunarMonth>&months){
		for(const auto&m : months){
			double s=cst_midjd(m.start_dt.year,m.start_dt.month,m.start_dt.day);
			if(s<cny_this||s>=cny_next){
				continue;
			}
			if(m.month_no==month_no&&m.is_leap==leap){
				start_day=s;
				end_day=cst_midjd(m.end_dt.year,m.end_dt.month,m.end_dt.day);
				found=true;
				return;
			}
		}
	};
	find_month(months_this);
	if(!found){
		find_month(months_next);
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

std::string node_num(double v){
	if(std::isfinite(v)){
		return format_num(v);
	}
	return "null";
}

void wr_node_txt(std::ostream&os,double jd_tdb,const EclipsePointMeta&meta){
	if(!std::isfinite(jd_tdb)){
		os<<"null\tnull\tnull\tnull\tnull\tnull\tnull";
		return;
	}
	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double jd_ut1=jd_utc;
	os<<node_num(jd_ut1)<<"\t"<<node_num(jd_td)<<"\t"<<node_num(jd_utc)<<"\t"
	  <<node_num(meta.zen_lat_deg)<<"\t"<<node_num(meta.zen_lon_deg)<<"\t"
	  <<node_num(meta.pa_deg)<<"\t"<<node_num(meta.axis_deg);
}

void wr_node_kv(std::ostream&os,const std::string&tag,double jd_tdb,
				const EclipsePointMeta&meta){
	double jd_td=std::numeric_limits<double>::quiet_NaN();
	double jd_utc=std::numeric_limits<double>::quiet_NaN();
	double jd_ut1=std::numeric_limits<double>::quiet_NaN();
	if(std::isfinite(jd_tdb)){
		jd_td=TimeScale::tdb_to_tt(jd_tdb);
		jd_utc=TimeScale::tdb_to_utc(jd_tdb);
		jd_ut1=jd_utc;
	}
	os<<tag<<"_jd_ut1="<<node_num(jd_ut1)<<"\n";
	os<<tag<<"_jd_td="<<node_num(jd_td)<<"\n";
	os<<tag<<"_jd="<<node_num(jd_utc)<<"\n";
	os<<tag<<"_zen_lat_deg="<<node_num(meta.zen_lat_deg)<<"\n";
	os<<tag<<"_zen_lon_deg="<<node_num(meta.zen_lon_deg)<<"\n";
	os<<tag<<"_pa_deg="<<node_num(meta.pa_deg)<<"\n";
	os<<tag<<"_axis_deg="<<node_num(meta.axis_deg)<<"\n";
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
	bool has_eot=false;
	EoTData eot;
	MoonXg moon_xg;
	HliData hli;
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
	bool inc_lecl=false;
	bool inc_secl=false;
	bool inc_ecl=false;
};

std::tuple<int,int,int> parse_ymd(const std::string&s){
	if(s.empty()){
		throw std::invalid_argument("invalid date, expected YEAR-MM-DD: "+s);
	}
	std::size_t year_sep=s.find('-',((s[0]=='+'||s[0]=='-')?1u:0u));
	std::size_t month_sep=
		(year_sep==std::string::npos)?std::string::npos:s.find('-',year_sep+1);
	if(year_sep==std::string::npos||month_sep==std::string::npos){
		throw std::invalid_argument("invalid date, expected YEAR-MM-DD: "+s);
	}
	std::string ytxt=s.substr(0,year_sep);
	std::string mtxt=s.substr(year_sep+1,month_sep-year_sep-1);
	std::string dtxt=s.substr(month_sep+1);
	if(mtxt.size()!=2||dtxt.size()!=2||!all_digits(mtxt)||!all_digits(dtxt)){
		throw std::invalid_argument("invalid date, expected YEAR-MM-DD: "+s);
	}
	int y=parse_int(ytxt,"year");
	int m=parse_int(mtxt,"month");
	int d=parse_int(dtxt,"day");
	if(m<1||m>12||d<1||d>31){
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
				 const std::string&tz_in,bool inc_ev,bool calc_eot,
				 double eot_lon_deg,double hli_lon_deg,
				 QueryCache*cache=nullptr){
	AtData out;
	out.time_raw=time_raw;
	out.tz_in=tz_in;
	out.display_tz=display_tz;
	out.jd_utc=jd_utc;
	out.jd_tdb=TimeScale::utc_to_tdb(jd_utc);

	AppLon local_app(eph);
	AppLon&app=cache?cache->app:local_app;
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
	out.lunar_date=res_lun(eph,jd_utc,cache);
	out.moon_xg=calc_moon_xg(eph,jd_utc);

	double lon_use=std::isfinite(hli_lon_deg)
					   ?hli_lon_deg
					   :static_cast<double>(tz_disp)/60.0*15.0;
	int ly=0;
	int lm=0;
	int ld=0;
	int lhh=0;
	int lmm=0;
	double lss=0.0;
	jd2greg(jd_utc+static_cast<double>(tz_disp)/1440.0,ly,lm,ld,lhh,lmm,lss);
	LunCal6 local_calc_hli(eph);
	LunCal6&lc_hli=cache?cache->calc:local_calc_hli;
	SolLunCal local_solver_hli(eph);
	SolLunCal&solver_hli=cache?cache->solver:local_solver_hli;
	HliInput hli_in;
	hli_in.jd_utc=jd_utc;
	hli_in.gy=ly;
	hli_in.gm=lm;
	hli_in.gd=ld;
	hli_in.hh=lhh;
	hli_in.mm=lmm;
	hli_in.ss=lss;
	hli_in.tz_off=tz_disp;
	hli_in.lon_deg=lon_use;
	hli_in.lun_year=out.lunar_date.lunar_year;
	hli_in.lun_month=out.lunar_date.lun_mno;
	hli_in.lun_day=out.lunar_date.lunar_day;
	hli_in.lun_leap=out.lunar_date.is_leap;
	hli_in.phase_name=out.phase_name;
	hli_in.moon_xg=out.moon_xg;
	out.hli=calc_hli(eph,lc_hli,solver_hli,app,hli_in);
	out.inc_ev=inc_ev;
	if(inc_ev){
		out.near_ev=comp_near(eph,jd_utc,tz_disp,cache);
	}
	out.has_eot=calc_eot;
	if(calc_eot){
		out.eot=app.eot_calc(jd_utc,eot_lon_deg);
	}

	out.utc_iso=fmt_iso(jd_utc,0,true);
	out.local_iso=fmt_iso(jd_utc,tz_disp,true);
	return out;
}

AtData at_ftxt(EphRead&eph,const std::string&time_raw,
			   const std::string&input_tz,int tz_disp,
			   const std::string&display_tz,bool inc_ev,bool calc_eot,
			   double eot_lon_deg,double hli_lon_deg,
			   QueryCache*cache=nullptr){
	IsoTime parsed=parse_iso(time_raw,input_tz);
	std::string tz_in=
		parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(input_tz));
	return at_fromjd(eph,parsed.jd_utc,tz_disp,display_tz,time_raw,tz_in,
					 inc_ev,calc_eot,eot_lon_deg,hli_lon_deg,cache);
}

bool is_full_moon_ev(const EventRec&ev){
	return ev.kind=="lunar_phase"&&ev.code=="full_moon";
}

bool is_new_moon_ev(const EventRec&ev){
	return ev.kind=="lunar_phase"&&ev.code=="new_moon";
}

double full_moon_dist_km(EphRead&eph,double jd_utc){
	double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	Vec3 r=eph.get_pos(eph.MOON,eph.EARTH,jd_tdb);
	return r.norm()*AU_KM;
}

void wr_enode(JsonWriter&w,double jd_tdb,int tz_off,
			  const EclipsePointMeta*meta=nullptr){
	if(!std::isfinite(jd_tdb)){
		w.null_val();
		return;
	}
	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double jd_ut1=jd_utc;
	w.obj_begin();
	w.key("jd");
	w.value(jd_utc);
	w.key("jd_tdb");
	w.value(jd_tdb);
	w.key("jd_td");
	w.value(jd_td);
	w.key("jd_ut1");
	w.value(jd_ut1);
	w.key("jd_utc");
	w.value(jd_utc);
	w.key("utc_iso");
	w.value(fmt_iso(jd_utc,0,true));
	w.key("ut1_iso");
	w.value(fmt_iso(jd_ut1,0,true));
	w.key("td_iso");
	w.value(fmt_iso(jd_td,0,true));
	w.key("loc_iso");
	w.value(fmt_iso(jd_utc,tz_off,true));
	w.key("zen_lat_deg");
	if(meta&&std::isfinite(meta->zen_lat_deg)){
		w.value(meta->zen_lat_deg);
	}else{
		w.null_val();
	}
	w.key("zen_lon_deg");
	if(meta&&std::isfinite(meta->zen_lon_deg)){
		w.value(meta->zen_lon_deg);
	}else{
		w.null_val();
	}
	w.key("pa_deg");
	if(meta&&std::isfinite(meta->pa_deg)){
		w.value(meta->pa_deg);
	}else{
		w.null_val();
	}
	w.key("axis_deg");
	if(meta&&std::isfinite(meta->axis_deg)){
		w.value(meta->axis_deg);
	}else{
		w.null_val();
	}
	w.obj_end();
}

void wr_num_or_null(JsonWriter&w,double v){
	if(std::isfinite(v)){
		w.value(v);
	}else{
		w.null_val();
	}
}

void wr_geo_json(JsonWriter&w,const EclipseGeoCoord&g){
	w.obj_begin();
	w.key("ra_deg");
	wr_num_or_null(w,g.ra_deg);
	w.key("dec_deg");
	wr_num_or_null(w,g.dec_deg);
	w.key("sd_deg");
	wr_num_or_null(w,g.sd_deg);
	w.key("ehp_deg");
	wr_num_or_null(w,g.ehp_deg);
	w.obj_end();
}

void wr_lib_json(JsonWriter&w,const EclipseLibration&lib){
	w.obj_begin();
	w.key("l_deg");
	wr_num_or_null(w,lib.l_deg);
	w.key("b_deg");
	wr_num_or_null(w,lib.b_deg);
	w.key("c_deg");
	wr_num_or_null(w,lib.c_deg);
	w.obj_end();
}

std::string node_liso(double jd_tdb,int tz_off){
	if(!std::isfinite(jd_tdb)){
		return "null";
	}
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	return fmt_iso(jd_utc,tz_off,true);
}

void wr_ecljson(JsonWriter&w,const LunarEclipse&ecl,int year,int tz_off){
	w.obj_begin();
	w.key("kind");
	w.value("lunar_eclipse");
	w.key("year");
	w.value(year);
	w.key("has");
	w.value(ecl.has);
	w.key("type");
	w.value(ecl.type);
	w.key("pen_mag");
	wr_num_or_null(w,ecl.pen_mag);
	w.key("umb_mag");
	wr_num_or_null(w,ecl.umb_mag);
	w.key("rp_re");
	wr_num_or_null(w,ecl.rp_re);
	w.key("ru_re");
	wr_num_or_null(w,ecl.ru_re);
	w.key("opp_rp_re");
	wr_num_or_null(w,ecl.opp_rp_re);
	w.key("opp_ru_re");
	wr_num_or_null(w,ecl.opp_ru_re);
	w.key("dur_pen_sec");
	wr_num_or_null(w,ecl.dur_pen_sec);
	w.key("dur_umb_sec");
	wr_num_or_null(w,ecl.dur_umb_sec);
	w.key("dur_tot_sec");
	wr_num_or_null(w,ecl.dur_tot_sec);
	w.key("dt_max_sec");
	wr_num_or_null(w,ecl.dt_max_sec);
	w.key("moon_dist_km");
	wr_num_or_null(w,ecl.moon_dist_km);
	w.key("gamma");
	wr_num_or_null(w,ecl.gamma);
	w.key("eps_deg");
	wr_num_or_null(w,ecl.eps_deg);
	w.key("sun_geo");
	wr_geo_json(w,ecl.sun_geo);
	w.key("moon_geo");
	wr_geo_json(w,ecl.moon_geo);
	w.key("lib");
	wr_lib_json(w,ecl.lib);
	w.key("p1");
	wr_enode(w,ecl.jd_tdb_p1,tz_off,&ecl.p1_meta);
	w.key("u1");
	wr_enode(w,ecl.jd_tdb_u1,tz_off,&ecl.u1_meta);
	w.key("opp");
	wr_enode(w,ecl.jd_tdb_opp,tz_off,&ecl.opp_meta);
	w.key("max");
	wr_enode(w,ecl.jd_tdb_max,tz_off,&ecl.max_meta);
	w.key("u4");
	wr_enode(w,ecl.jd_tdb_u4,tz_off,&ecl.u4_meta);
	w.key("p4");
	wr_enode(w,ecl.jd_tdb_p4,tz_off,&ecl.p4_meta);
	w.key("u2");
	wr_enode(w,ecl.jd_tdb_u2,tz_off,&ecl.u2_meta);
	w.key("u3");
	wr_enode(w,ecl.jd_tdb_u3,tz_off,&ecl.u3_meta);
	w.obj_end();
}

void wr_ptvis_json(JsonWriter&w,const LunarEclipsePointVis&pv,int tz_off){
	w.obj_begin();
	w.key("stage_window");
	w.value(pv.stage_window);
	w.key("lat_deg");
	w.value(pv.lat_deg);
	w.key("lon_deg");
	w.value(pv.lon_deg);
	w.key("height_m");
	w.value(pv.height_m);
	w.key("visible");
	w.value(pv.visible);
	w.key("max_alt_deg");
	if(std::isfinite(pv.max_alt_deg)){
		w.value(pv.max_alt_deg);
	}else{
		w.null_val();
	}
	w.key("first_visible");
	if(std::isfinite(pv.first_jd_utc)){
		w.obj_begin();
		w.key("jd_utc");
		w.value(pv.first_jd_utc);
		w.key("utc_iso");
		w.value(fmt_iso(pv.first_jd_utc,0,true));
		w.key("loc_iso");
		w.value(fmt_iso(pv.first_jd_utc,tz_off,true));
		w.obj_end();
	}else{
		w.null_val();
	}
	w.key("last_visible");
	if(std::isfinite(pv.last_jd_utc)){
		w.obj_begin();
		w.key("jd_utc");
		w.value(pv.last_jd_utc);
		w.key("utc_iso");
		w.value(fmt_iso(pv.last_jd_utc,0,true));
		w.key("loc_iso");
		w.value(fmt_iso(pv.last_jd_utc,tz_off,true));
		w.obj_end();
	}else{
		w.null_val();
	}
	w.key("sample_count");
	w.value(pv.sample_count);
	w.obj_end();
}

void wr_glbvis_json(JsonWriter&w,const LunarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	w.key("stage_window");
	w.value(gv.stage_window);
	w.key("jd_start_utc");
	w.value(gv.jd_start_utc);
	w.key("jd_end_utc");
	w.value(gv.jd_end_utc);
	w.key("utc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,0,true));
	w.key("utc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,0,true));
	w.key("loc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,tz_off,true));
	w.key("loc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,tz_off,true));
	w.key("lat_step_deg");
	w.value(gv.lat_step_deg);
	w.key("lon_step_deg");
	w.value(gv.lon_step_deg);
	w.key("sample_count");
	w.value(gv.sample_count);
	w.key("points");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("lat");
		w.value(pt.lat_deg);
		w.key("lon");
		w.value(pt.lon_deg);
		w.key("max_alt_deg");
		w.value(pt.max_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

void wr_glbvis_geojson(JsonWriter&w,const LunarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	w.key("type");
	w.value("FeatureCollection");
	w.key("stage_window");
	w.value(gv.stage_window);
	w.key("utc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,0,true));
	w.key("utc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,0,true));
	w.key("loc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,tz_off,true));
	w.key("loc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,tz_off,true));
	w.key("lat_step_deg");
	w.value(gv.lat_step_deg);
	w.key("lon_step_deg");
	w.value(gv.lon_step_deg);
	w.key("sample_count");
	w.value(gv.sample_count);
	w.key("features");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("type");
		w.value("Feature");
		w.key("geometry");
		w.obj_begin();
		w.key("type");
		w.value("Point");
		w.key("coordinates");
		w.arr_begin();
		w.value(pt.lon_deg);
		w.value(pt.lat_deg);
		w.arr_end();
		w.obj_end();
		w.key("properties");
		w.obj_begin();
		w.key("max_alt_deg");
		w.value(pt.max_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

void wr_sol_ecljson(JsonWriter&w,const SolarEclipse&ecl,int year,int tz_off){
	w.obj_begin();
	w.key("kind");
	w.value("solar_eclipse");
	w.key("year");
	w.value(year);
	w.key("has");
	w.value(ecl.has);
	w.key("type");
	w.value(ecl.type);
	w.key("mag");
	wr_num_or_null(w,ecl.mag);
	w.key("obscuration");
	wr_num_or_null(w,ecl.obscuration);
	w.key("gamma");
	wr_num_or_null(w,ecl.gamma);
	w.key("sep_max_deg");
	wr_num_or_null(w,ecl.sep_max_deg);
	w.key("sun_sd_max_deg");
	wr_num_or_null(w,ecl.sun_sd_max_deg);
	w.key("moon_sd_max_deg");
	wr_num_or_null(w,ecl.moon_sd_max_deg);
	w.key("sun_dist_km");
	wr_num_or_null(w,ecl.sun_dist_km);
	w.key("moon_dist_km");
	wr_num_or_null(w,ecl.moon_dist_km);
	w.key("dt_max_sec");
	wr_num_or_null(w,ecl.dt_max_sec);
	w.key("rp_re");
	wr_num_or_null(w,ecl.rp_re);
	w.key("ru_re");
	wr_num_or_null(w,ecl.ru_re);
	w.key("c1_loc");
	w.value(node_liso(ecl.jd_tdb_c1,tz_off));
	w.key("c2_loc");
	w.value(node_liso(ecl.jd_tdb_c2,tz_off));
	w.key("max_loc");
	w.value(node_liso(ecl.jd_tdb_max,tz_off));
	w.key("c3_loc");
	w.value(node_liso(ecl.jd_tdb_c3,tz_off));
	w.key("c4_loc");
	w.value(node_liso(ecl.jd_tdb_c4,tz_off));
	w.obj_end();
}

void wr_sol_ptvis_json(JsonWriter&w,const SolarEclipsePointVis&pv,int tz_off){
	w.obj_begin();
	w.key("stage_window");
	w.value(pv.stage_window);
	w.key("lat_deg");
	w.value(pv.lat_deg);
	w.key("lon_deg");
	w.value(pv.lon_deg);
	w.key("height_m");
	w.value(pv.height_m);
	w.key("has_eclipse");
	w.value(pv.has_eclipse);
	w.key("visible");
	w.value(pv.visible);
	w.key("central");
	w.value(pv.central);
	w.key("max_mag");
	wr_num_or_null(w,pv.max_mag);
	w.key("max_obscuration");
	wr_num_or_null(w,pv.max_obscuration);
	w.key("max_sun_alt_deg");
	wr_num_or_null(w,pv.max_sun_alt_deg);
	w.key("max_loc_iso");
	if(std::isfinite(pv.max_jd_utc)){
		w.value(fmt_iso(pv.max_jd_utc,tz_off,true));
	}else{
		w.null_val();
	}
	auto wr_contact=[&](const char*key,double jd_utc){
		w.key(key);
		if(std::isfinite(jd_utc)){
			w.obj_begin();
			w.key("jd_utc");
			w.value(jd_utc);
			w.key("utc_iso");
			w.value(fmt_iso(jd_utc,0,true));
			w.key("loc_iso");
			w.value(fmt_iso(jd_utc,tz_off,true));
			w.obj_end();
		}else{
			w.null_val();
		}
	};
	wr_contact("c1",pv.c1_jd_utc);
	wr_contact("c2",pv.c2_jd_utc);
	wr_contact("max",pv.max_jd_utc);
	wr_contact("c3",pv.c3_jd_utc);
	wr_contact("c4",pv.c4_jd_utc);
	w.key("first_visible");
	if(std::isfinite(pv.first_jd_utc)){
		w.value(fmt_iso(pv.first_jd_utc,tz_off,true));
	}else{
		w.null_val();
	}
	w.key("last_visible");
	if(std::isfinite(pv.last_jd_utc)){
		w.value(fmt_iso(pv.last_jd_utc,tz_off,true));
	}else{
		w.null_val();
	}
	w.key("sample_count");
	w.value(pv.sample_count);
	w.obj_end();
}

void wr_sol_glbvis_json(JsonWriter&w,const SolarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	w.key("stage_window");
	w.value(gv.stage_window);
	w.key("jd_start_utc");
	w.value(gv.jd_start_utc);
	w.key("jd_end_utc");
	w.value(gv.jd_end_utc);
	w.key("utc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,0,true));
	w.key("utc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,0,true));
	w.key("loc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,tz_off,true));
	w.key("loc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,tz_off,true));
	w.key("lat_step_deg");
	w.value(gv.lat_step_deg);
	w.key("lon_step_deg");
	w.value(gv.lon_step_deg);
	w.key("sample_count");
	w.value(gv.sample_count);
	w.key("points");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("lat");
		w.value(pt.lat_deg);
		w.key("lon");
		w.value(pt.lon_deg);
		w.key("max_mag");
		w.value(pt.max_mag);
		w.key("max_sun_alt_deg");
		w.value(pt.max_sun_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

void wr_sol_glbvis_geojson(JsonWriter&w,const SolarEclipseGlobalVis&gv,int tz_off){
	w.obj_begin();
	w.key("type");
	w.value("FeatureCollection");
	w.key("stage_window");
	w.value(gv.stage_window);
	w.key("utc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,0,true));
	w.key("utc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,0,true));
	w.key("loc_start_iso");
	w.value(fmt_iso(gv.jd_start_utc,tz_off,true));
	w.key("loc_end_iso");
	w.value(fmt_iso(gv.jd_end_utc,tz_off,true));
	w.key("lat_step_deg");
	w.value(gv.lat_step_deg);
	w.key("lon_step_deg");
	w.value(gv.lon_step_deg);
	w.key("sample_count");
	w.value(gv.sample_count);
	w.key("features");
	w.arr_begin();
	for(const auto&pt : gv.points){
		w.obj_begin();
		w.key("type");
		w.value("Feature");
		w.key("geometry");
		w.obj_begin();
		w.key("type");
		w.value("Point");
		w.key("coordinates");
		w.arr_begin();
		w.value(pt.lon_deg);
		w.value(pt.lat_deg);
		w.arr_end();
		w.obj_end();
		w.key("properties");
		w.obj_begin();
		w.key("max_mag");
		w.value(pt.max_mag);
		w.key("max_sun_alt_deg");
		w.value(pt.max_sun_alt_deg);
		w.key("first_visible");
		w.value(fmt_iso(pt.first_jd_utc,tz_off,true));
		w.key("last_visible");
		w.value(fmt_iso(pt.last_jd_utc,tz_off,true));
		w.obj_end();
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

LunarEclipse calc_ecl_for_event(EphRead&eph,const EventRec&ev){
	LunarEclipse ecl;
	if(is_full_moon_ev(ev)||ev.kind=="lunar_eclipse"){
		double jd_tdb=
			std::isfinite(ev.jd_tdb)?ev.jd_tdb:TimeScale::utc_to_tdb(ev.jd_utc);
		calc_lunar_eclipse(eph,jd_tdb,&ecl);
	}
	return ecl;
}

SolarEclipse calc_sol_ecl_for_event(EphRead&eph,const EventRec&ev){
	SolarEclipse ecl;
	if(is_new_moon_ev(ev)||ev.kind=="solar_eclipse"){
		double jd_tdb=
			std::isfinite(ev.jd_tdb)?ev.jd_tdb:TimeScale::utc_to_tdb(ev.jd_utc);
		calc_solar_eclipse(eph,jd_tdb,&ecl);
	}
	return ecl;
}

void wr_ejson(JsonWriter&w,const EventRec&ev,EphRead&eph,bool calc_eclipse=false,
			  int tz_off=0){
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
	if(ev.kind=="lunar_eclipse"){
		w.key("lunar_eclipse");
		LunarEclipse ecl=calc_ecl_for_event(eph,ev);
		wr_ecljson(w,ecl,ev.year,tz_off);
	}
	if(ev.kind=="solar_eclipse"){
		w.key("solar_eclipse");
		SolarEclipse ecl=calc_sol_ecl_for_event(eph,ev);
		wr_sol_ecljson(w,ecl,ev.year,tz_off);
	}
	if(is_full_moon_ev(ev)){
		w.key("moon_dist_km");
		w.value(full_moon_dist_km(eph,ev.jd_utc));
		if(calc_eclipse){
			w.key("lunar_eclipse");
			LunarEclipse ecl=calc_ecl_for_event(eph,ev);
			wr_ecljson(w,ecl,ev.year,tz_off);
		}
	}
	if(is_new_moon_ev(ev)&&calc_eclipse){
		w.key("solar_eclipse");
		SolarEclipse ecl=calc_sol_ecl_for_event(eph,ev);
		wr_sol_ecljson(w,ecl,ev.year,tz_off);
	}
	w.obj_end();
}

void wr_nslot(JsonWriter&w,const NearEvt&ev,EphRead&eph){
	if(!ev.has){
		w.null_val();
		return;
	}
	wr_ejson(w,ev.event,eph);
}

void wr_gz_json(JsonWriter&w,const GzNode&g){
	w.obj_begin();
	w.key("text");
	w.value(g.text);
	w.key("stem");
	w.value(g.stem);
	w.key("branch");
	w.value(g.branch);
	w.obj_end();
}

void wr_hli_json(JsonWriter&w,const HliData&h){
	w.obj_begin();
	w.key("year_lunar");
	wr_gz_json(w,h.y_lun);
	w.key("year_lchun");
	wr_gz_json(w,h.y_lchun);
	w.key("month");
	wr_gz_json(w,h.m_gz);
	w.key("day");
	wr_gz_json(w,h.d_gz);
	w.key("hour_clock");
	wr_gz_json(w,h.h_gz);
	w.key("hour_true_solar");
	wr_gz_json(w,h.h_gz_true);
	w.key("bazi_clock");
	w.value(h.bazi_clock);
	w.key("bazi_true");
	w.value(h.bazi_true);

	w.key("jianchu");
	w.value(h.jianchu);
	w.key("duty_god");
	w.value(h.duty_god);
	w.key("duty_tag");
	w.value(h.duty_tag);
	w.key("clash");
	w.value(h.clash);
	w.key("chong_sha");
	w.value(h.chong_sha);
	w.key("zodiac_day");
	w.value(h.zodiac_day);
	w.key("six_he");
	w.value(h.six_he);
	w.key("three_he");
	w.value(h.three_he);
	w.key("pengzu");
	w.value(h.pengzu);
	w.key("nayin");
	w.value(h.nayin);
	w.key("wuxing_day");
	w.value(h.wx_day);
	w.key("fetal_god");
	w.value(h.fetal_god);
	w.key("meridian");
	w.value(h.meridian);
	w.key("lucky_dir");
	w.value(h.lucky_dir);
	w.key("wealth_dir");
	w.value(h.wealth_dir);
	w.key("mascot_dir");
	w.value(h.mascot_dir);
	w.key("sun_noble_dir");
	w.value(h.sun_noble_dir);
	w.key("moon_noble_dir");
	w.value(h.moon_noble_dir);
	w.key("xiu28");
	w.value(h.xiu28);
	w.key("xiu_star");
	w.value(h.xiu_id);

	w.key("good_gods");
	w.arr_begin();
	for(const auto&s : h.good_gods){
		w.value(s);
	}
	w.arr_end();
	w.key("bad_gods");
	w.arr_begin();
	for(const auto&s : h.bad_gods){
		w.value(s);
	}
	w.arr_end();
	w.key("yi");
	w.arr_begin();
	for(const auto&s : h.yi){
		w.value(s);
	}
	w.arr_end();
	w.key("ji");
	w.arr_begin();
	for(const auto&s : h.ji){
		w.value(s);
	}
	w.arr_end();
	w.key("yi_ji_level");
	w.value(h.yi_ji_level);
	w.key("yi_ji_rule");
	w.value(h.yi_ji_rule);
	w.key("lon_deg");
	w.value(h.lon_deg);
	w.key("eot_minutes");
	w.value(h.eot_min);
	w.key("true_solar_minutes");
	w.value(h.tst_min);
	w.key("hour_jx");
	w.arr_begin();
	for(const auto&x : h.hour_jx){
		w.obj_begin();
		w.key("slot");
		w.value(x.slot);
		w.key("gz");
		w.value(x.gz);
		w.key("luck");
		w.value(x.luck);
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
}

void wr_adjs(JsonWriter&w,const AtData&d,EphRead&eph){
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
	w.key("moon_xg");
	w.obj_begin();
	w.key("region");
	w.value(d.moon_xg.region);
	w.key("star_id");
	w.value(d.moon_xg.star_id);
	w.key("star_name");
	w.value(d.moon_xg.star_name);
	w.key("sep_deg");
	wr_num_or_null(w,d.moon_xg.sep_deg);
	w.obj_end();
	w.key("lunar_date");
	wr_ljson(w,d.lunar_date);
	w.key("huangli");
	wr_hli_json(w,d.hli);
	w.key("eot");
	if(!d.has_eot){
		w.null_val();
	}else{
		w.obj_begin();
		w.key("longitude_deg");
		w.value(d.eot.lon_deg);
		w.key("longitude_rad");
		w.value(d.eot.lon_rad);
		w.key("apparent_solar_time_rad");
		w.value(d.eot.apparent_solar_time_rad);
		w.key("mean_solar_time_rad");
		w.value(d.eot.mean_solar_time_rad);
		w.key("eot_rad");
		w.value(d.eot.eot_rad);
		w.key("eot_minutes");
		w.value(d.eot.eot_minutes);
		w.key("eot_seconds");
		w.value(d.eot.eot_seconds);
		w.obj_end();
	}
	w.key("near_ev");
	if(!d.inc_ev){
		w.null_val();
	}else{
		w.obj_begin();
		w.key("st_prev");
		wr_nslot(w,d.near_ev.solar_prev,eph);
		w.key("st_next");
		wr_nslot(w,d.near_ev.solar_next,eph);
		w.key("lp_prev");
		wr_nslot(w,d.near_ev.phase_prev,eph);
		w.key("lp_next");
		wr_nslot(w,d.near_ev.phase_next,eph);
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
	w.key("eot_lon_deg");
	if(d.has_eot){
		w.value(d.eot.lon_deg);
	}else{
		w.null_val();
	}
	w.key("hli_lon_deg");
	w.value(d.hli.lon_deg);
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
	if(d.has_eot){
		os<<"input.eot_lon_deg="<<format_num(d.eot.lon_deg)<<"\n";
	}
	os<<"data.sun_lam="<<format_num(d.lam_s)<<"\n";
	os<<"data.moon_lam="<<format_num(d.lam_m)<<"\n";
	os<<"data.elongation_rad="<<format_num(d.elong)<<"\n";
	os<<"data.elongation_deg="<<format_num(d.elong_deg)<<"\n";
	os<<"data.ill_frac="<<format_num(d.ill_frac)<<"\n";
	os<<"data.ill_pct="<<format_num(d.ill_pct)<<"\n";
	os<<"data.waxing="<<(d.waxing?"1":"0")<<"\n";
	os<<"data.phase_name="<<d.phase_name<<"\n";
	os<<"data.moon_xg.region="<<d.moon_xg.region<<"\n";
	os<<"data.moon_xg.star_id="<<d.moon_xg.star_id<<"\n";
	os<<"data.moon_xg.star_name="<<d.moon_xg.star_name<<"\n";
	os<<"data.moon_xg.sep_deg="<<format_num(d.moon_xg.sep_deg)<<"\n";
	os<<"data.lunar_year="<<d.lunar_date.lunar_year<<"\n";
	os<<"data.lun_mno="<<d.lunar_date.lun_mno<<"\n";
	os<<"data.lun_leap="<<(d.lunar_date.is_leap?"1":"0")<<"\n";
	os<<"data.lun_mlab="<<d.lunar_date.lun_mlab<<"\n";
	os<<"data.lunar_day="<<d.lunar_date.lunar_day<<"\n";
	os<<"data.lun_label="<<d.lunar_date.lun_label<<"\n";
	os<<"data.hli.y_lun="<<d.hli.y_lun.text<<"\n";
	os<<"data.hli.y_lchun="<<d.hli.y_lchun.text<<"\n";
	os<<"data.hli.month="<<d.hli.m_gz.text<<"\n";
	os<<"data.hli.day="<<d.hli.d_gz.text<<"\n";
	os<<"data.hli.hour_clock="<<d.hli.h_gz.text<<"\n";
	os<<"data.hli.hour_true="<<d.hli.h_gz_true.text<<"\n";
	os<<"data.hli.bazi_clock="<<d.hli.bazi_clock<<"\n";
	os<<"data.hli.bazi_true="<<d.hli.bazi_true<<"\n";
	os<<"data.hli.jianchu="<<d.hli.jianchu<<"\n";
	os<<"data.hli.duty_god="<<d.hli.duty_god<<"\n";
	os<<"data.hli.duty_tag="<<d.hli.duty_tag<<"\n";
	os<<"data.hli.clash="<<d.hli.clash<<"\n";
	os<<"data.hli.chong_sha="<<d.hli.chong_sha<<"\n";
	os<<"data.hli.zodiac_day="<<d.hli.zodiac_day<<"\n";
	os<<"data.hli.six_he="<<d.hli.six_he<<"\n";
	os<<"data.hli.three_he="<<d.hli.three_he<<"\n";
	os<<"data.hli.pengzu="<<d.hli.pengzu<<"\n";
	os<<"data.hli.nayin="<<d.hli.nayin<<"\n";
	os<<"data.hli.wuxing_day="<<d.hli.wx_day<<"\n";
	os<<"data.hli.fetal_god="<<d.hli.fetal_god<<"\n";
	os<<"data.hli.meridian="<<d.hli.meridian<<"\n";
	os<<"data.hli.lucky_dir="<<d.hli.lucky_dir<<"\n";
	os<<"data.hli.wealth_dir="<<d.hli.wealth_dir<<"\n";
	os<<"data.hli.mascot_dir="<<d.hli.mascot_dir<<"\n";
	os<<"data.hli.sun_noble_dir="<<d.hli.sun_noble_dir<<"\n";
	os<<"data.hli.moon_noble_dir="<<d.hli.moon_noble_dir<<"\n";
	os<<"data.hli.xiu28="<<d.hli.xiu28<<"\n";
	os<<"data.hli.xiu_star="<<d.hli.xiu_id<<"\n";
	os<<"data.hli.yi_ji_level="<<d.hli.yi_ji_level<<"\n";
	os<<"data.hli.yi_ji_rule="<<d.hli.yi_ji_rule<<"\n";
	os<<"data.hli.yi="<<join_pipe(d.hli.yi)<<"\n";
	os<<"data.hli.ji="<<join_pipe(d.hli.ji)<<"\n";
	os<<"data.hli.good_gods="<<join_pipe(d.hli.good_gods)<<"\n";
	os<<"data.hli.bad_gods="<<join_pipe(d.hli.bad_gods)<<"\n";
	os<<"data.hli.lon_deg="<<format_num(d.hli.lon_deg)<<"\n";
	os<<"data.hli.eot_min="<<format_num(d.hli.eot_min)<<"\n";
	os<<"data.hli.tst_min="<<format_num(d.hli.tst_min)<<"\n";
	if(d.has_eot){
		os<<"data.eot.longitude_deg="<<format_num(d.eot.lon_deg)<<"\n";
		os<<"data.eot.longitude_rad="<<format_num(d.eot.lon_rad)<<"\n";
		os<<"data.eot.apparent_solar_time_rad="
		  <<format_num(d.eot.apparent_solar_time_rad)<<"\n";
		os<<"data.eot.mean_solar_time_rad="
		  <<format_num(d.eot.mean_solar_time_rad)<<"\n";
		os<<"data.eot.eot_rad="<<format_num(d.eot.eot_rad)<<"\n";
		os<<"data.eot.eot_minutes="<<format_num(d.eot.eot_minutes)<<"\n";
		os<<"data.eot.eot_seconds="<<format_num(d.eot.eot_seconds)<<"\n";
	}
	if(d.inc_ev){
		os<<"[near_ev]\n";
		os<<"slot\tkind\tcode\tname\tjd_utc\ttm_uiso\ttm_liso\n";
		wr_etln(os,"st_prev",d.near_ev.solar_prev);
		wr_etln(os,"st_next",d.near_ev.solar_next);
		wr_etln(os,"lp_prev",d.near_ev.phase_prev);
		wr_etln(os,"lp_next",d.near_ev.phase_next);
	}
	os<<"[hour_jx]\n";
	os<<"slot\tgz\tluck\n";
	for(const auto&x : d.hli.hour_jx){
		os<<x.slot<<"\t"<<x.gz<<"\t"<<x.luck<<"\n";
	}
}

std::vector<EventRec> col_eyrs(EphRead&eph,const std::set<int>&years,int tz_off,
							   std::ostream*log,bool inc_eclipse=false){
	SolLunCal solver(eph);
	std::vector<EventRec> events;
	for(int y : years){
		YearResult yr=solver.compute_year(y,log);
		std::vector<EventRec> solar=bld_stev(yr,tz_off);
		std::vector<EventRec> phase=bld_lpev(yr,tz_off);
		events.insert(events.end(),solar.begin(),solar.end());
		events.insert(events.end(),phase.begin(),phase.end());
		if(inc_eclipse){
			std::vector<EventRec> lecl=bld_lunar_eclipse_events(eph,yr,tz_off);
			std::vector<EventRec> secl=bld_solar_eclipse_events(eph,yr,tz_off);
			events.insert(events.end(),lecl.begin(),lecl.end());
			events.insert(events.end(),secl.begin(),secl.end());
		}
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
	f.inc_lecl=false;
	f.inc_secl=false;
	f.inc_ecl=false;
	std::string token;
	std::istringstream iss(text);
	while(std::getline(iss,token,',')){
		token=to_low(token);
		if(token=="solar_term"||token=="solar-term"){
			f.inc_st=true;
		}else if(token=="lunar_phase"||token=="lunar-phase"){
			f.inc_lph=true;
		}else if(token=="lunar_eclipse"||token=="lunar-eclipse"||
				 token=="lunar-ecl"){
			f.inc_lecl=true;
		}else if(token=="solar_eclipse"||token=="solar-eclipse"||
				 token=="solar-ecl"){
			f.inc_secl=true;
		}else if(token=="eclipse"){
			f.inc_lecl=true;
			f.inc_secl=true;
		}else if(!token.empty()){
			throw std::invalid_argument("invalid kind: "+token);
		}
	}
	f.inc_ecl=f.inc_lecl||f.inc_secl;
	if(!f.inc_st&&!f.inc_lph&&!f.inc_ecl){
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
	if(ev.kind=="lunar_eclipse"){
		return f.inc_lecl;
	}
	if(ev.kind=="solar_eclipse"){
		return f.inc_secl;
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

