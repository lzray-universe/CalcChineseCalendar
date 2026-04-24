#include<memory>

namespace{

enum class ExportHliMode{
	Off,
	One,
	All,
};

struct ExportSeed{
	int year=0;
	int month=0;
	int day=0;
	std::string date;
	double day_start_utc=0.0;
	double sample_jd_utc=0.0;
};

struct ExportDay{
	ExportSeed seed;
	AtData atd;
	std::vector<EventRec> events;
	std::vector<EventRec> eclipses;
	std::vector<EventRec> astro_events;
	std::vector<std::pair<std::string,HliData>> huangli_all;
};

struct ExportOpt{
	std::string ephem;
	std::string from_ym;
	std::string to_ym;
	std::string tz;
	std::string lunar_day_tz;
	std::string format;
	std::string out_path;
	std::string at_time="12:00:00";
	bool pretty=false;
	bool quiet=false;
	bool include_events=true;
	bool include_eclipse=false;
	bool include_astro=false;
	std::string astro_mode_text="less";
	std::string astro_pick_csv;
	AstroObs astro_obs;
	int jobs=0;
	double hli_lon_deg=120.0;
	ExportHliMode hli_mode=ExportHliMode::One;
	std::string hli_text="folk";
	HliRuleSet hli_rules=make_hli_rule_set(HliProfileCode::Folk);
	bool year_boundary_set=false;
	bool month_boundary_set=false;
	bool leap_month_mode_set=false;
	bool day_boundary_set=false;
};

struct ExportRes{
	int tz_off=0;
	int lunar_day_tz_off=0;
	std::vector<ExportDay> days;
	StarPick astro_pick;
};

int ym_index(int year,int month){
	return year*12+(month-1);
}

std::string ym_text(int year,int month){
	std::ostringstream oss;
	oss<<std::setfill('0')<<std::setw(4)<<year<<"-"<<std::setw(2)<<month;
	return oss.str();
}

std::pair<int,int> ym_from_index(int idx){
	int year=idx/12;
	int month=idx%12+1;
	if(month<=0){
		month+=12;
		--year;
	}
	return {year,month};
}

std::vector<ExportSeed> build_export_seeds(const ExportOpt&opt,
										   int lunar_day_tz_off){
	int from_y=0;
	int from_m=0;
	int to_y=0;
	int to_m=0;
	std::tie(from_y,from_m)=parse_ym(opt.from_ym);
	std::tie(to_y,to_m)=parse_ym(opt.to_ym);
	const int from_idx=ym_index(from_y,from_m);
	const int to_idx=ym_index(to_y,to_m);
	if(to_idx<from_idx){
		throw std::invalid_argument("--to must be >= --from");
	}

	int hh=12;
	int mm=0;
	double ss=0.0;
	parse_hms(opt.at_time,hh,mm,ss);
	const double sample_off=
		(static_cast<double>(hh*3600+mm*60)+ss)/86400.0;

	std::vector<ExportSeed> seeds;
	for(int idx=from_idx;idx<=to_idx;++idx){
		int y=0;
		int m=0;
		std::tie(y,m)=ym_from_index(idx);
		const int day_count=days_gm(y,m);
		for(int d=1;d<=day_count;++d){
			ExportSeed seed;
			seed.year=y;
			seed.month=m;
			seed.day=d;
			seed.date=ymd_str(y,m,d);
			seed.day_start_utc=civil_midjd(y,m,d,lunar_day_tz_off);
			seed.sample_jd_utc=seed.day_start_utc+sample_off;
			seeds.push_back(std::move(seed));
		}
	}
	return seeds;
}

std::string hli_mode_text(const ExportOpt&opt){
	if(opt.hli_mode==ExportHliMode::Off){
		return "off";
	}
	if(opt.hli_mode==ExportHliMode::All){
		return "all";
	}
	return opt.hli_text;
}

void apply_hli_overrides(HliRuleSet&rules,const ExportOpt&opt){
	if(opt.year_boundary_set){
		rules.year_boundary=opt.hli_rules.year_boundary;
	}
	if(opt.month_boundary_set){
		rules.month_boundary=opt.hli_rules.month_boundary;
	}
	if(opt.leap_month_mode_set){
		rules.leap_month_mode=opt.hli_rules.leap_month_mode;
	}
	if(opt.day_boundary_set){
		rules.day_boundary=opt.hli_rules.day_boundary;
	}
	rules=normalize_hli_rule_set(rules);
}

std::vector<std::pair<std::string,HliRuleSet>>
export_all_hli_rules(const ExportOpt&opt){
	const std::array<HliProfileCode,4> profiles={
		HliProfileCode::Folk,
		HliProfileCode::ZiPing,
		HliProfileCode::PurpleStar,
		HliProfileCode::XieJi,
	};
	std::vector<std::pair<std::string,HliRuleSet>> out;
	out.reserve(profiles.size());
	for(HliProfileCode profile : profiles){
		HliRuleSet rules=make_hli_rule_set(profile);
		apply_hli_overrides(rules,opt);
		out.push_back({hli_profile_key(profile),rules});
	}
	return out;
}

void assign_event_to_day(const std::vector<ExportSeed>&seeds,const EventRec&ev,
						 std::vector<std::vector<EventRec>>&slots){
	if(seeds.empty()){
		return;
	}
	const double begin=seeds.front().day_start_utc;
	const double end=seeds.back().day_start_utc+1.0;
	if(ev.jd_utc<begin||ev.jd_utc>=end){
		return;
	}
	int idx=static_cast<int>(std::floor((ev.jd_utc-begin)+1e-10));
	if(idx<0||static_cast<std::size_t>(idx)>=slots.size()){
		return;
	}
	slots[static_cast<std::size_t>(idx)].push_back(ev);
}

std::string join_event_names(const std::vector<EventRec>&events){
	std::vector<std::string> names;
	names.reserve(events.size());
	for(const auto&ev : events){
		names.push_back(ev.name);
	}
	return join_pipe(names);
}

std::string join_event_times(const std::vector<EventRec>&events){
	std::vector<std::string> times;
	times.reserve(events.size());
	for(const auto&ev : events){
		times.push_back(ev.loc_iso);
	}
	return join_pipe(times);
}

EvtFilt export_event_filter(const ExportOpt&opt){
	EvtFilt filter;
	filter.inc_st=opt.include_events;
	filter.inc_lph=opt.include_events;
	filter.inc_lecl=opt.include_eclipse;
	filter.inc_secl=opt.include_eclipse;
	filter.inc_ecl=opt.include_eclipse;
	return filter;
}

struct ExportWorkerCtx{
	const ExportOpt*opt=nullptr;
	int tz_off=0;
	int lunar_day_tz_off=0;
	EphRead eph;
	QueryCache cache;
	std::vector<std::pair<std::string,HliRuleSet>> all_hli_rules;

