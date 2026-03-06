#include "lunar/global_context.hpp"

#include<algorithm>
#include<cctype>
#include<cmath>
#include<filesystem>
#include<limits>
#include<optional>
#include<stdexcept>
#include<string>
#include<tuple>
#include<unordered_map>
#include<unordered_set>
#include<vector>

#include "lunar/arg_parser.hpp"
#include "lunar/cli.hpp"
#include "lunar/format.hpp"
#include "lunar/i18n.hpp"
#include "lunar/interact.hpp"
#include "lunar/math.hpp"
#include "lunar/spc_ephem.hpp"

namespace{

namespace fs=std::filesystem;

struct CovInfo{
	bool loaded=false;
	bool has_cov=false;
	double jd_start=std::numeric_limits<double>::quiet_NaN();
	double jd_end=std::numeric_limits<double>::quiet_NaN();
};

std::string to_low_copy(const std::string&s){
	std::string out=s;
	std::transform(out.begin(),out.end(),out.begin(),[](unsigned char c){
		return static_cast<char>(std::tolower(c));
	});
	return out;
}

std::string trim_copy(const std::string&s){
	std::size_t beg=0;
	while(beg<s.size()&&std::isspace(static_cast<unsigned char>(s[beg]))){
		++beg;
	}
	std::size_t end=s.size();
	while(end>beg&&std::isspace(static_cast<unsigned char>(s[end-1]))){
		--end;
	}
	return s.substr(beg,end-beg);
}

bool ends_with_bsp(const std::string&s){
	if(s.size()<4){
		return false;
	}
	std::string tail=to_low_copy(s.substr(s.size()-4));
	return tail==".bsp";
}

bool looks_like_bsp_token(const std::string&s){
	if(ends_with_bsp(s)){
		return true;
	}
	std::error_code ec;
	fs::path p=s;
	if(!fs::exists(p,ec)||!fs::is_regular_file(p,ec)){
		return false;
	}
	return to_low_copy(p.extension().string())==".bsp";
}

std::vector<std::string> split_bsp_list(const std::string&text){
	std::vector<std::string> out;
	std::string token;
	auto flush=[&](){
		std::string t=trim_copy(token);
		if(!t.empty()){
			out.push_back(t);
		}
		token.clear();
	};
	for(char ch : text){
		if(ch==';'||ch==','){
			flush();
		}else{
			token.push_back(ch);
		}
	}
	flush();
	return out;
}

void add_unique_existing(std::vector<std::string>&out,
						 std::unordered_set<std::string>&seen,
						 const std::string&path){
	if(path.empty()){
		return;
	}
	fs::path p=fs::path(path);
	std::error_code ec;
	if(!fs::exists(p,ec)||!fs::is_regular_file(p,ec)){
		return;
	}
	std::string norm=to_low_copy(fs::weakly_canonical(p,ec).string());
	if(norm.empty()){
		norm=to_low_copy(p.lexically_normal().string());
	}
	if(seen.insert(norm).second){
		out.push_back(p.string());
	}
}

std::optional<std::pair<int,int>> parse_ym(const std::string&s){
	std::size_t pos=s.find('-');
	if(pos==std::string::npos){
		return std::nullopt;
	}
	try{
		int y=std::stoi(s.substr(0,pos));
		int m=std::stoi(s.substr(pos+1));
		if(m<1||m>12){
			return std::nullopt;
		}
		return std::make_pair(y,m);
	}catch(...){
		return std::nullopt;
	}
}

std::optional<std::tuple<int,int,int>> parse_ymd_simple(const std::string&s){
	if(s.size()!=10||s[4]!='-'||s[7]!='-'){
		return std::nullopt;
	}
	try{
		int y=std::stoi(s.substr(0,4));
		int m=std::stoi(s.substr(5,2));
		int d=std::stoi(s.substr(8,2));
		if(m<1||m>12||d<1||d>31){
			return std::nullopt;
		}
		return std::make_tuple(y,m,d);
	}catch(...){
		return std::nullopt;
	}
}

std::optional<std::string> find_opt_val(const std::vector<std::string>&args,
										const std::string&opt){
	for(std::size_t i=0;i<args.size();++i){
		const std::string&arg=args[i];
		if(arg==opt){
			if(i+1<args.size()){
				return args[i+1];
			}
			return std::nullopt;
		}
		if(arg.rfind(opt+"=",0)==0){
			return arg.substr(opt.size()+1);
		}
	}
	return std::nullopt;
}

bool needs_bsp(const std::string&cmd){
	static const std::unordered_set<std::string> kNeed={
		"months","calendar","year","event","at","convert","day",
		"monthview","next","range","search","eclipse","festival",
		"almanac","info","selftest"};
	return kNeed.find(cmd)!=kNeed.end();
}

bool is_help_only(const std::vector<std::string>&args){
	return args.size()==1&&(args[0]=="-h"||args[0]=="--help");
}

std::optional<std::pair<double,double>> infer_jd_interval(
	const std::string&command,const std::vector<std::string>&args,
	const std::string&default_tz){
	try{
		if(command=="day"||command=="almanac"){
			if(args.empty()||lunar::ArgParser::is_opt(args[0])){
				return std::nullopt;
			}
			auto ymd=parse_ymd_simple(args[0]);
			if(!ymd){
				return std::nullopt;
			}
			int y=std::get<0>(*ymd);
			int m=std::get<1>(*ymd);
			int d=std::get<2>(*ymd);
			double start=greg2jd(y,m,d,0,0,0.0)-UTC8DAY;
			return std::make_pair(start,start+1.0);
		}
		if(command=="monthview"){
			if(args.empty()||lunar::ArgParser::is_opt(args[0])){
				return std::nullopt;
			}
			auto ym=parse_ym(args[0]);
			if(!ym){
				return std::nullopt;
			}
			int y=ym->first;
			int m=ym->second;
			int m2=(m==12)?1:(m+1);
			int y2=(m==12)?(y+1):y;
			double start=greg2jd(y,m,1,0,0,0.0)-UTC8DAY;
			double end=greg2jd(y2,m2,1,0,0,0.0)-UTC8DAY;
			return std::make_pair(start,end);
		}
		if(command=="at"){
			std::optional<std::string> t=find_opt_val(args,"--time");
			if(!t&&!args.empty()&&!lunar::ArgParser::is_opt(args[0])){
				t=args[0];
			}
			if(!t){
				return std::nullopt;
			}
			IsoTime p=parse_iso(*t,default_tz);
			return std::make_pair(p.jd_utc-0.5,p.jd_utc+0.5);
		}
		if(command=="convert"){
			if(find_opt_val(args,"--from-lunar")){
				return std::nullopt;
			}
			if(!args.empty()&&!lunar::ArgParser::is_opt(args[0])){
				IsoTime p=parse_iso(args[0],default_tz);
				return std::make_pair(p.jd_utc-1.0,p.jd_utc+1.0);
			}
			return std::nullopt;
		}
		if(command=="range"||command=="search"){
			auto from=find_opt_val(args,"--from");
			auto to=find_opt_val(args,"--to");
			if(from&&to){
				IsoTime f=parse_iso(*from,default_tz);
				IsoTime t=parse_iso(*to,default_tz);
				return std::make_pair(std::min(f.jd_utc,t.jd_utc),
									  std::max(f.jd_utc,t.jd_utc));
			}
			if(command=="search"&&from){
				IsoTime f=parse_iso(*from,default_tz);
				int count=1;
				if(auto c=find_opt_val(args,"--count")){
					count=std::max(1,std::stoi(*c));
				}
				double span=std::max(30.0,static_cast<double>(count)*45.0);
				return std::make_pair(f.jd_utc,f.jd_utc+span);
			}
			return std::nullopt;
		}
		if(command=="next"){
			auto from=find_opt_val(args,"--from");
			if(!from){
				return std::nullopt;
			}
			IsoTime f=parse_iso(*from,default_tz);
			int count=1;
			if(auto c=find_opt_val(args,"--count")){
				count=std::max(1,std::stoi(*c));
			}
			double span=std::max(45.0,static_cast<double>(count)*50.0);
			return std::make_pair(f.jd_utc,f.jd_utc+span);
		}
		if(command=="eclipse"){
			auto near=find_opt_val(args,"--near");
			if(!near){
				return std::nullopt;
			}
			auto ymd=parse_ymd_simple(*near);
			if(!ymd){
				return std::nullopt;
			}
			int y=std::get<0>(*ymd);
			int m=std::get<1>(*ymd);
			int d=std::get<2>(*ymd);
			double jd=greg2jd(y,m,d,0,0,0.0)-UTC8DAY;
			return std::make_pair(jd-45.0,jd+45.0);
		}
		if(command=="festival"){
			if(args.empty()||lunar::ArgParser::is_opt(args[0])){
				return std::nullopt;
			}
			int y=std::stoi(args[0]);
			double start=greg2jd(y,1,1,0,0,0.0)-UTC8DAY;
			double end=greg2jd(y+2,1,1,0,0,0.0)-UTC8DAY;
			return std::make_pair(start,end);
		}
		if(command=="year"){
			if(args.empty()||lunar::ArgParser::is_opt(args[0])){
				return std::nullopt;
			}
			int y=std::stoi(args[0]);
			double start=greg2jd(y,1,1,0,0,0.0)-UTC8DAY;
			double end=greg2jd(y+2,1,1,0,0,0.0)-UTC8DAY;
			return std::make_pair(start,end);
		}
		if(command=="calendar"||command=="months"){
			if(args.empty()||lunar::ArgParser::is_opt(args[0])){
				return std::nullopt;
			}
			std::vector<int> years=parse_year(args[0]);
			if(years.empty()){
				return std::nullopt;
			}
			auto mm=std::minmax_element(years.begin(),years.end());
			double start=greg2jd(*mm.first,1,1,0,0,0.0)-UTC8DAY;
			double end=greg2jd(*mm.second+2,1,1,0,0,0.0)-UTC8DAY;
			return std::make_pair(start,end);
		}
		if(command=="event"){
			if(args.empty()){
				return std::nullopt;
			}
			std::string cat=to_low_copy(args[0]);
			if(cat=="solar-term"&&args.size()>=3){
				int y=std::stoi(args[2]);
				double start=greg2jd(y,1,1,0,0,0.0)-UTC8DAY;
				double end=greg2jd(y+2,1,1,0,0,0.0)-UTC8DAY;
				return std::make_pair(start,end);
			}
			if(cat=="lunar-phase"){
				auto near=find_opt_val(args,"--near");
				if(!near){
					return std::nullopt;
				}
				IsoTime p=parse_iso(*near,default_tz);
				return std::make_pair(p.jd_utc-45.0,p.jd_utc+45.0);
			}
			if(cat=="lunar-eclipse"||cat=="lunar_eclipse"||cat=="solar-eclipse"||
			   cat=="solar_eclipse"){
				auto near=find_opt_val(args,"--near");
				if(!near){
					return std::nullopt;
				}
				auto ymd=parse_ymd_simple(*near);
				if(!ymd){
					return std::nullopt;
				}
				int y=std::get<0>(*ymd);
				int m=std::get<1>(*ymd);
				int d=std::get<2>(*ymd);
				double jd=greg2jd(y,m,d,0,0,0.0)-UTC8DAY;
				return std::make_pair(jd-45.0,jd+45.0);
			}
			return std::nullopt;
		}
		return std::nullopt;
	}catch(...){
		return std::nullopt;
	}
}

CovInfo load_cov_cached(const std::string&path){
	static std::unordered_map<std::string,CovInfo> cache;
	auto it=cache.find(path);
	if(it!=cache.end()){
		return it->second;
	}
	CovInfo rec;
	rec.loaded=true;
	try{
		EphRead eph(path);
		eph.load_kern();
		std::vector<int> ids=eph.spk_objects();
		if(!ids.empty()){
			double min_et=std::numeric_limits<double>::infinity();
			double max_et=-std::numeric_limits<double>::infinity();
			for(int id : ids){
				std::vector<std::pair<double,double>> cov=eph.spk_coverage(id);
				for(const auto&span : cov){
					min_et=std::min(min_et,span.first);
					max_et=std::max(max_et,span.second);
				}
			}
			if(std::isfinite(min_et)&&std::isfinite(max_et)&&min_et<max_et){
				rec.has_cov=true;
				rec.jd_start=2451545.0+min_et/SEC_DAY;
				rec.jd_end=2451545.0+max_et/SEC_DAY;
			}
		}
	}catch(...){
		rec.has_cov=false;
	}
	cache[path]=rec;
	return rec;
}

std::string choose_bsp(const std::vector<std::string>&candidates,
					   const std::optional<std::pair<double,double>>&interval){
	if(candidates.empty()){
		return "";
	}
	if(!interval){
		return candidates.front();
	}
	double q0=interval->first;
	double q1=interval->second;
	double best_overlap=-1.0;
	std::string best=candidates.front();
	for(const auto&path : candidates){
		CovInfo cov=load_cov_cached(path);
		if(!cov.has_cov){
			continue;
		}
		if(cov.jd_start<=q0&&cov.jd_end>=q1){
			return path;
		}
		double ov=std::max(0.0,std::min(cov.jd_end,q1)-std::max(cov.jd_start,q0));
		if(ov>best_overlap){
			best_overlap=ov;
			best=path;
		}
	}
	return best;
}

}

