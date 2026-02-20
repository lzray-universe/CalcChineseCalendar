#include "lunar/cli.hpp"
#include "lunar/cli_common.hpp"

#include<algorithm>
#include<array>
#include<cctype>
#include<cmath>
#include<filesystem>
#include<fstream>
#include<functional>
#include<iomanip>
#include<iostream>
#include<limits>
#include<map>
#include<set>
#include<sstream>
#include<stdexcept>
#include<tuple>
#include<unordered_map>
#include<utility>

#include "lunar/calendar.hpp"
#include "lunar/download.hpp"
#include "lunar/events.hpp"
#include "lunar/format.hpp"
#include "lunar/ics.hpp"
#include "lunar/js_writer.hpp"
#include "lunar/time_scale.hpp"

namespace{

struct MonYrData{
	int year=0;
	std::string mode;
	std::vector<MonthRec> months;
};

struct CalYrData{
	int year=0;
	std::string mode;
	std::vector<EventRec> sol_terms;
	std::vector<EventRec> lun_phase;
	std::vector<MonthRec> months;
	bool inc_month=false;
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

MonthRec mk_mrec(const LunarMonth&m,int tz_off){
	MonthRec rec;
	rec.label=m.label;
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
	w.value("lunar.v1");
	w.key("ephem");
	w.value(ephem);
	w.key("tz_display");
	w.value(tz_display);
	w.key("notes");
	w.arr_begin();
	w.value("算法不变，仅格式化输出和CLI能力增强");
	w.value("--tz仅影响显示，不改变农历规则与计算流程");
	w.arr_end();
	w.obj_end();
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
	w.key("jd_tdb");
	if(std::isfinite(ev.jd_tdb)){
		w.value(ev.jd_tdb);
	}else{
		w.null_val();
	}
	w.key("jd_utc");
	w.value(ev.jd_utc);
	w.key("utc_iso");
	w.value(ev.utc_iso);
	w.key("loc_iso");
	w.value(ev.loc_iso);
	w.obj_end();
}

void wr_mj(JsonWriter&w,const MonthRec&m){
	w.obj_begin();
	w.key("label");
	w.value(m.label);
	w.key("month_no");
	w.value(m.month_no);
	w.key("is_leap");
	w.value(m.is_leap);
	w.key("st_jdutc");
	w.value(m.st_jdutc);
	w.key("ed_jdutc");
	w.value(m.ed_jdutc);
	w.key("st_utc");
	w.value(m.st_utc);
	w.key("st_loc");
	w.value(m.st_loc);
	w.key("ed_utc");
	w.value(m.ed_utc);
	w.key("ed_loc");
	w.value(m.ed_loc);
	w.obj_end();
}

void wr_cyjs(JsonWriter&w,const CalYrData&item,bool inc_mode){
	w.obj_begin();
	w.key("year");
	w.value(item.year);
	if(inc_mode){
		w.key("mode");
		w.value(item.mode);
	}
	w.key("sol_terms");
	w.arr_begin();
	for(const auto&ev : item.sol_terms){
		wr_ejson(w,ev);
	}
	w.arr_end();
	w.key("lun_phase");
	w.arr_begin();
	for(const auto&ev : item.lun_phase){
		wr_ejson(w,ev);
	}
	w.arr_end();
	if(item.inc_month){
		w.key("months");
		w.arr_begin();
		for(const auto&m : item.months){
			wr_mj(w,m);
		}
		w.arr_end();
	}
	w.obj_end();
}

void wr_mjs(std::ostream&os,const std::vector<MonYrData>&data,
			const std::string&ephem,const std::string&tz_display,bool pretty){
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	w.arr_begin();
	for(const auto&bundle : data){
		w.obj_begin();
		w.key("year");
		w.value(bundle.year);
		w.key("mode");
		w.value(bundle.mode);
		w.key("months");
		w.arr_begin();
		for(const auto&m : bundle.months){
			wr_mj(w,m);
		}
		w.arr_end();
		w.obj_end();
	}
	w.arr_end();
	w.obj_end();
	os<<"\n";
}

void wr_caljs(std::ostream&os,const std::vector<CalYrData>&years,
			  const std::string&ephem,const std::string&tz_display,bool pretty){
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	if(years.size()==1){
		wr_cyjs(w,years.front(),false);
	}else{
		w.arr_begin();
		for(const auto&item : years){
			wr_cyjs(w,item,false);
		}
		w.arr_end();
	}
	w.obj_end();
	os<<"\n";
}

void wr_yjs(std::ostream&os,const CalYrData&year_data,const std::string&ephem,
			const std::string&tz_display,bool pretty){
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	wr_cyjs(w,year_data,true);
	w.obj_end();
	os<<"\n";
}

void wr_ejdoc(std::ostream&os,const EventRec&event,const std::string&ephem,
			  const std::string&tz_display,bool pretty){
	JsonWriter w(os,pretty);
	w.obj_begin();
	write_meta(w,ephem,tz_display);
	w.key("data");
	wr_ejson(w,event);
	w.obj_end();
	os<<"\n";
}

std::string csv_quote(const std::string&s){
	bool need_q=false;
	for(char c : s){
		if(c==','||c=='"'||c=='\n'||c=='\r'){
			need_q=true;
			break;
		}
	}
	if(!need_q){
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

IcsEvent ev_toics(const EventRec&ev){
	IcsEvent out;
	std::ostringstream uid;
	uid<<"lunar-"<<ev.kind<<"-"<<ev.code<<"-"<<std::setprecision(12)<<ev.jd_utc;
	out.uid=uid.str();
	out.summary=ev.name;
	std::ostringstream desc;
	desc<<"kind="<<ev.kind<<"; code="<<ev.code
		<<"; jd_utc="<<std::setprecision(17)<<ev.jd_utc;
	if(std::isfinite(ev.jd_tdb)){
		desc<<"; jd_tdb="<<ev.jd_tdb;
	}
	out.desc=desc.str();
	out.jd_utc=ev.jd_utc;
	return out;
}

void wr_eics(std::ostream&os,const std::string&ephem,const std::string&cal_name,
			 const std::vector<EventRec>&events){
	std::vector<IcsEvent> ics_events;
	ics_events.reserve(events.size());
	for(const auto&ev : events){
		ics_events.push_back(ev_toics(ev));
	}
	write_ics(os,"lunar-cli//"+tool_ver(),cal_name,ics_events,
			  {"ephem="+ephem,"算法不变，仅输出增强"});
}

void wr_mtxt(std::ostream&os,const std::vector<MonYrData>&data,
			 const std::string&tz_display){
	os<<"tool=lunar format=txt type=months tz_display="<<tz_display<<"\n";
	os<<"note=--tz仅影响显示，不改变计算\n";
	for(const auto&bundle : data){
		os<<"\n[year="<<bundle.year<<" mode="<<bundle.mode<<"]\n";
		os<<"label\tmonth_no\tis_leap\tst_jd\ted_jd\tst_utc"
			"iso\tst_liso\ted_uiso\ted_liso\n";
		os<<std::setprecision(17);
		for(const auto&m : bundle.months){
			os<<m.label<<"\t"<<m.month_no<<"\t"<<(m.is_leap?1:0)<<"\t"
			  <<m.st_jdutc<<"\t"<<m.ed_jdutc<<"\t"<<m.st_utc<<"\t"<<m.st_loc
			  <<"\t"<<m.ed_utc<<"\t"<<m.ed_loc<<"\n";
		}
	}
}

void wr_mcsv(std::ostream&os,const std::vector<MonYrData>&data){
	os<<"year,mode,label,month_no,is_leap,st_jdutc,ed_jdutc,start_utc_"
		"iso,st_loc,ed_utc,ed_loc\n";
	os<<std::setprecision(17);
	for(const auto&bundle : data){
		for(const auto&m : bundle.months){
			os<<bundle.year<<","<<csv_quote(bundle.mode)<<","
			  <<csv_quote(m.label)<<","<<m.month_no<<","<<(m.is_leap?1:0)<<","
			  <<m.st_jdutc<<","<<m.ed_jdutc<<","<<csv_quote(m.st_utc)<<","
			  <<csv_quote(m.st_loc)<<","<<csv_quote(m.ed_utc)<<","
			  <<csv_quote(m.ed_loc)<<"\n";
		}
	}
}

void wr_etxt(std::ostream&os,const std::vector<EventRec>&events){
	os<<"kind\tcode\tname\tyear\tjd_tdb\tjd_utc\ttm_uiso\ttm_loc"
		"iso\n";
	os<<std::setprecision(17);
	for(const auto&ev : events){
		os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t";
		if(std::isfinite(ev.jd_tdb)){
			os<<ev.jd_tdb;
		}else{
			os<<"null";
		}
		os<<"\t"<<ev.jd_utc<<"\t"<<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
	}
}

void wr_caltx(std::ostream&os,const std::vector<CalYrData>&years,
			  const std::string&tz_display){
	os<<"tool=lunar format=txt type=calendar tz_display="<<tz_display<<"\n";
	os<<"note=--tz仅影响显示，不改变计算\n";
	for(const auto&item : years){
		os<<"\n[year="<<item.year<<"]\n";
		os<<"## sol_terms\n";
		wr_etxt(os,item.sol_terms);
		os<<"## lun_phase\n";
		wr_etxt(os,item.lun_phase);
		if(item.inc_month){
			os<<"## months\n";
			os<<"label\tmonth_no\tis_leap\tst_jd\ted_jd\tst_utc"
				"iso\tst_liso\ted_uiso\ted_liso\n";
			os<<std::setprecision(17);
			for(const auto&m : item.months){
				os<<m.label<<"\t"<<m.month_no<<"\t"<<(m.is_leap?1:0)<<"\t"
				  <<m.st_jdutc<<"\t"<<m.ed_jdutc<<"\t"<<m.st_utc<<"\t"<<m.st_loc
				  <<"\t"<<m.ed_utc<<"\t"<<m.ed_loc<<"\n";
			}
		}
	}
}

void wr_ytxt(std::ostream&os,const CalYrData&item,const std::string&tz_display){
	os<<"tool=lunar format=txt type=year mode="<<item.mode
	  <<" tz_display="<<tz_display<<"\n";
	os<<"note=--tz仅影响显示，不改变计算\n";
	os<<"\n[year="<<item.year<<" mode="<<item.mode<<"]\n";
	os<<"## sol_terms\n";
	wr_etxt(os,item.sol_terms);
	os<<"## lun_phase\n";
	wr_etxt(os,item.lun_phase);
	os<<"## months\n";
	os<<"label\tmonth_no\tis_leap\tst_jd\ted_jd\tst_utc"
		"iso\tst_liso\ted_uiso\ted_liso\n";
	os<<std::setprecision(17);
	for(const auto&m : item.months){
		os<<m.label<<"\t"<<m.month_no<<"\t"<<(m.is_leap?1:0)<<"\t"<<m.st_jdutc
		  <<"\t"<<m.ed_jdutc<<"\t"<<m.st_utc<<"\t"<<m.st_loc<<"\t"<<m.ed_utc
		  <<"\t"<<m.ed_loc<<"\n";
	}
}

void wr_setxt(std::ostream&os,const EventRec&ev,const std::string&tz_display){
	os<<"tool=lunar format=txt type=event tz_display="<<tz_display<<"\n";
	os<<"kind\tcode\tname\tyear\tjd_tdb\tjd_utc\ttm_uiso\ttm_loc"
		"iso\n";
	os<<std::setprecision(17);
	os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t";
	if(std::isfinite(ev.jd_tdb)){
		os<<ev.jd_tdb;
	}else{
		os<<"null";
	}
	os<<"\t"<<ev.jd_utc<<"\t"<<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
}

void chk_mode(const std::string&mode){
	if(mode!="lunar"&&mode!="gregorian"){
		throw std::invalid_argument("mode must be lunar or gregorian");
	}
}

std::tuple<int,int,int> parse_ld(const std::string&s){
	if(s.size()!=10||s[4]!='-'||s[7]!='-'){
		throw std::invalid_argument("invalid date, expected YYYY-MM-DD: "+s);
	}
	for(std::size_t i=0;i<s.size();++i){
		if(i==4||i==7){
			continue;
		}
		if(!std::isdigit(static_cast<unsigned char>(s[i]))){
			throw std::invalid_argument("invalid date, expected YYYY-MM-DD: "+
										s);
		}
	}
	int y=parse_int(s.substr(0,4),"year");
	int m=parse_int(s.substr(5,2),"month");
	int d=parse_int(s.substr(8,2),"day");
	if(m<1||m>12||d<1||d>31){
		throw std::invalid_argument("invalid date value: "+s);
	}
	return {y,m,d};
}

void run_mout(const MonthsArgs&args,const std::vector<MonYrData>&data,
			  const std::string&format,const std::string&out_path){
	OutTgt out=open_out(out_path);
	const FmtMap handlers={
		{"json",[&](){ wr_mjs(*out.stream,data,args.ephem,args.tz,args.pretty); }},
		{"txt",[&](){ wr_mtxt(*out.stream,data,args.tz); }},
		{"csv",[&](){ wr_mcsv(*out.stream,data); }},
	};
	run_fmt(handlers,format,"months");
	note_out(out_path,args.quiet);
}

}

std::vector<int> parse_year(const std::string&arg){
	std::vector<int> years;
	std::string tmp;
	std::vector<std::string> parts;
	std::istringstream iss(arg);
	while(std::getline(iss,tmp,',')){
		std::string p;
		for(char c : tmp){
			if(!std::isspace(static_cast<unsigned char>(c))){
				p.push_back(c);
			}
		}
		if(!p.empty()){
			parts.push_back(p);
		}
	}
	if(parts.empty()){
		throw std::invalid_argument("years argument is empty");
	}

	for(const auto&part : parts){
		auto pos=part.find('-');
		if(pos!=std::string::npos){
			int start=parse_int(part.substr(0,pos),"year");
			int end=parse_int(part.substr(pos+1),"year");
			if(end<start){
				throw std::invalid_argument("invalid year range: "+part);
			}
			for(int y=start;y<=end;++y){
				years.push_back(y);
			}
		}else{
			years.push_back(parse_int(part,"year"));
		}
	}

	std::vector<int> ordered;
	std::set<int> seen;
	for(int y : years){
		if(seen.insert(y).second){
			ordered.push_back(y);
		}
	}
	return ordered;
}

void cli_month(const MonthsArgs&args){
	std::vector<int> years=parse_year(args.years);
	const std::string mode=to_low(args.mode);
	chk_mode(mode);

	int tz_off=parse_tz(args.tz);

	EphRead eph(args.ephem);
	LunCal6 calc(eph);

	std::vector<MonYrData> data;
	data.reserve(years.size());
	for(int y : years){
		if(!args.quiet){
			std::cerr<<"computing months for year "<<y<<" ..."<<std::endl;
		}
		std::vector<LunarMonth> months=
			(mode=="lunar")?enum_lyr(calc,y):enum_gyr(calc,y);
		MonYrData row;
		row.year=y;
		row.mode=mode;
		row.months=bld_mrec(months,tz_off);
		data.push_back(std::move(row));
	}

	if(!args.out_json.empty()||!args.out_txt.empty()){
		if(!args.out_json.empty()){
			OutTgt out=open_out(args.out_json);
			wr_mjs(*out.stream,data,args.ephem,args.tz,true);
			note_out(args.out_json,args.quiet);
		}
		if(!args.out_txt.empty()){
			OutTgt out=open_out(args.out_txt);
			wr_mtxt(*out.stream,data,args.tz);
			note_out(args.out_txt,args.quiet);
		}
		return;
	}

	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","csv"},"months");
	run_mout(args,data,format,args.out);
}

void cli_cal(const CalArgs&args){
	int tz_off=parse_tz(args.tz);
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","ics"},"calendar");

	std::vector<int> years;
	if(args.has_years){
		years=parse_year(args.years_arg);
	}else{
		years.push_back(2025);
	}

	EphRead eph(args.ephem);
	SolLunCal solver(eph);
	LunCal6 calc(eph);

	std::vector<CalYrData> out_data;
	out_data.reserve(years.size());
	for(int y : years){
		YearResult yr=solver.compute_year(y,args.quiet?nullptr:&std::cerr);
		CalYrData item;
		item.year=y;
		item.mode="lunar";
		item.sol_terms=bld_stev(yr,tz_off);
		item.lun_phase=bld_lpev(yr,tz_off);
		item.inc_month=args.inc_month;
		if(args.inc_month){
			std::vector<LunarMonth> months=enum_lyr(calc,y);
			item.months=bld_mrec(months,tz_off);
		}
		out_data.push_back(std::move(item));
	}

	OutTgt out=open_out(args.out);
	const FmtMap handlers={
		{"json",[&](){
			 wr_caljs(*out.stream,out_data,args.ephem,args.tz,args.pretty);
		 }},
		{"ics",[&](){
			 std::vector<EventRec> merged;
			 for(const auto&item : out_data){
				 merged.insert(merged.end(),item.sol_terms.begin(),
							   item.sol_terms.end());
				 merged.insert(merged.end(),item.lun_phase.begin(),
							   item.lun_phase.end());
			 }
			 std::sort(merged.begin(),merged.end(),[](const EventRec&a,
													 const EventRec&b){
				 return a.jd_utc<b.jd_utc;
			 });
			 std::ostringstream name;
			 name<<"lunar-calendar";
			 if(!years.empty()){
				 name<<"-"<<years.front();
				 if(years.size()>1){
					 name<<"-to-"<<years.back();
				 }
			 }
			 wr_eics(*out.stream,args.ephem,name.str(),merged);
		 }},
		{"txt",[&](){ wr_caltx(*out.stream,out_data,args.tz); }},
	};
	run_fmt(handlers,format,"calendar");
	note_out(args.out,args.quiet);
}

void cli_year(const YearArgs&args){
	int tz_off=parse_tz(args.tz);
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","ics"},"year");
	const std::string mode=to_low(args.mode);
	chk_mode(mode);

	EphRead eph(args.ephem);
	SolLunCal solver(eph);
	LunCal6 calc(eph);

	YearResult yr=solver.compute_year(args.year,args.quiet?nullptr:&std::cerr);
	CalYrData data;
	data.year=args.year;
	data.mode=mode;
	data.sol_terms=bld_stev(yr,tz_off);
	data.lun_phase=bld_lpev(yr,tz_off);
	data.inc_month=true;
	std::vector<LunarMonth> months=
		(mode=="lunar")?enum_lyr(calc,args.year):enum_gyr(calc,args.year);
	data.months=bld_mrec(months,tz_off);

	OutTgt out=open_out(args.out);
	const FmtMap handlers={
		{"json",[&](){ wr_yjs(*out.stream,data,args.ephem,args.tz,args.pretty); }},
		{"ics",[&](){
			 std::vector<EventRec> merged=data.sol_terms;
			 merged.insert(merged.end(),data.lun_phase.begin(),
						   data.lun_phase.end());
			 std::sort(merged.begin(),merged.end(),[](const EventRec&a,
													 const EventRec&b){
				 return a.jd_utc<b.jd_utc;
			 });
			 wr_eics(*out.stream,args.ephem,
					 "lunar-year-"+std::to_string(args.year),merged);
		 }},
		{"txt",[&](){ wr_ytxt(*out.stream,data,args.tz); }},
	};
	run_fmt(handlers,format,"year");
	note_out(args.out,args.quiet);
}

void cli_event(const EventArgs&args){
	int tz_off=parse_tz(args.tz);
	const std::string format=to_low(args.format);
	chk_fmt(format,{"json","txt","ics"},"event");

	EphRead eph(args.ephem);
	SolLunCal solver(eph);

	const std::string category=to_low(args.category);
	EventRec ev;
	if(category=="solar-term"){
		if(!args.has_year){
			throw std::invalid_argument("solar-term requires a <year>");
		}
		const auto&defs=SolLunCal::st_defs();
		auto it=defs.find(args.code);
		if(it==defs.end()){
			throw std::invalid_argument("unknown solar-term code: "+args.code);
		}
		LocalDT dt=solver.find_st(args.code,args.year);
		ev=mk_erec("solar_term",args.code,it->second.name,args.year,
				   std::numeric_limits<double>::quiet_NaN(),dt.toUtcJD(),
				   tz_off);
	}else if(category=="lunar-phase"){
		const auto&defs=SolLunCal::lp_defs();
		auto it=defs.find(args.code);
		if(it==defs.end()){
			throw std::invalid_argument("unknown lunar-phase code: "+args.code);
		}
		if(args.near_date.empty()){
			throw std::invalid_argument(
				"lunar-phase requires --near YYYY-MM-DD");
		}
		int y=0;
		int m=0;
		int d=0;
		std::tie(y,m,d)=parse_ld(args.near_date);
		LocalDT loc_mid=SolLunCal::mk_local(y,m,d,0,0,0.0);
		double jd_utc=SolLunCal::loc2utc(loc_mid);
		double jd_guess=TimeScale::utc_to_tdb(jd_utc);
		LocalDT dt=solver.find_lp(args.code,jd_guess);
		ev=mk_erec("lunar_phase",args.code,it->second.name,dt.year,
				   std::numeric_limits<double>::quiet_NaN(),dt.toUtcJD(),
				   tz_off);
	}else{
		throw std::invalid_argument(
			"event category must be solar-term or lunar-phase");
	}

	OutTgt out=open_out(args.out);
	const FmtMap handlers={
		{"json",[&](){ wr_ejdoc(*out.stream,ev,args.ephem,args.tz,args.pretty); }},
		{"ics",[&](){
			 std::vector<EventRec> one{ev};
			 wr_eics(*out.stream,args.ephem,"lunar-event",one);
		 }},
		{"txt",[&](){ wr_setxt(*out.stream,ev,args.tz); }},
	};
	run_fmt(handlers,format,"event");
	note_out(args.out,args.quiet);
}

void cli_dl(const DlArgs&args){
	const std::string action=to_low(args.action);
	if(action=="list"){
		auto opts=bsp_opts();
		std::cout<<"id\tsize\trange\turl\n";
		for(const auto&opt : opts){
			std::cout<<opt.id<<"\t"<<opt.size<<"\t"<<opt.range<<"\t"<<opt.url
					 <<"\n";
		}
		return;
	}

	if(action!="get"){
		throw std::invalid_argument("download action must be list or get");
	}

	auto opts=bsp_opts();
	const BspOption*found=nullptr;
	std::string id_l=to_low(args.id);
	for(const auto&opt : opts){
		if(to_low(opt.id)==id_l){
			found=&opt;
			break;
		}
	}
	if(!found){
		throw std::invalid_argument("unknown bsp id: "+args.id);
	}

	std::filesystem::path dir=args.dir.empty()?std::filesystem::current_path()
											  :std::filesystem::path(args.dir);
	std::error_code ec;
	std::filesystem::create_directories(dir,ec);
	if(ec){
		throw std::runtime_error("failed to create output directory: "+
								 dir.string());
	}
	std::filesystem::path filename=std::filesystem::path(found->url).filename();
	std::filesystem::path target=dir/filename;
	if(!dl_file(found->url,target.string())){
		throw std::runtime_error("download failed");
	}
	std::cout<<target.string()<<std::endl;
}

int cmd_month(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_month();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("months requires: <bsp> <years>");
	}

	MonthsArgs margs;
	margs.ephem=args[0];
	margs.years=args[1];
	const OptMap handlers={
		{"--mode",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 margs.mode=to_low(req_val(src,idx,opt));
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 margs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ margs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ margs.tz=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 margs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ margs.quiet=true; }},
		{"--output",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 margs.out_json=req_val(src,idx,opt);
		 }},
		{"--output-txt",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 margs.out_txt=req_val(src,idx,opt);
		 }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_month();
			return 0;
		}
		apply_opt(handlers,args,i,a,"months");
	}

	if((!margs.out_json.empty()||!margs.out_txt.empty())&&!margs.out.empty()){
		throw std::invalid_argument(
			"--out cannot be combined with deprecated --output/--output-txt");
	}

	cli_month(margs);
	return 0;
}