	ExportWorkerCtx(const ExportOpt&src,int tz,int lunar_day_tz):
		opt(&src),
		tz_off(tz),
		lunar_day_tz_off(lunar_day_tz),
		eph(src.ephem),
		cache(eph),
		all_hli_rules(export_all_hli_rules(src)){}
};

void compute_export_day(ExportWorkerCtx&ctx,ExportDay&day){
	const ExportOpt&opt=*ctx.opt;
	day.atd=at_fromjd(ctx.eph,day.seed.sample_jd_utc,ctx.tz_off,
					  ctx.lunar_day_tz_off,opt.tz,
					  day.seed.date+"T"+opt.at_time,opt.lunar_day_tz,
					  false,false,0.0,opt.hli_lon_deg,&opt.hli_rules,
					  &ctx.cache);
	if(opt.hli_mode==ExportHliMode::All){
		day.huangli_all.clear();
		day.huangli_all.reserve(ctx.all_hli_rules.size());
		for(const auto&item : ctx.all_hli_rules){
			AtData hli_at=at_fromjd(ctx.eph,day.seed.sample_jd_utc,
									ctx.tz_off,ctx.lunar_day_tz_off,opt.tz,
									day.seed.date+"T"+opt.at_time,
									opt.lunar_day_tz,false,false,0.0,
									opt.hli_lon_deg,&item.second,&ctx.cache);
			day.huangli_all.push_back({item.first,std::move(hli_at.hli)});
		}
	}
}

ExportOpt parse_export(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument(
			"export requires: <bsp> <YYYY-MM> or --from <YYYY-MM> --to <YYYY-MM>");
	}
	InterCfg cfg=load_def();
	ExportOpt opt;
	opt.ephem=args[0];
	opt.tz=cfg.default_tz;
	opt.lunar_day_tz=resolve_lunar_day_tz(cfg);
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"&&
	   opt.format!="jsonl"){
		opt.format="jsonl";
	}
	opt.pretty=cfg.def_prety;
	opt.hli_rules=hli_rules_from_cfg(cfg);
	opt.hli_text=hli_profile_key(
		static_cast<HliProfileCode>(opt.hli_rules.profile_code));

	double astro_lat=0.0;
	double astro_lon=0.0;
	double astro_h=0.0;
	bool has_lat=false;
	bool has_lon=false;
	bool has_h=false;
	std::string from_year;
	std::string to_year;
	auto set_full_scope=[&](){
		opt.include_events=true;
		opt.include_eclipse=true;
		opt.include_astro=true;
		opt.hli_mode=ExportHliMode::All;
		opt.hli_text="all";
		HliRuleSet rules=make_hli_rule_set(HliProfileCode::Folk);
		apply_hli_overrides(rules,opt);
		opt.hli_rules=rules;
	};
	auto set_basic_scope=[&](){
		opt.include_events=true;
		opt.include_eclipse=false;
		opt.include_astro=false;
		opt.hli_mode=ExportHliMode::One;
		HliProfileCode profile=HliProfileCode::Folk;
		(void)parse_hli_profile(cfg.hli_trad,&profile);
		opt.hli_text=hli_profile_key(profile);
		HliRuleSet rules=make_hli_rule_set(profile);
		apply_hli_overrides(rules,opt);
		opt.hli_rules=rules;
	};

	lunar::ArgParser parser;
	parser.add_value("--from",[&](const std::string&v){ opt.from_ym=v; })
		.add_value("--to",[&](const std::string&v){ opt.to_ym=v; })
		.add_value("--from-year",[&](const std::string&v){ from_year=v; })
		.add_value("--to-year",[&](const std::string&v){ to_year=v; })
		.add_value("--tz",[&](const std::string&v){ opt.tz=v; })
		.add_value("--lunar-day-tz",
				   [&](const std::string&v){ opt.lunar_day_tz=v; })
		.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; })
		.add_value("--at",[&](const std::string&v){ opt.at_time=v; })
		.add_value("--jobs",[&](const std::string&v){
			opt.jobs=parse_int(v,"--jobs");
			if(opt.jobs<1){
				throw std::invalid_argument("--jobs must be >=1");
			}
		})
		.add_value("--events",[&](const std::string&v){
			opt.include_events=parse_bool01(v,"--events");
		})
		.add_value("--eclipse",[&](const std::string&v){
			opt.include_eclipse=parse_bool01(v,"--eclipse");
		})
		.add_value("--scope",[&](const std::string&v){
			std::string low=to_low(v);
			if(low=="full"){
				set_full_scope();
			}else if(low=="basic"){
				set_basic_scope();
			}else{
				throw std::invalid_argument("--scope must be basic or full");
			}
		})
		.add_value("--full",[&](const std::string&v){
			if(parse_bool01(v,"--full")){
				set_full_scope();
			}else{
				set_basic_scope();
			}
		})
		.add_value("--huangli",[&](const std::string&v){
			std::string low=to_low(v);
			opt.hli_text=low;
			if(low=="off"||low=="none"||low=="0"){
				opt.hli_mode=ExportHliMode::Off;
				return;
			}
			if(low=="all"){
				opt.hli_mode=ExportHliMode::All;
				HliRuleSet rules=make_hli_rule_set(HliProfileCode::Folk);
				apply_hli_overrides(rules,opt);
				opt.hli_rules=rules;
				return;
			}
			HliProfileCode profile=HliProfileCode::Folk;
			if(!parse_hli_profile(low,&profile)){
				throw std::invalid_argument(
					"invalid --huangli: "+v+
					" (expected off|folk|ziping|purple|xieji|all)");
			}
			opt.hli_mode=ExportHliMode::One;
			opt.hli_text=hli_profile_key(profile);
			HliRuleSet rules=make_hli_rule_set(profile);
			apply_hli_overrides(rules,opt);
			opt.hli_rules=rules;
		})
		.add_value("--trad",[&](const std::string&v){
			std::string low=to_low(v);
			if(low=="all"){
				opt.hli_mode=ExportHliMode::All;
				opt.hli_text="all";
				HliRuleSet rules=make_hli_rule_set(HliProfileCode::Folk);
				apply_hli_overrides(rules,opt);
				opt.hli_rules=rules;
				return;
			}
			HliProfileCode profile=HliProfileCode::Folk;
			if(!parse_hli_profile(low,&profile)){
				throw std::invalid_argument(
					"invalid --trad: "+v+" (expected folk|ziping|purple|xieji|all)");
			}
			opt.hli_mode=ExportHliMode::One;
			opt.hli_text=hli_profile_key(profile);
			HliRuleSet rules=make_hli_rule_set(profile);
			apply_hli_overrides(rules,opt);
			opt.hli_rules=rules;
		})
		.add_value("--lon",[&](const std::string&v){
			opt.hli_lon_deg=parse_double(v,"--lon");
		})
		.add_value("--year-boundary",[&](const std::string&v){
			HliYearBoundary parsed=HliYearBoundary::LunarNewYear;
			if(!parse_hli_year_boundary(v,&parsed)){
				throw std::invalid_argument(
					"invalid --year-boundary: "+v+
					" (expected lichun|lunar_new_year|dongzhi)");
			}
			opt.hli_rules.year_boundary=static_cast<int>(parsed);
			opt.year_boundary_set=true;
		})
		.add_value("--month-boundary",[&](const std::string&v){
			HliMonthBoundary parsed=HliMonthBoundary::LunarFirstDay;
			if(!parse_hli_month_boundary(v,&parsed)){
				throw std::invalid_argument(
					"invalid --month-boundary: "+v+
					" (expected solar_term|lunar_first_day)");
			}
			opt.hli_rules.month_boundary=static_cast<int>(parsed);
			opt.month_boundary_set=true;
		})
		.add_value("--leap-month-mode",[&](const std::string&v){
			HliLeapMonthMode parsed=HliLeapMonthMode::InheritPrevious;
			if(!parse_hli_leap_month_mode(v,&parsed)){
				throw std::invalid_argument(
					"invalid --leap-month-mode: "+v+
					" (expected ignore|inherit_previous|split_midway|shift_to_next)");
			}
			opt.hli_rules.leap_month_mode=static_cast<int>(parsed);
			opt.leap_month_mode_set=true;
		})
		.add_value("--day-boundary",[&](const std::string&v){
			HliDayBoundary parsed=HliDayBoundary::Hour23;
			if(!parse_hli_day_boundary(v,&parsed)){
				throw std::invalid_argument(
					"invalid --day-boundary: "+v+" (expected hour23|hour0)");
			}
			opt.hli_rules.day_boundary=static_cast<int>(parsed);
			opt.day_boundary_set=true;
		})
		.add_value("--astro",[&](const std::string&v){
			opt.include_astro=parse_bool01(v,"--astro");
		})
		.add_value("--astro-mode",
				   [&](const std::string&v){ opt.astro_mode_text=v; })
		.add_value("--astro-pick",
				   [&](const std::string&v){ opt.astro_pick_csv=v; })
		.add_value("--astro-lat",[&](const std::string&v){
			astro_lat=parse_double(v,"--astro-lat");
			has_lat=true;
		})
		.add_value("--astro-lon",[&](const std::string&v){
			astro_lon=parse_double(v,"--astro-lon");
			has_lon=true;
		})
		.add_value("--astro-height",[&](const std::string&v){
			astro_h=parse_double(v,"--astro-height");
			has_h=true;
		});

	bool used_pos_month=false;
	for(std::size_t i=1;i<args.size();++i){
		if(args[i]=="-h"||args[i]=="--help"){
			use_export();
			opt.format.clear();
			return opt;
		}
		if(parser.parse_one(args,i,"export")){
			continue;
		}
		if(!used_pos_month&&opt.from_ym.empty()){
			opt.from_ym=args[i];
			opt.to_ym=args[i];
			used_pos_month=true;
			continue;
		}
		throw std::invalid_argument("unknown option for export: "+args[i]);
	}

	if(!from_year.empty()||!to_year.empty()){
		if(from_year.empty()){
			from_year=to_year;
		}
		if(to_year.empty()){
			to_year=from_year;
		}
		int fy=parse_int(from_year,"--from-year");
		int ty=parse_int(to_year,"--to-year");
		opt.from_ym=ym_text(fy,1);
		opt.to_ym=ym_text(ty,12);
	}
	if(opt.from_ym.empty()){
		throw std::invalid_argument(
			"export requires <YYYY-MM>, --from <YYYY-MM>, or --from-year <year>");
	}
	if(opt.to_ym.empty()){
		opt.to_ym=opt.from_ym;
	}
	if(has_lat!=has_lon){
		throw std::invalid_argument(
			"astro site requires both --astro-lat and --astro-lon");
	}
	if(has_h&&!has_lat){
		throw std::invalid_argument(
			"--astro-height requires --astro-lat and --astro-lon");
	}
	if(has_lat){
		opt.astro_obs.has_site=true;
		opt.astro_obs.lat_deg=astro_lat;
		opt.astro_obs.lon_deg=astro_lon;
		opt.astro_obs.h_m=has_h?astro_h:0.0;
	}
	chk_fmt(opt.format,{"json","jsonl","csv","txt"},"export");
	opt.lunar_day_tz=canonical_tz_text(opt.lunar_day_tz);
	opt.hli_rules=normalize_hli_rule_set(opt.hli_rules);
	(void)parse_ym(opt.from_ym);
	(void)parse_ym(opt.to_ym);
	int hh=0;
	int mm=0;
	double ss=0.0;
	parse_hms(opt.at_time,hh,mm,ss);
	return opt;
}

