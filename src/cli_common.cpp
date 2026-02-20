#include "lunar/cli_common.hpp"

#include<algorithm>
#include<array>
#include<cctype>
#include<iostream>
#include<limits>
#include<stdexcept>

#include "lunar/format.hpp"

namespace cli_util{

bool is_opt(const std::string&s){ return !s.empty()&&s[0]=='-'; }

std::string to_low(std::string s){
	for(char&c : s){
		c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return s;
}

int parse_int(const std::string&text,const std::string&label){
	std::size_t pos=0;
	int v=0;
	try{
		v=std::stoi(text,&pos);
	}catch(...){
		throw std::invalid_argument("invalid "+label+": "+text);
	}
	if(pos!=text.size()){
		throw std::invalid_argument("invalid "+label+": "+text);
	}
	return v;
}

bool parse_bool01(const std::string&text,const std::string&label){
	if(text=="0"){
		return false;
	}
	if(text=="1"){
		return true;
	}
	throw std::invalid_argument(label+" must be 0 or 1");
}

std::string req_val(const std::vector<std::string>&args,std::size_t&idx,
					const std::string&opt){
	if(idx+1>=args.size()){
		throw std::invalid_argument("missing value for option: "+opt);
	}
	++idx;
	return args[idx];
}

OutTgt open_out(const std::string&path){
	OutTgt out;
	if(path.empty()){
		out.stream=&std::cout;
		return out;
	}
	out.file.open(path,std::ios::binary);
	if(!out.file){
		throw std::runtime_error("failed to open output file: "+path);
	}
	out.stream=&out.file;
	return out;
}

void note_out(const std::string&path,bool quiet){
	if(!path.empty()&&!quiet){
		std::cerr<<"written: "<<path<<std::endl;
	}
}

void chk_fmt(const std::string&format,const std::set<std::string>&allowed,
			 const std::string&ctx){
	if(allowed.find(format)==allowed.end()){
		throw std::invalid_argument("invalid --format for "+ctx+": "+format);
	}
}

EventRec mk_erec(const std::string&kind,const std::string&code,
				 const std::string&name,int year,double jd_tdb,double jd_utc,
				 int tz_off){
	EventRec rec;
	rec.kind=kind;
	rec.code=code;
	rec.name=name;
	rec.year=year;
	rec.jd_tdb=jd_tdb;
	rec.jd_utc=jd_utc;
	rec.utc_iso=fmt_iso(jd_utc,0,true);
	rec.loc_iso=fmt_iso(jd_utc,tz_off,true);
	return rec;
}

EventRec mk_erec(const std::string&kind,const std::string&code,
				 const std::string&name,int year,double jd_utc,int tz_off){
	return mk_erec(kind,code,name,year,std::numeric_limits<double>::quiet_NaN(),
				   jd_utc,tz_off);
}

std::vector<EventRec> bld_stev(const YearResult&yr,int tz_off){
	std::vector<EventRec> out;
	out.reserve(yr.sol_terms.size());
	for(const auto&kv : yr.sol_terms){
		const std::string&code=kv.first;
		const SolarTerm&info=kv.second;
		out.push_back(mk_erec("solar_term",code,info.name,yr.year,
							  info.datetime.toUtcJD(),tz_off));
	}
	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return out;
}

std::vector<EventRec> bld_lpev(const YearResult&yr,int tz_off){
	static const std::array<std::pair<const char*,const char*>,4> kDefs={{
		{"new_moon","\u6714"},
		{"fst_qtr","\u4e0a\u5f26"},
		{"full_moon","\u671b"},
		{"lst_qtr","\u4e0b\u5f26"},
	}};

	std::vector<EventRec> out;
	out.reserve(yr.lun_phase.size()*kDefs.size());
	for(const auto&item : yr.lun_phase){
		const LocalDT dts[]={item.new_moon,item.fst_qtr,item.full_moon,
							 item.lst_qtr};
		for(std::size_t i=0;i<kDefs.size();++i){
			out.push_back(mk_erec("lunar_phase",kDefs[i].first,kDefs[i].second,
								  yr.year,dts[i].toUtcJD(),tz_off));
		}
	}
	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	return out;
}

} // namespace cli_util