int cmd_cal(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_cal();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument("calendar requires: <bsp> [<years>]");
	}

	CalArgs cargs;
	cargs.ephem=args[0];
	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		cargs.years_arg=args[i];
		cargs.has_years=true;
		++i;
	}
	const OptMap handlers={
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 cargs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ cargs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ cargs.tz=req_val(src,idx,opt); }},
		{"--include-months",[&](const std::vector<std::string>&src,
								std::size_t&idx,const std::string&opt){
			 cargs.inc_month=parse_bool01(req_val(src,idx,opt),"--include-months");
		 }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 cargs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ cargs.quiet=true; }},
	};
	for(;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_cal();
			return 0;
		}
		apply_opt(handlers,args,i,a,"calendar");
	}

	cli_cal(cargs);
	return 0;
}

int cmd_year(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_year();
		return 0;
	}
	if(args.size()<2){
		throw std::invalid_argument("year requires: <bsp> <year>");
	}

	YearArgs yargs;
	yargs.ephem=args[0];
	yargs.year=parse_int(args[1],"year");
	const OptMap handlers={
		{"--mode",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 yargs.mode=to_low(req_val(src,idx,opt));
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 yargs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ yargs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ yargs.tz=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 yargs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ yargs.quiet=true; }},
	};

	for(std::size_t i=2;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_year();
			return 0;
		}
		apply_opt(handlers,args,i,a,"year");
	}

	cli_year(yargs);
	return 0;
}

