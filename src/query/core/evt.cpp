// Event serializers, event collection, filters, and ICS output.
namespace{

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
							   std::ostream*log,bool inc_eclipse=false){
	SolLunCal solver(eph);
	std::vector<EventRec> events;
	const bool show_progress=(log!=nullptr&&years.size()>1);
	for(int y : years){
		if(show_progress){
			log_year_progress(log,y);
		}
		YearResult yr=solver.compute_year(y,nullptr);
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

void wr_elics(std::ostream&os,const std::string&ephem,
			  const std::string&cal_name,const std::vector<EventRec>&events){
	std::vector<IcsEvent> out;
	out.reserve(events.size());
	for(const auto&ev : events){
		out.push_back(event_to_ics(ev,false));
	}
	write_ics(os,"lunar-cli//"+tool_ver(),cal_name,out,
			  {"schema=lunar.v1","ephem="+ephem,lunar::i18n::tz_note()});
}

}