GlobalContext load_global_ctx(){
	GlobalContext out;
	InterCfg cfg;
	load_cfg(cfg);
	out.default_tz=cfg.default_tz.empty()?"+08:00":cfg.default_tz;
	out.default_format=cfg.def_fmt.empty()?"txt":cfg.def_fmt;
	out.default_pretty=cfg.def_prety;

	std::vector<std::string> list;
	std::unordered_set<std::string> seen;
	if(!cfg.def_bsp.empty()){
		std::vector<std::string> def_items=split_bsp_list(cfg.def_bsp);
		if(def_items.empty()){
			add_unique_existing(list,seen,cfg.def_bsp);
		}else{
			for(const auto&item : def_items){
				add_unique_existing(list,seen,item);
			}
		}
	}
	for(const auto&item : cfg.bsp_list){
		add_unique_existing(list,seen,item);
	}
	if(!cfg.bsp_dir.empty()){
		std::error_code ec;
		fs::path d=cfg.bsp_dir;
		if(fs::exists(d,ec)&&fs::is_directory(d,ec)){
			std::vector<fs::path> local;
			for(const auto&ent : fs::directory_iterator(d,ec)){
				if(ent.is_regular_file()&&
				   to_low_copy(ent.path().extension().string())==".bsp"){
					local.push_back(ent.path());
				}
			}
			std::sort(local.begin(),local.end());
			for(const auto&p : local){
				add_unique_existing(list,seen,p.string());
			}
		}
	}
	{
		std::error_code ec;
		fs::path cwd=fs::current_path(ec);
		if(!ec&&fs::exists(cwd,ec)&&fs::is_directory(cwd,ec)){
			std::vector<fs::path> local;
			for(const auto&ent : fs::directory_iterator(cwd,ec)){
				if(ent.is_regular_file()&&
				   to_low_copy(ent.path().extension().string())==".bsp"){
					local.push_back(ent.path());
				}
			}
			std::sort(local.begin(),local.end());
			for(const auto&p : local){
				add_unique_existing(list,seen,p.string());
			}
		}
	}
	out.bsp_candidates=std::move(list);
	return out;
}