int cmd_event(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_event();
		return 0;
	}
	if(args.size()<3){
		throw std::invalid_argument(
			"event requires: <bsp> <solar-term|lunar-phase> ...");
	}

	EventArgs eargs;
	eargs.ephem=args[0];
	eargs.category=to_low(args[1]);
	eargs.code=args[2];

	std::size_t i=3;
	if(eargs.category=="solar-term"){
		if(i>=args.size()){
			throw std::invalid_argument("solar-term requires: <code> <year>");
		}
		eargs.year=parse_int(args[i],"year");
		eargs.has_year=true;
		++i;
	}else if(eargs.category!="lunar-phase"){
		throw std::invalid_argument(
			"event category must be solar-term or lunar-phase");
	}
	const OptMap handlers={
		{"--near",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 eargs.near_date=req_val(src,idx,opt);
		 }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 eargs.format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ eargs.out=req_val(src,idx,opt); }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ eargs.tz=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 eargs.pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ eargs.quiet=true; }},
	};

	for(;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_event();
			return 0;
		}
		apply_opt(handlers,args,i,a,"event");
	}

	if(eargs.category=="lunar-phase"&&eargs.near_date.empty()){
		throw std::invalid_argument("lunar-phase requires --near YYYY-MM-DD");
	}

	cli_event(eargs);
	return 0;
}