ExportRes run_export(const ExportOpt&opt){
	ExportRes res;
	res.tz_off=parse_tz(opt.tz);
	res.lunar_day_tz_off=parse_tz(opt.lunar_day_tz);
	std::vector<ExportSeed> seeds=build_export_seeds(opt,res.lunar_day_tz_off);
	res.days.resize(seeds.size());
	for(std::size_t i=0;i<seeds.size();++i){
		res.days[i].seed=std::move(seeds[i]);
	}
	if(res.days.empty()){
		return res;
	}

	std::vector<std::vector<EventRec>> day_events(res.days.size());
	std::vector<std::vector<EventRec>> day_eclipses(res.days.size());
	if(opt.include_events||opt.include_eclipse){
		EphRead eph(opt.ephem);
		std::set<int> years;
		years.insert(res.days.front().seed.year-1);
		for(const auto&day : res.days){
			years.insert(day.seed.year);
		}
		years.insert(res.days.back().seed.year+1);
		std::vector<EventRec> events=
			col_eyrs(eph,years,res.tz_off,opt.quiet?nullptr:&std::cerr,
					 export_event_filter(opt));
		std::vector<ExportSeed> seed_view;
		seed_view.reserve(res.days.size());
		for(const auto&day : res.days){
			seed_view.push_back(day.seed);
		}
		for(const auto&ev : events){
			std::vector<std::vector<EventRec>>&slots=
				(ev.kind=="lunar_eclipse"||ev.kind=="solar_eclipse")
					?day_eclipses
					:day_events;
			assign_event_to_day(seed_view,ev,slots);
		}
	}

	std::vector<std::vector<EventRec>> day_astro(res.days.size());
	if(opt.include_astro){
		StarMode mode=parse_star_mode(opt.astro_mode_text);
		res.astro_pick=make_star_pick(mode,opt.astro_pick_csv);
		EphRead eph(opt.ephem);
		const double begin=res.days.front().seed.day_start_utc;
		const double end=res.days.back().seed.day_start_utc+1.0;
		std::vector<AstroEvt> raw=
			calc_astro_evt(eph,begin,end,res.astro_pick,opt.astro_obs);
		std::vector<ExportSeed> seed_view;
		seed_view.reserve(res.days.size());
		for(const auto&day : res.days){
			seed_view.push_back(day.seed);
		}
		for(const auto&ev : raw){
			assign_event_to_day(seed_view,mk_astro_rec(ev,res.tz_off),day_astro);
		}
	}

	for(std::size_t i=0;i<res.days.size();++i){
		res.days[i].events=std::move(day_events[i]);
		res.days[i].eclipses=std::move(day_eclipses[i]);
		res.days[i].astro_events=std::move(day_astro[i]);
	}

	lunar::exec::for_each_index(
		res.days.size(),static_cast<std::size_t>(opt.jobs),
		[&](){
			return std::make_unique<ExportWorkerCtx>(
				opt,res.tz_off,res.lunar_day_tz_off);
		},
		[&](std::unique_ptr<ExportWorkerCtx>&ctx,std::size_t idx){
			compute_export_day(*ctx,res.days[idx]);
		});

	return res;
}