std::vector<std::string> prep_cmd_args(const std::string&command,
									   const std::vector<std::string>&args,
									   const GlobalContext&ctx){
	std::vector<std::string> stripped;
	stripped.reserve(args.size());
	std::string explicit_bsp;
	for(std::size_t i=0;i<args.size();++i){
		const std::string&arg=args[i];
		if(arg=="--bsp"){
			explicit_bsp=lunar::ArgParser::require_value(args,i,arg);
			continue;
		}
		if(arg.rfind("--bsp=",0)==0){
			explicit_bsp=arg.substr(6);
			continue;
		}
		stripped.push_back(arg);
	}

	if(!needs_bsp(command)){
		return stripped;
	}
	if(is_help_only(stripped)){
		return stripped;
	}

	std::vector<std::string> rest=stripped;
	if(explicit_bsp.empty()&&!rest.empty()&&!lunar::ArgParser::is_opt(rest[0])&&
	   looks_like_bsp_token(rest[0])){
		explicit_bsp=rest[0];
		rest.erase(rest.begin());
	}

	std::string chosen=explicit_bsp;
	if(chosen.empty()){
		auto interval=infer_jd_interval(command,rest,ctx.default_tz);
		chosen=choose_bsp(ctx.bsp_candidates,interval);
	}
	if(chosen.empty()){
		throw std::invalid_argument(lunar::i18n::pick(
			"未找到可用 BSP。请使用 --bsp <path> 或 `lunar config set def_bsp <path>`",
			"no bsp found. use --bsp <path> or `lunar config set def_bsp <path>`",
			"BSP が見つかりません。--bsp <path> または `lunar config set def_bsp <path>` を使用してください",
			"사용 가능한 BSP 를 찾지 못했습니다. --bsp <path> 또는 `lunar config set def_bsp <path>` 를 사용하세요"));
	}

	std::vector<std::string> out;
	out.reserve(rest.size()+1);
	out.push_back(chosen);
	out.insert(out.end(),rest.begin(),rest.end());
	return out;
}
