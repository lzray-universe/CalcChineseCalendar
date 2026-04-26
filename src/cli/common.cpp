namespace{

struct MonYrData{
	int year=0;
	std::string mode;
	std::vector<MonthRec> months;
	std::vector<LunarEclipse> eclipses;
	bool inc_eclipse=false;
};

struct CalYrData{
	int year=0;
	std::string mode;
	std::vector<EventRec> sol_terms;
	std::vector<EventRec> lun_phase;
	std::vector<MonthRec> months;
	bool inc_month=false;
	std::vector<LunarEclipse> eclipses;
	bool inc_eclipse=false;
};

using cli_util::OutTgt;
using cli_util::bld_lpev;
using cli_util::bld_stev;
using cli_util::chk_fmt;
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

MonthRec mk_mrec(const LunarMonth&m,int tz_off){
	MonthRec rec;
	rec.label=lunar::i18n::tr_lunar_month(m.month_no,m.is_leap,m.label);
	rec.month_no=m.month_no;
	rec.is_leap=m.is_leap;
	rec.st_jdutc=m.start_dt.toUtcJD();
	rec.ed_jdutc=m.end_dt.toUtcJD();
	rec.st_utc=fmt_iso(rec.st_jdutc,0,true);
	rec.ed_utc=fmt_iso(rec.ed_jdutc,0,true);
	rec.st_loc=fmt_iso(rec.st_jdutc,tz_off,true);
	rec.ed_loc=fmt_iso(rec.ed_jdutc,tz_off,true);
	return rec;
}


std::vector<MonthRec> bld_mrec(const std::vector<LunarMonth>&months,int tz_off){
	std::vector<MonthRec> out;
	out.reserve(months.size());
	for(const auto&m : months){
		out.push_back(mk_mrec(m,tz_off));
	}
	return out;
}

void write_meta(JsonWriter&w,const std::string&ephem,
				const std::string&tz_display){
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
	w.arr_end();
	w.obj_end();
}

std::vector<LunarEclipse> bld_eclipses(EphRead&eph,const YearResult&yr){
	std::vector<LunarEclipse> out;
	out.reserve(yr.lun_phase.size());
	for(const auto&item : yr.lun_phase){
		double jd_utc=item.full_moon.toUtcJD();
		double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
		LunarEclipse ecl;
		if(calc_lunar_eclipse(eph,jd_tdb,&ecl)&&ecl.has){
			out.push_back(ecl);
		}
	}
	std::sort(out.begin(),out.end(),[](const LunarEclipse&a,const LunarEclipse&b){
		return a.jd_tdb_max<b.jd_tdb_max;
	});
	return out;
}