void write_export_input(JsonWriter&w,const ExportOpt&opt,const ExportRes&res){
	w.key("input");
	w.obj_begin();
	w.key("from_month");
	w.value(opt.from_ym);
	w.key("to_month");
	w.value(opt.to_ym);
	w.key("day_count");
	w.value(static_cast<int>(res.days.size()));
	w.key("smp_time");
	w.value(opt.at_time);
	w.key("lunar_day_tz");
	w.value(opt.lunar_day_tz);
	w.key("jobs");
	w.value(opt.jobs);
	w.key("events");
	w.value(opt.include_events);
	w.key("eclipse");
	w.value(opt.include_eclipse);
	w.key("astro");
	w.value(opt.include_astro);
	w.key("astro_mode");
	if(opt.include_astro){
		w.value(opt.astro_mode_text);
	}else{
		w.null_val();
	}
	w.key("astro_pick");
	if(opt.include_astro&&to_low(opt.astro_mode_text)=="pick"){
		w.value(opt.astro_pick_csv);
	}else{
		w.null_val();
	}
	w.key("astro_site");
	w.value(opt.astro_obs.has_site);
	w.key("astro_lat_deg");
	if(opt.astro_obs.has_site){
		w.value(opt.astro_obs.lat_deg);
	}else{
		w.null_val();
	}
	w.key("astro_lon_deg");
	if(opt.astro_obs.has_site){
		w.value(opt.astro_obs.lon_deg);
	}else{
		w.null_val();
	}
	w.key("astro_height_m");
	if(opt.astro_obs.has_site){
		w.value(opt.astro_obs.h_m);
	}else{
		w.null_val();
	}
	w.key("huangli");
	w.value(hli_mode_text(opt));
	w.key("lon_deg");
	w.value(opt.hli_lon_deg);
	w.obj_end();
}

