#pragma once

#include<cmath>
#include<iomanip>
#include<iosfwd>
#include<limits>
#include<stdexcept>
#include<sstream>
#include<string>
#include<vector>

#include "lunar/events.hpp"
#include "lunar/format.hpp"
#include "lunar/ics.hpp"
#include "lunar/json.hpp"
#include "lunar/lunar_eclipse.hpp"
#include "lunar/spc_ephem.hpp"
#include "lunar/time_scale.hpp"

inline std::string csv_quote(const std::string&s){
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

inline std::string csv_num(double v){
	std::ostringstream oss;
	oss<<std::setprecision(17)<<v;
	return oss.str();
}

class CsvWriter{
public:
	explicit CsvWriter(std::ostream&os) : os_(os){}

	void write_field(const std::string&name,const std::string&value){
		push(name,csv_quote(value));
	}

	void write_field(const std::string&name,const char*value){
		push(name,csv_quote(value==nullptr?"":std::string(value)));
	}

	void write_field(const std::string&name,int value){
		push(name,std::to_string(value));
	}

	void write_field(const std::string&name,bool value){
		push(name,value?"1":"0");
	}

	void write_field(const std::string&name,double value){
		push(name,csv_num(value));
	}

	void write_raw(const std::string&name,const std::string&value){
		push(name,value);
	}

	void write_header(const std::vector<std::string>&header){
		if(header_.empty()){
			header_=header;
			write_line(header_);
		}else if(header_!=header){
			throw std::logic_error("csv header mismatch");
		}
	}

	void finish_row(){
		if(header_.empty()){
			header_=row_header_;
			write_line(header_);
		}else if(header_!=row_header_){
			throw std::logic_error("csv row header mismatch");
		}
		write_line(row_values_);
		row_header_.clear();
		row_values_.clear();
	}

private:
	std::ostream&os_;
	std::vector<std::string> header_;
	std::vector<std::string> row_header_;
	std::vector<std::string> row_values_;

	void push(const std::string&name,const std::string&value){
		row_header_.push_back(name);
		row_values_.push_back(value);
	}

	void write_line(const std::vector<std::string>&cells){
		for(std::size_t i=0;i<cells.size();++i){
			if(i!=0){
				os_<<",";
			}
			os_<<cells[i];
		}
		os_<<"\n";
	}
};

inline bool is_full_moon_ev(const EventRec&ev){
	return ev.kind=="lunar_phase"&&ev.code=="full_moon";
}

inline bool is_new_moon_ev(const EventRec&ev){
	return ev.kind=="lunar_phase"&&ev.code=="new_moon";
}

inline double full_moon_dist_km(EphRead&eph,double jd_utc){
	double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	Vec3 r=raw_vec(eph.get_pos(eph.MOON,eph.EARTH,jd_tdb));
	return r.norm()*AU_KM;
}

inline IcsEvent event_to_ics(const EventRec&ev,bool include_jd_tdb=false){
	IcsEvent out;
	std::ostringstream uid;
	uid<<"lunar-"<<ev.kind<<"-"<<ev.code<<"-"<<std::setprecision(12)
	   <<ev.jd_utc;
	out.uid=uid.str();
	out.summary=ev.name;
	std::ostringstream desc;
	desc<<"kind="<<ev.kind<<"; code="<<ev.code
		<<"; jd_utc="<<std::setprecision(17)<<ev.jd_utc;
	if(include_jd_tdb&&std::isfinite(ev.jd_tdb)){
		desc<<"; jd_tdb="<<ev.jd_tdb;
	}
	out.desc=desc.str();
	out.jd_utc=ev.jd_utc;
	return out;
}

inline std::string node_num(double v){
	if(!std::isfinite(v)){
		return "null";
	}
	std::ostringstream oss;
	oss<<std::setprecision(17)<<v;
	return oss.str();
}

inline void wr_num_txt(std::ostream&os,double v){
	os<<node_num(v);
}

inline void wr_node_txt(std::ostream&os,double jd_tdb,
						const EclipsePointMeta&meta){
	if(!std::isfinite(jd_tdb)){
		os<<"null\tnull\tnull\tnull\tnull\tnull\tnull";
		return;
	}
	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double jd_ut1=TimeScale::tdb_to_ut1(jd_tdb);
	os<<node_num(jd_ut1)<<"\t"<<node_num(jd_td)<<"\t"<<node_num(jd_utc)<<"\t"
	  <<node_num(meta.zen_lat_deg)<<"\t"<<node_num(meta.zen_lon_deg)<<"\t"
	  <<node_num(meta.pa_deg)<<"\t"<<node_num(meta.axis_deg);
}

inline void wr_node_kv(std::ostream&os,const std::string&tag,double jd_tdb,
					   const EclipsePointMeta&meta){
	double jd_td=std::numeric_limits<double>::quiet_NaN();
	double jd_utc=std::numeric_limits<double>::quiet_NaN();
	double jd_ut1=std::numeric_limits<double>::quiet_NaN();
	if(std::isfinite(jd_tdb)){
		jd_td=TimeScale::tdb_to_tt(jd_tdb);
		jd_utc=TimeScale::tdb_to_utc(jd_tdb);
		jd_ut1=TimeScale::tdb_to_ut1(jd_tdb);
	}
	os<<tag<<"_jd_ut1="<<node_num(jd_ut1)<<"\n";
	os<<tag<<"_jd_td="<<node_num(jd_td)<<"\n";
	os<<tag<<"_jd="<<node_num(jd_utc)<<"\n";
	os<<tag<<"_zen_lat_deg="<<node_num(meta.zen_lat_deg)<<"\n";
	os<<tag<<"_zen_lon_deg="<<node_num(meta.zen_lon_deg)<<"\n";
	os<<tag<<"_pa_deg="<<node_num(meta.pa_deg)<<"\n";
	os<<tag<<"_axis_deg="<<node_num(meta.axis_deg)<<"\n";
}

inline void wr_num_or_null(JsonWriter&w,double v){
	if(std::isfinite(v)){
		w.value(v);
	}else{
		w.null_val();
	}
}

inline void wr_geo_json(JsonWriter&w,const EclipseGeoCoord&g){
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

inline void wr_lib_json(JsonWriter&w,const EclipseLibration&lib){
	w.obj_begin();
	w.key("l_deg");
	wr_num_or_null(w,lib.l_deg);
	w.key("b_deg");
	wr_num_or_null(w,lib.b_deg);
	w.key("c_deg");
	wr_num_or_null(w,lib.c_deg);
	w.obj_end();
}

inline void wr_enode(JsonWriter&w,double jd_tdb,int tz_off,
					 const EclipsePointMeta*meta=nullptr){
	if(!std::isfinite(jd_tdb)){
		w.null_val();
		return;
	}
	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double jd_ut1=TimeScale::tdb_to_ut1(jd_tdb);
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

inline std::string node_liso(double jd_tdb,int tz_off){
	if(!std::isfinite(jd_tdb)){
		return "null";
	}
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	return fmt_iso(jd_utc,tz_off,true);
}

inline void wr_ecljson(JsonWriter&w,const LunarEclipse&ecl,int year,int tz_off){
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