int cmd_dl(const std::vector<std::string>&args){
	if(args.empty()||(args.size()==1&&(args[0]=="-h"||args[0]=="--help"))){
		use_dl();
		return 0;
	}

	DlArgs dargs;
	dargs.action=to_low(args[0]);

	std::size_t i=1;
	if(dargs.action=="get"){
		if(i>=args.size()){
			throw std::invalid_argument("download get requires <id>");
		}
		dargs.id=args[i];
		++i;
	}else if(dargs.action!="list"){
		throw std::invalid_argument("download action must be list or get");
	}
	const OptMap handlers={
		{"--dir",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ dargs.dir=req_val(src,idx,opt); }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ dargs.quiet=true; }},
	};

	for(;i<args.size();++i){
		const std::string&a=args[i];
		if(a=="-h"||a=="--help"){
			use_dl();
			return 0;
		}
		apply_opt(handlers,args,i,a,"download");
	}

	cli_dl(dargs);
	return 0;
}

std::string tool_ver(){ return "lunar-cli-2026.02"; }

void use_month(){
	std::cout
		<<"Usage:\n"
		<<"  lunar months <bsp> <years>\n"
		<<"    [--mode lunar|gregorian]\n"
		<<"    [--format json|txt|csv] [--out <path>] [--tz +08:00|Z|-05:00]\n"
		<<"    [--pretty 0|1] [--quiet]\n"
		<<"    [--output <json>] [--output-txt <txt>]   # deprecated\n"
		<<"Examples:\n"
		<<"  lunar months D:\\de442.bsp 2025\n"
		<<"  lunar months D:\\de442.bsp 2024-2026 --mode gregorian --format "
		  "json --out months.json\n"
		<<"  lunar months D:\\de442.bsp 2025 --format csv --out months.csv "
		  "--tz Z\n"
		<<"Notes:\n"
		<<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_cal(){
	std::cout
		<<"Usage:\n"
		<<"  lunar calendar <bsp> [<years>]\n"
		<<"    [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]\n"
		<<"    [--include-months 0|1] [--pretty 0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar calendar D:\\de442.bsp 2025\n"
		<<"  lunar calendar D:\\de442.bsp 2024-2026 --format json --out "
		  "cal.json\n"
		<<"  lunar calendar D:\\de442.bsp 2025 --format ics --out cal.ics\n"
		<<"Notes:\n"
		<<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_year(){
	std::cout
		<<"Usage:\n"
		<<"  lunar year <bsp> <year>\n"
		<<"    [--mode lunar|gregorian]\n"
		<<"    [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]\n"
		<<"    [--pretty 0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar year D:\\de442.bsp 2025\n"
		<<"  lunar year D:\\de442.bsp 2025 --format json --out year-2025.json\n"
		<<"  lunar year D:\\de442.bsp 2025 --format ics --out year-2025.ics\n"
		<<"Notes:\n"
		<<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_event(){
	std::cout<<"Usage:\n"
			 <<"  lunar event <bsp> solar-term <code> <year>\n"
			 <<"    [--format json|txt|ics] [--out <path>] [--tz "
			   "+08:00|Z|-05:00] [--pretty 0|1] [--quiet]\n"
			 <<"  lunar event <bsp> lunar-phase "
			   "<new_moon|fst_qtr|full_moon|lst_qtr>\n"
			 <<"    --near <YYYY-MM-DD> [--format json|txt|ics] [--out <path>] "
			   "[--tz ...] [--pretty 0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar event D:\\de442.bsp solar-term Z2 2025\n"
			 <<"  lunar event D:\\de442.bsp lunar-phase full_moon --near "
			   "2025-09-07\n"
			 <<"  lunar event D:\\de442.bsp solar-term J1 2025 --format ics "
			   "--out event.ics\n"
			 <<"Notes:\n"
			 <<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_dl(){
	std::cout<<"Usage:\n"
			 <<"  lunar download list\n"
			 <<"  lunar download get <id> [--dir <path>]\n"
			 <<"Examples:\n"
			 <<"  lunar download list\n"
			 <<"  lunar download get de442\n"
			 <<"  lunar download get de442s --dir D:\\ephem\n";
}

void use_main(){
	std::cout<<"Usage:\n"
			 <<"  lunar --help\n"
			 <<"  lunar --version\n"
			 <<"  lunar months   ...\n"
			 <<"  lunar calendar ...\n"
			 <<"  lunar year     ...\n"
			 <<"  lunar event    ...\n"
			 <<"  lunar at       ...\n"
			 <<"  lunar convert  ...\n"
			 <<"  lunar day      ...\n"
			 <<"  lunar monthview...\n"
			 <<"  lunar next     ...\n"
			 <<"  lunar range    ...\n"
			 <<"  lunar search   ...\n"
			 <<"  lunar festival ...\n"
			 <<"  lunar almanac  ...\n"
			 <<"  lunar info     ...\n"
			 <<"  lunar selftest ...\n"
			 <<"  lunar config   ...\n"
			 <<"  lunar completion...\n"
			 <<"  lunar download ...\n"
			 <<"\n"
			 <<"Compatibility:\n"
			 <<"  lunar <bsp> <years> [months options...]  # same as months\n"
			 <<"\n"
			 <<"Subcommand help:\n"
			 <<"  lunar months --help\n"
			 <<"  lunar calendar --help\n"
			 <<"  lunar year --help\n"
			 <<"  lunar event --help\n"
			 <<"  lunar at --help\n"
			 <<"  lunar convert --help\n"
			 <<"  lunar day --help\n"
			 <<"  lunar monthview --help\n"
			 <<"  lunar next --help\n"
			 <<"  lunar range --help\n"
			 <<"  lunar search --help\n"
			 <<"  lunar festival --help\n"
			 <<"  lunar almanac --help\n"
			 <<"  lunar info --help\n"
			 <<"  lunar selftest --help\n"
			 <<"  lunar config --help\n"
			 <<"  lunar completion --help\n"
			 <<"  lunar download --help\n";
}