void write_export_day_json(JsonWriter&w,const ExportOpt&opt,
						   const ExportDay&day,EphRead&eph,int tz_off){
	const AtData&atd=day.atd;
	w.obj_begin();
	w.key("greg_date");
	w.value(day.seed.date);
	w.key("greg_year");
	w.value(day.seed.year);
	w.key("greg_month");
	w.value(day.seed.month);
	w.key("greg_day");
	w.value(day.seed.day);
	w.key("sample");
	w.obj_begin();
	w.key("jd_utc");
	w.value(atd.jd_utc);
	w.key("jd_tdb");
	w.value(atd.jd_tdb);
	w.key("utc_iso");
	w.value(atd.utc_iso);
	w.key("loc_iso");
	w.value(atd.local_iso);
	w.obj_end();
	w.key("lunar_date");
	wr_ljson(w,atd.lunar_date);
	w.key("ganzhi");
	w.obj_begin();
	w.key("year");
	wr_gz_json(w,atd.hli.y_rule);
	w.key("month");
	wr_gz_json(w,atd.hli.m_gz);
	w.key("day");
	wr_gz_json(w,atd.hli.d_gz);
	w.obj_end();
	w.key("moon");
	w.obj_begin();
	w.key("elongation_deg");
	w.value(atd.elong_deg);
	w.key("ill_frac");
	w.value(atd.ill_frac);
	w.key("ill_pct");
	w.value(atd.ill_pct);
	w.key("waxing");
	w.value(atd.waxing);
	w.key("phase_name");
	w.value(atd.phase_name);
	w.key("xg");
	w.obj_begin();
	w.key("region");
	w.value(atd.moon_xg.region);
	w.key("star_id");
	w.value(atd.moon_xg.star_id);
	w.key("star_name");
	w.value(atd.moon_xg.star_name);
	w.key("sep_deg");
	wr_num_or_null(w,atd.moon_xg.sep_deg);
	w.obj_end();
	w.obj_end();
	w.key("events");
	w.arr_begin();
	for(const auto&ev : day.events){
		wr_ejson(w,ev,eph,false,tz_off);
	}
	w.arr_end();
	w.key("eclipses");
	w.arr_begin();
	for(const auto&ev : day.eclipses){
		wr_ejson(w,ev,eph,true,tz_off);
	}
	w.arr_end();
	w.key("astro_events");
	w.arr_begin();
	for(const auto&ev : day.astro_events){
		wr_ejson(w,ev,eph,false,tz_off);
	}
	w.arr_end();
	w.key("huangli");
	if(opt.hli_mode==ExportHliMode::Off){
		w.null_val();
	}else if(opt.hli_mode==ExportHliMode::All){
		w.obj_begin();
		for(const auto&item : day.huangli_all){
			w.key(item.first);
			wr_hli_json(w,item.second);
		}
		w.obj_end();
	}else{
		wr_hli_json(w,atd.hli);
	}
	w.obj_end();
}

