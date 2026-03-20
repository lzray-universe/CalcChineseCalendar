namespace lunar::core{

namespace{

bool core_day_is_leap_year(int year){
	return (year%4==0&&year%100!=0)||(year%400==0);
}

int core_day_days_in_month(int year,int month){
	static const std::array<int,12> kDays={
		31,28,31,30,31,30,31,31,30,31,30,31
	};
	if(month<1||month>12){
		throw std::invalid_argument("month must be in 1..12");
	}
	if(month==2&&core_day_is_leap_year(year)){
		return 29;
	}
	return kDays[static_cast<std::size_t>(month-1)];
}

GanzhiSummary core_day_pick_ganzhi(const AtData&atd,const HliRuleSet&rules){
	GanzhiSummary out;
	out.year=atd.hli.y_rule;
	out.month=atd.hli.m_gz;
	out.day=atd.hli.d_gz;
	out.hli_rules=normalize_hli_rule_set(rules);
	return out;
}

}

GanzhiSummary compute_ganzhi(const GanzhiComputeOptions&opt){
	DayComputeOptions day_opt;
	day_opt.ephem=opt.ephem;
	day_opt.date_text=opt.date_text;
	day_opt.at_time=opt.at_time;
	day_opt.tz=opt.tz;
	day_opt.include_events=false;
	day_opt.include_astro=false;
	day_opt.hli_rules=opt.hli_rules;
	DayResult day=compute_day(day_opt);
	return core_day_pick_ganzhi(day.at_data,opt.hli_rules);
}

GanzhiMonthSummary compute_ganzhi_month(const GanzhiMonthComputeOptions&opt){
	if(opt.year==0){
		throw std::invalid_argument("year must not be 0");
	}

	int tz_off=parse_tz(opt.tz);
	int hh=12;
	int mm=0;
	double ss=0.0;
	parse_hms(opt.at_time,hh,mm,ss);
	int day_count=core_day_days_in_month(opt.year,opt.month);

	EphRead eph(opt.ephem);
	QueryCache cache(eph);

	GanzhiMonthSummary out;
	out.year=opt.year;
	out.month=opt.month;
	out.at_time=opt.at_time;
	out.tz=opt.tz;
	out.hli_rules=normalize_hli_rule_set(opt.hli_rules);
	out.years.reserve(static_cast<std::size_t>(day_count));
	out.months.reserve(static_cast<std::size_t>(day_count));
	out.days.reserve(static_cast<std::size_t>(day_count));

	for(int day=1;day<=day_count;++day){
		double smp_jdutc=greg2jd(opt.year,opt.month,day,hh,mm,ss)-UTC8DAY;
		AtData atd=at_fromjd(eph,smp_jdutc,tz_off,opt.tz,
							 ymd_str(opt.year,opt.month,day)+"T"+opt.at_time,
							 opt.tz,false,false,0.0,120.0,&opt.hli_rules,&cache);
		out.years.push_back(atd.hli.y_rule);
		out.months.push_back(atd.hli.m_gz);
		out.days.push_back(atd.hli.d_gz);
	}

	return out;
}

DayResult compute_day(const DayComputeOptions&opt){
	int y=0;
	int m=0;
	int d=0;
	std::tie(y,m,d)=parse_ymd(opt.date_text);
	int hh=12;
	int mm=0;
	double ss=0.0;
	parse_hms(opt.at_time,hh,mm,ss);

	int tz_off=parse_tz(opt.tz);
	double smp_jdutc=greg2jd(y,m,d,hh,mm,ss)-UTC8DAY;
	double day_sutc=cst_midjd(y,m,d);
	double day_eutc=day_sutc+1.0;

	StarPick astro_pick;
	if(opt.include_astro){
		StarMode mode=parse_star_mode(opt.astro_mode_text);
		astro_pick=make_star_pick(mode,opt.astro_pick_csv);
	}
	AstroObs astro_obs;
	if(opt.has_astro_site){
		astro_obs.has_site=true;
		astro_obs.lat_deg=opt.astro_lat_deg;
		astro_obs.lon_deg=opt.astro_lon_deg;
		astro_obs.h_m=opt.astro_height_m;
	}

	EphRead eph(opt.ephem);
	QueryCache cache(eph);

	DayResult out;
	out.ephem=opt.ephem;
	out.date_text=opt.date_text;
	out.at_time=opt.at_time;
	out.tz=opt.tz;
	out.hli_lon_deg=opt.hli_lon_deg;
	out.inc_astro=opt.include_astro;
	out.astro_mode_text=opt.astro_mode_text;
	out.astro_pick_csv=opt.astro_pick_csv;
	out.astro_obs=astro_obs;
	out.at_data=at_fromjd(eph,smp_jdutc,tz_off,opt.tz,opt.date_text+"T"+opt.at_time,
						  "+08:00",false,false,0.0,opt.hli_lon_deg,&opt.hli_rules,
						  &cache);

	if(opt.include_events){
		std::set<int> years={y-1,y,y+1};
		std::vector<EventRec> all=
			col_eyrs(eph,years,tz_off,opt.quiet?nullptr:&std::cerr);
		for(const auto&ev : all){
			if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
				out.day_events.push_back(ev);
			}
		}
		std::sort(out.day_events.begin(),out.day_events.end(),
				  [](const EventRec&a,const EventRec&b){
					  return a.jd_utc<b.jd_utc;
				  });
	}

	if(opt.include_astro){
		std::vector<AstroEvt> raw=
			calc_astro_evt(eph,day_sutc,day_eutc,astro_pick,astro_obs);
		out.astro_events.reserve(raw.size());
		for(const auto&ev : raw){
			out.astro_events.push_back(mk_astro_rec(ev,tz_off));
		}
	}

	return out;
}

}
