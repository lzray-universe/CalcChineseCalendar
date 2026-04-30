#include "lunar/cli/common.hpp"

#include<algorithm>
#include<array>
#include<cctype>
#include<ctime>
#include<iostream>
#include<limits>
#include<stdexcept>

#include "lunar/format.hpp"
#include "lunar/i18n.hpp"
#include "lunar/math.hpp"
#include "lunar/time_scale.hpp"

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

std::tuple<int,int,int> parse_ymd_fixed(const std::string&text,
										const std::string&label){
	const std::string err="invalid "+label+", expected YEAR-MM-DD: "+text;
	if(text.empty()){
		throw std::invalid_argument(err);
	}
	const std::size_t year_sep=
		text.find('-',((text[0]=='+'||text[0]=='-')?1u:0u));
	const std::size_t month_sep=
		(year_sep==std::string::npos)?
			std::string::npos:text.find('-',year_sep+1);
	if(year_sep==std::string::npos||month_sep==std::string::npos){
		throw std::invalid_argument(err);
	}
	const std::string ytxt=text.substr(0,year_sep);
	const std::string mtxt=text.substr(year_sep+1,month_sep-year_sep-1);
	const std::string dtxt=text.substr(month_sep+1);
	if(mtxt.size()!=2||dtxt.size()!=2||!all_digits(mtxt)||
	   !all_digits(dtxt)){
		throw std::invalid_argument(err);
	}
	const int y=parse_int(ytxt,"year");
	const int m=parse_int(mtxt,"month");
	const int d=parse_int(dtxt,"day");
	if(m<1||m>12||d<1||d>31){
		throw std::invalid_argument("invalid "+label+" value: "+text);
	}
	return {y,m,d};
}

double current_jd_utc(){
	const std::time_t now=std::time(nullptr);
	std::tm utc_tm{};
#if defined(_WIN32)
	gmtime_s(&utc_tm,&now);
#else
	gmtime_r(&now,&utc_tm);
#endif
	return greg2jd(utc_tm.tm_year+1900,utc_tm.tm_mon+1,utc_tm.tm_mday,
				   utc_tm.tm_hour,utc_tm.tm_min,
				   static_cast<double>(utc_tm.tm_sec));
}

std::string req_val(const std::vector<std::string>&args,std::size_t&idx,
					const std::string&opt){
	if(idx+1>=args.size()){
		throw std::invalid_argument("missing value for option: "+opt);
	}
	++idx;
	if(args[idx]=="-h"||args[idx]=="--help"||args[idx].rfind("--",0)==0){
		throw std::invalid_argument("missing value for option: "+opt);
	}
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
		std::cerr<<lunar::i18n::pick("已写入: ","written: ","出力先: ","저장됨: ")
				 <<path<<std::endl;
	}
}

void log_year_progress(std::ostream*log,int year){
	if(log==nullptr){
		return;
	}
	(*log)<<lunar::i18n::pick("进度: 正在计算年份 ","progress: computing year ",
							  "進行中: 計算年 ","진행: 계산 연도 ")
		  <<year<<std::endl;
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
	rec.name=lunar::i18n::tr_event_name(kind,code,name);
	rec.year=year;
	rec.jd_tdb=jd_tdb;
	rec.jd_utc=jd_utc;
	rec.utc_iso=fmt_iso(jd_utc,0,true);
	rec.loc_iso=fmt_iso(jd_utc,tz_off,true);
	return rec;
}

EventRec mk_erec(const std::string&kind,const std::string&code,
				 const std::string&name,int year,double jd_utc,int tz_off){
	return mk_erec(kind,code,name,year,TimeScale::utc_to_tdb(jd_utc),jd_utc,
				   tz_off);
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

}