void write_export_json(std::ostream&os,const ExportOpt&opt,const ExportRes&res,
					   bool jsonl){
	EphRead eph(opt.ephem);
	if(jsonl){
		for(const auto&day : res.days){
			JsonWriter w(os,false);
			write_export_day_json(w,opt,day,eph,res.tz_off);
			os<<"\n";
		}
		return;
	}
	JsonWriter w(os,opt.pretty);
	w.obj_begin();
	write_meta(w,opt.ephem,opt.tz,
			   {"type=export",lunar_day_rule_note(opt.lunar_day_tz)});
	write_export_input(w,opt,res);
	w.key("data");
	w.arr_begin();
	for(const auto&day : res.days){
		write_export_day_json(w,opt,day,eph,res.tz_off);
	}
	w.arr_end();
	w.obj_end();
	os<<"\n";
}

void write_export_csv(std::ostream&os,const ExportOpt&opt,const ExportRes&res){
	CsvWriter csv(os);
	for(const auto&day : res.days){
		const AtData&atd=day.atd;
		csv.write_field("greg_date",day.seed.date);
		csv.write_field("greg_year",day.seed.year);
		csv.write_field("greg_month",day.seed.month);
		csv.write_field("greg_day",day.seed.day);
		csv.write_field("lunar_year",atd.lunar_date.lunar_year);
		csv.write_field("lun_mno",atd.lunar_date.lun_mno);
		csv.write_field("lun_leap",atd.lunar_date.is_leap);
		csv.write_field("lun_mlab",atd.lunar_date.lun_mlab);
		csv.write_field("lunar_day",atd.lunar_date.lunar_day);
		csv.write_field("lun_label",atd.lunar_date.lun_label);
		csv.write_field("gz_year",atd.hli.y_rule.text);
		csv.write_field("gz_month",atd.hli.m_gz.text);
		csv.write_field("gz_day",atd.hli.d_gz.text);
		csv.write_raw("ill_pct",format_num(atd.ill_pct));
		csv.write_field("phase_name",atd.phase_name);
		csv.write_field("moon_xg_region",atd.moon_xg.region);
		csv.write_field("moon_xg_star",atd.moon_xg.star_name);
		csv.write_raw("moon_xg_sep_deg",format_num(atd.moon_xg.sep_deg));
		csv.write_field("smp_liso",atd.local_iso);
		csv.write_field("events",join_event_names(day.events));
		csv.write_field("event_times",join_event_times(day.events));
		csv.write_field("eclipses",join_event_names(day.eclipses));
		csv.write_field("eclipse_times",join_event_times(day.eclipses));
		csv.write_field("astro_events",join_event_names(day.astro_events));
		csv.write_field("astro_event_times",join_event_times(day.astro_events));
		csv.write_field("huangli_mode",hli_mode_text(opt));
		if(opt.hli_mode==ExportHliMode::All){
			std::vector<std::string> yi;
			std::vector<std::string> ji;
			for(const auto&item : day.huangli_all){
				yi.push_back(item.first+":"+join_pipe(item.second.yi));
				ji.push_back(item.first+":"+join_pipe(item.second.ji));
			}
			csv.write_field("huangli_yi",join_pipe(yi));
			csv.write_field("huangli_ji",join_pipe(ji));
		}else if(opt.hli_mode==ExportHliMode::One){
			wr_hli_csv(csv,atd.hli,HliCsvLayout::Day);
		}
		csv.finish_row();
	}
}

