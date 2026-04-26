#include "src/common/exec.hpp"

#include<iterator>

// Event serializers, event collection, filters, and ICS output.
namespace{

struct YearEvtCtx{
	EphRead* eph=nullptr;
	SolLunCal solver;

	explicit YearEvtCtx(EphRead&src)
		: eph(&src),solver(src){}
};

std::vector<EventRec> bld_year_events(YearEvtCtx&ctx,int year,int tz_off,
									  const EvtFilt&filter){
	YearResult yr=ctx.solver.compute_year(year,nullptr);
	std::vector<EventRec> out;
	if(filter.inc_st){
		std::vector<EventRec> solar=bld_stev(yr,tz_off);
		out.insert(out.end(),solar.begin(),solar.end());
	}
	if(filter.inc_lph){
		std::vector<EventRec> phase=bld_lpev(yr,tz_off);
		out.insert(out.end(),phase.begin(),phase.end());
	}
	if(filter.inc_lecl){
		std::vector<EventRec> lecl=
			bld_lunar_eclipse_events(*ctx.eph,yr,tz_off);
		out.insert(out.end(),lecl.begin(),lecl.end());
	}
	if(filter.inc_secl){
		std::vector<EventRec> secl=
			bld_solar_eclipse_events(*ctx.eph,yr,tz_off);
		out.insert(out.end(),secl.begin(),secl.end());
	}
	return out;
}

EvtFilt mk_evt_flt(bool inc_eclipse){
	EvtFilt filter;
	filter.inc_ecl=inc_eclipse;
	filter.inc_lecl=inc_eclipse;
	filter.inc_secl=inc_eclipse;
	return filter;
}

std::set<int> add_years(std::set<int>&loaded,int year_lo,int year_hi){
	std::set<int> out;
	for(int year=year_lo;year<=year_hi;++year){
		if(loaded.insert(year).second){
			out.insert(year);
		}
	}
	return out;
}

void merge_evs(std::vector<EventRec>&dst,std::vector<EventRec> src){
	if(src.empty()){
		return;
	}
	const std::size_t split=dst.size();
	dst.insert(dst.end(),
			   std::make_move_iterator(src.begin()),
			   std::make_move_iterator(src.end()));
	std::inplace_merge(dst.begin(),dst.begin()+static_cast<std::ptrdiff_t>(split),
					   dst.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
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

std::vector<EventRec> col_eyrs(EphRead&eph,const std::set<int>&years,int tz_off,
							   std::ostream*log,const EvtFilt&filter){
	const std::vector<int> year_list(years.begin(),years.end());
	const bool show_progress=(log!=nullptr&&years.size()>1);
	if(show_progress){
		for(int y : year_list){
			log_year_progress(log,y);
		}
	}
	std::vector<std::vector<EventRec>> groups(year_list.size());
	lunar::exec::for_each_index(
		year_list.size(),0,
		[&](){ return YearEvtCtx(eph); },
		[&](YearEvtCtx&ctx,std::size_t idx){
			groups[idx]=
				bld_year_events(ctx,year_list[idx],tz_off,filter);
		});

	std::size_t total=0;
	for(const auto&group : groups){
		total+=group.size();
	}
	std::vector<EventRec> events;
	events.reserve(total);
	for(auto&group : groups){
		events.insert(events.end(),
					  std::make_move_iterator(group.begin()),
					  std::make_move_iterator(group.end()));
	}
	std::sort(events.begin(),events.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return events;
}

std::vector<EventRec> col_eyrs(EphRead&eph,const std::set<int>&years,int tz_off,
							   std::ostream*log,bool inc_eclipse=false){
	return col_eyrs(eph,years,tz_off,log,mk_evt_flt(inc_eclipse));
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

bool only_ecl_flt(const EvtFilt&f){
	return !f.inc_st&&(!f.inc_lph)&&f.inc_ecl;
}

std::string lun_ecl_code(const std::string&type){
	if(type=="T"){
		return "total";
	}
	if(type=="U"){
		return "partial";
	}
	return "penumbral";
}

std::string lun_ecl_name(const std::string&type){
	std::string fallback;
	if(type=="T"){
		fallback="月全食";
	}else if(type=="U"){
		fallback="月偏食";
	}else{
		fallback="半影月食";
	}
	return lunar::i18n::tr_event_name("lunar_eclipse",lun_ecl_code(type),
									  fallback);
}

std::string sol_ecl_code(const std::string&type){
	if(type=="T"){
		return "total";
	}
	if(type=="A"){
		return "annular";
	}
	if(type=="H"){
		return "hybrid";
	}
	return "partial";
}

std::string sol_ecl_name(const std::string&type){
	std::string fallback;
	if(type=="T"){
		fallback="日全食";
	}else if(type=="A"){
		fallback="日环食";
	}else if(type=="H"){
		fallback="全环食";
	}else{
		fallback="日偏食";
	}
	return lunar::i18n::tr_event_name("solar_eclipse",sol_ecl_code(type),
									  fallback);
}

EventRec mk_lun_ecl_ev(const LunarEclipse&ecl,int year,int tz_off){
	EventRec ev;
	ev.kind="lunar_eclipse";
	ev.code=lun_ecl_code(ecl.type);
	ev.name=lun_ecl_name(ecl.type);
	ev.year=year;
	ev.jd_tdb=ecl.jd_tdb_max;
	ev.jd_utc=TimeScale::tdb_to_utc(ecl.jd_tdb_max);
	ev.utc_iso=fmt_iso(ev.jd_utc,0,true);
	ev.loc_iso=fmt_iso(ev.jd_utc,tz_off,true);
	return ev;
}

EventRec mk_sol_ecl_ev(const SolarEclipse&ecl,int year,int tz_off){
	EventRec ev;
	ev.kind="solar_eclipse";
	ev.code=sol_ecl_code(ecl.type);
	ev.name=sol_ecl_name(ecl.type);
	ev.year=year;
	ev.jd_tdb=ecl.jd_tdb_max;
	ev.jd_utc=TimeScale::tdb_to_utc(ecl.jd_tdb_max);
	ev.utc_iso=fmt_iso(ev.jd_utc,0,true);
	ev.loc_iso=fmt_iso(ev.jd_utc,tz_off,true);
	return ev;
}

std::vector<EventRec> bld_next_ecl_year(YearEvtCtx&ctx,int year,double jd_from,
										int tz_off,const EvtFilt&filter,
										const std::string&code_filter){
	YearResult yr=ctx.solver.compute_year(year,nullptr);
	std::vector<EventRec> out;
	if(filter.inc_lecl){
		for(const auto&item : yr.lun_phase){
			double jd_utc=item.full_moon.toUtcJD();
			if(jd_utc<=jd_from){
				continue;
			}
			double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
			LunarEclipse ecl;
			if(!calc_lunar_eclipse(*ctx.eph,jd_tdb,&ecl)||!ecl.has){
				continue;
			}
			EventRec ev=mk_lun_ecl_ev(ecl,yr.year,tz_off);
			if(ev.jd_utc<=jd_from){
				continue;
			}
			if(!code_filter.empty()&&ev.code!=code_filter){
				continue;
			}
			out.push_back(std::move(ev));
		}
	}
	if(filter.inc_secl){
		for(const auto&item : yr.lun_phase){
			double jd_utc=item.new_moon.toUtcJD();
			if(jd_utc<=jd_from){
				continue;
			}
			double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
			SolarEclipse ecl;
			if(!calc_solar_eclipse(*ctx.eph,jd_tdb,&ecl)||!ecl.has){
				continue;
			}
			EventRec ev=mk_sol_ecl_ev(ecl,yr.year,tz_off);
			if(ev.jd_utc<=jd_from){
				continue;
			}
			if(!code_filter.empty()&&ev.code!=code_filter){
				continue;
			}
			out.push_back(std::move(ev));
		}
	}
	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	out.erase(
		std::unique(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
			return std::fabs(a.jd_utc-b.jd_utc)<1e-9;
		}),
		out.end());
	return out;
}

std::vector<EventRec> col_next_ecl(EphRead&eph,double jd_from,int year_from,
								   int count,int tz_off,std::ostream*log,
								   const EvtFilt&filter,
								   const std::string&code_filter){
	YearEvtCtx ctx(eph);
	std::vector<EventRec> out;
	for(int span=0;span<=8&&static_cast<int>(out.size())<count;++span){
		const int year=year_from+span;
		if(log!=nullptr&&count>1){
			log_year_progress(log,year);
		}
		merge_evs(out,bld_next_ecl_year(ctx,year,jd_from,tz_off,filter,
										code_filter));
	}
	if(static_cast<int>(out.size())>count){
		out.resize(static_cast<std::size_t>(count));
	}
	return out;
}

void wr_elics(std::ostream&os,const std::string&ephem,
			  const std::string&cal_name,const std::vector<EventRec>&events){
	std::vector<IcsEvent> out;
	out.reserve(events.size());
	for(const auto&ev : events){
		out.push_back(event_to_ics(ev,false));
	}
	write_ics(os,"lunar-cli//"+tool_ver(),cal_name,out,
			  {"schema="+tool_ver(),"ephem="+ephem,lunar::i18n::tz_note()});
}

}
