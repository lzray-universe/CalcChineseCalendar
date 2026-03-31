// Lunar date conversion and nearby solar-term or phase lookup.
namespace{

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

NearEvents comp_near(EphRead&eph,double jd_utc,int tz_off,int lunar_day_tz_off,
					 QueryCache*cache=nullptr){
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2civil(jd_utc,lunar_day_tz_off,cst_year,cst_month,cst_day);

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

LunDate res_lun(EphRead&eph,double jd_utc,int lunar_day_tz_off,
				QueryCache*cache=nullptr){
	int cst_year=0;
	int cst_month=0;
	int cst_day=0;
	utc2civil(jd_utc,lunar_day_tz_off,cst_year,cst_month,cst_day);
	double qry_dutc=civil_midjd(cst_year,cst_month,cst_day,lunar_day_tz_off);

	LunCal6 local_calc(eph);
	LunCal6&calc=cache?cache->calc:local_calc;

	bool found=false;
	LunarMonth selected;
	double sel_sday=0.0;
	double sel_eday=0.0;
	for(int y : {cst_year,cst_year-1,cst_year+1}){
		const auto&months=calc.get_months(y);
		for(const auto&m : months){
			double start_day=civil_midjd(
				m.start_dt.year,m.start_dt.month,m.start_dt.day,lunar_day_tz_off);
			double end_day=civil_midjd(
				m.end_dt.year,m.end_dt.month,m.end_dt.day,lunar_day_tz_off);
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
			cny_sday=civil_midjd(
				m.start_dt.year,m.start_dt.month,m.start_dt.day,lunar_day_tz_off);
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
	info.lun_mlab=
		lunar::i18n::tr_lunar_month(selected.month_no,selected.is_leap,selected.label);
	info.lunar_day=lunar_day;
	info.cst_year=cst_year;
	info.cst_month=cst_month;
	info.cst_day=cst_day;
	info.cstday_jd=qry_dutc;

	info.lun_label=lunar::i18n::tr_lunar_label(
		lunar_year,selected.month_no,selected.is_leap,lunar_day);
	return info;
}

GregDate res_greg(EphRead&eph,int lunar_year,int month_no,int day,bool leap,
				  int lunar_day_tz_off,QueryCache*cache=nullptr){
	LunCal6 local_calc(eph);
	LunCal6&calc=cache?cache->calc:local_calc;
	auto find_cny=[&](int greg_year) -> double{
		const auto&months=calc.get_months(greg_year);
		for(const auto&m : months){
			if(m.month_no==1&&!m.is_leap){
				return civil_midjd(m.start_dt.year,m.start_dt.month,
								 m.start_dt.day,lunar_day_tz_off);
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
			double s=civil_midjd(
				m.start_dt.year,m.start_dt.month,m.start_dt.day,lunar_day_tz_off);
			if(s<cny_this||s>=cny_next){
				continue;
			}
			if(m.month_no==month_no&&m.is_leap==leap){
				start_day=s;
				end_day=civil_midjd(
					m.end_dt.year,m.end_dt.month,m.end_dt.day,lunar_day_tz_off);
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
	utc2civil(tgt_dutc,lunar_day_tz_off,gy,gm,gd);

	GregDate out;
	out.year=gy;
	out.month=gm;
	out.day=gd;
	out.cstday_jd=tgt_dutc;
	return out;
}

}