void write_export_txt(std::ostream&os,const ExportOpt&opt,const ExportRes&res){
	os<<"tool=lunar format=txt type=export tz_display="<<opt.tz<<"\n";
	os<<"input.from_month="<<opt.from_ym<<"\n";
	os<<"input.to_month="<<opt.to_ym<<"\n";
	os<<"input.day_count="<<res.days.size()<<"\n";
	os<<"input.smp_time="<<opt.at_time<<"\n";
	os<<"input.lunar_day_tz="<<opt.lunar_day_tz<<"\n";
	os<<"input.events="<<(opt.include_events?"1":"0")<<"\n";
	os<<"input.eclipse="<<(opt.include_eclipse?"1":"0")<<"\n";
	os<<"input.astro="<<(opt.include_astro?"1":"0")<<"\n";
	os<<"input.astro_mode="<<opt.astro_mode_text<<"\n";
	os<<"input.huangli="<<hli_mode_text(opt)<<"\n";
	for(const auto&day : res.days){
		const AtData&atd=day.atd;
		os<<"[day "<<day.seed.date<<"]\n";
		os<<"greg_date="<<day.seed.date<<"\n";
		os<<"lun_label="<<atd.lunar_date.lun_label<<"\n";
		os<<"gz_year="<<atd.hli.y_rule.text<<"\n";
		os<<"gz_month="<<atd.hli.m_gz.text<<"\n";
		os<<"gz_day="<<atd.hli.d_gz.text<<"\n";
		os<<"ill_pct="<<format_num(atd.ill_pct)<<"\n";
		os<<"phase_name="<<atd.phase_name<<"\n";
		os<<"sample_liso="<<atd.local_iso<<"\n";
		os<<"events="<<join_event_names(day.events)<<"\n";
		os<<"event_times="<<join_event_times(day.events)<<"\n";
		os<<"eclipses="<<join_event_names(day.eclipses)<<"\n";
		os<<"eclipse_times="<<join_event_times(day.eclipses)<<"\n";
		os<<"astro_events="<<join_event_names(day.astro_events)<<"\n";
		if(opt.hli_mode==ExportHliMode::One){
			wr_hli_txt(os,atd.hli,HliTxtLayout::Day);
		}else if(opt.hli_mode==ExportHliMode::All){
			for(const auto&item : day.huangli_all){
				os<<"huangli."<<item.first<<".yi="<<join_pipe(item.second.yi)<<"\n";
				os<<"huangli."<<item.first<<".ji="<<join_pipe(item.second.ji)<<"\n";
			}
		}
	}
}

void write_export(std::ostream&os,const ExportOpt&opt,const ExportRes&res){
	const FmtMap fmts={
		{"json",[&](){ write_export_json(os,opt,res,false); }},
		{"jsonl",[&](){ write_export_json(os,opt,res,true); }},
		{"csv",[&](){ write_export_csv(os,opt,res); }},
		{"txt",[&](){ write_export_txt(os,opt,res); }},
	};
	run_fmt(fmts,opt.format,"export");
}

}

int cmd_export(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_export();
		return 0;
	}
	ExportOpt opt=parse_export(args);
	if(opt.format.empty()){
		return 0;
	}
	ExportRes res=run_export(opt);
	OutTgt out=open_out(opt.out_path);
	write_export(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}
