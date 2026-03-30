#include "lunar/solar_zodiac.hpp"

namespace{

struct ZodOpt{
	std::string ephem;
	std::string time_raw;
	int year=0;
	bool has_time=false;
	bool has_year=false;
	std::string input_tz;
	std::string tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
};

struct ZodRes{
	int tz_off=0;
	std::string tz_in;
	SolarZodiacPoint point;
	SolarZodiacYearSummary year_sum;
};

double days_from_sec(double seconds){ return seconds/SEC_DAY; }

std::string zodiac_name(const std::string&code){
	return lunar::i18n::tr_solar_zodiac_name(code,code);
}

void write_zod_meta(JsonWriter&w,const std::string&ephem,
					const std::string&tz,const std::string&mode,bool year_mode){
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
	w.key("type");
	w.value("zodiac");
	w.key("mode");
	w.value(mode);
	w.key("tz_display");
	w.value(tz);
	w.key("notes");
	w.arr_begin();
	w.value(lunar::i18n::pick("太阳星座按地心太阳视黄经计算，已含光行时修正。",
							  "Solar zodiac uses apparent geocentric solar "
							  "ecliptic longitude with light-time correction.",
							  "太陽星座は光行時補正込みの地心太陽視黄経で計算します。",
							  "?? ???? ??? ??? ??? ?? ?? ????? ?????."));
	if(year_mode){
		w.value(lunar::i18n::pick(
			"zodiac --year 模式下，--tz 同时定义显示时区与公历年裁剪窗口。",
			"In zodiac --year mode, --tz defines both the display timezone "
			"and the civil-year clipping window.",
			"zodiac --year では --tz が表示タイムゾーンと暦年切り出し窓を兼ねます。",
			"zodiac --year ???? --tz ? ?? ????? ?? ?? ?? ??? ?? ?????."));
	}else{
		w.value(lunar::i18n::pick(
			"zodiac --time 模式下，--tz 仅影响显示，不改变星座判定结果。",
			"In zodiac --time mode, --tz affects display only and does not "
			"change the sign result.",
			"zodiac --time では --tz は表示のみに影響し、星座判定は変わりません。",
			"zodiac --time ???? --tz ? ??? ??? ??? ??? ??? ????."));
	}
	w.arr_end();
	w.obj_end();
}

void wr_interval_json(JsonWriter&w,const SolarZodiacYearInterval&item,int tz_off){
	const SolarZodiacDef&def=solar_zodiac_def(item.sign_index);
	w.obj_begin();
	w.key("sign_index");
	w.value(item.sign_index);
	w.key("sign_order");
	w.value(item.sign_index+1);
	w.key("sign_code");
	w.value(item.sign_code);
	w.key("sign_name");
	w.value(zodiac_name(item.sign_code));
	w.key("term_code");
	w.value(item.term_code);
	w.key("start_lambda_deg");
	w.value(def.start_lambda_rad*180.0/PI);
	w.key("end_lambda_deg");
	w.value(def.end_lambda_rad*180.0/PI);
	w.key("sign_start_jd_utc");
	w.value(item.sign_start_jd_utc);
	w.key("sign_end_jd_utc");
	w.value(item.sign_end_jd_utc);
	w.key("sign_start_utc_iso");
	w.value(fmt_iso(item.sign_start_jd_utc,0,true));
	w.key("sign_end_utc_iso");
	w.value(fmt_iso(item.sign_end_jd_utc,0,true));
	w.key("sign_start_loc_iso");
	w.value(fmt_iso(item.sign_start_jd_utc,tz_off,true));
	w.key("sign_end_loc_iso");
	w.value(fmt_iso(item.sign_end_jd_utc,tz_off,true));
	w.key("in_year_start_jd_utc");
	w.value(item.in_year_start_jd_utc);
	w.key("in_year_end_jd_utc");
	w.value(item.in_year_end_jd_utc);
	w.key("in_year_start_utc_iso");
	w.value(fmt_iso(item.in_year_start_jd_utc,0,true));
	w.key("in_year_end_utc_iso");
	w.value(fmt_iso(item.in_year_end_jd_utc,0,true));
	w.key("in_year_start_loc_iso");
	w.value(fmt_iso(item.in_year_start_jd_utc,tz_off,true));
	w.key("in_year_end_loc_iso");
	w.value(fmt_iso(item.in_year_end_jd_utc,tz_off,true));
	w.key("in_year_dur_sec");
	w.value(item.in_year_dur_sec);
	w.key("in_year_dur_days");
	w.value(days_from_sec(item.in_year_dur_sec));
	w.key("clipped_start");
	w.value(item.clipped_start);
	w.key("clipped_end");
	w.value(item.clipped_end);
	w.obj_end();
}

ZodOpt parse_zod(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument(
			"zodiac requires: <bsp> (--time <time> | --year <year>)");
	}
	InterCfg cfg=load_def();
	ZodOpt opt;
	opt.ephem=args[0];
	opt.input_tz=cfg.default_tz;
	opt.tz=cfg.default_tz;
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"){
		opt.format="txt";
	}
	opt.pretty=cfg.def_prety;
	bool has_input_tz=false;

	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		opt.time_raw=args[i];
		opt.has_time=true;
		++i;
	}
	const OptMap handlers={
		{"--time",[&](const std::vector<std::string>&src,std::size_t&j,
					  const std::string&tag){
			 opt.time_raw=req_val(src,j,tag);
			 opt.has_time=true;
		 }},
		{"--year",[&](const std::vector<std::string>&src,std::size_t&j,
					  const std::string&tag){
			 opt.year=parse_int(req_val(src,j,tag),"--year");
			 opt.has_year=true;
		 }},
		{"--input-tz",[&](const std::vector<std::string>&src,std::size_t&j,
						  const std::string&tag){
			 opt.input_tz=req_val(src,j,tag);
			 has_input_tz=true;
		 }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&j,
					const std::string&tag){ opt.tz=req_val(src,j,tag); }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&j,
						const std::string&tag){
			 opt.format=to_low(req_val(src,j,tag));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&j,
					 const std::string&tag){ opt.out_path=req_val(src,j,tag); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&j,
						const std::string&tag){
			 opt.pretty=parse_bool01(req_val(src,j,tag),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ opt.quiet=true; }},
	};
	for(;i<args.size();++i){
		apply_opt(handlers,args,i,args[i],"zodiac");
	}
	if(opt.has_time==opt.has_year){
		throw std::invalid_argument(
			"zodiac requires exactly one of --time <time> or --year <year>");
	}
	if(opt.has_year&&has_input_tz){
		throw std::invalid_argument(
			"zodiac --input-tz is only valid together with --time");
	}
	chk_fmt(opt.format,{"json","txt","csv"},"zodiac");
	return opt;
}

ZodRes run_zod(const ZodOpt&opt){
	ZodRes res;
	res.tz_off=parse_tz(opt.tz);
	EphRead eph(opt.ephem);
	if(opt.has_time){
		IsoTime parsed=parse_iso(opt.time_raw,opt.input_tz);
		res.tz_in=
			parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(opt.input_tz));
		res.point=calc_solar_zodiac_at(eph,parsed.jd_utc);
	}else{
		res.year_sum=calc_solar_zodiac_year(eph,opt.year,res.tz_off);
	}
	return res;
}

void write_zod(std::ostream&os,const ZodOpt&opt,const ZodRes&res){
	if(opt.has_time){
		const SolarZodiacPoint&pt=res.point;
		const SolarZodiacDef&def=solar_zodiac_def(pt.sign_index);
		const FmtMap fmts={
			{"json",[&](){
				 JsonWriter w(os,opt.pretty);
				 w.obj_begin();
				 write_zod_meta(w,opt.ephem,opt.tz,"point",false);
				 w.key("input");
				 w.obj_begin();
				 w.key("time_raw");
				 w.value(opt.time_raw);
				 w.key("input_tz");
				 w.value(res.tz_in);
				 w.key("display_tz");
				 w.value(opt.tz);
				 w.key("jd_utc");
				 w.value(pt.jd_utc);
				 w.key("jd_tdb");
				 w.value(pt.jd_tdb);
				 w.key("utc_iso");
				 w.value(fmt_iso(pt.jd_utc,0,true));
				 w.key("loc_iso");
				 w.value(fmt_iso(pt.jd_utc,res.tz_off,true));
				 w.obj_end();
				 w.key("data");
				 w.obj_begin();
				 w.key("sun_lam");
				 w.value(pt.sun_lam_rad);
				 w.key("sun_lam_deg");
				 w.value(pt.sun_lam_deg);
				 w.key("sign_index");
				 w.value(pt.sign_index);
				 w.key("sign_order");
				 w.value(pt.sign_index+1);
				 w.key("sign_code");
				 w.value(pt.sign_code);
				 w.key("sign_name");
				 w.value(zodiac_name(pt.sign_code));
				 w.key("term_code");
				 w.value(pt.term_code);
				 w.key("start_lambda_deg");
				 w.value(def.start_lambda_rad*180.0/PI);
				 w.key("end_lambda_deg");
				 w.value(def.end_lambda_rad*180.0/PI);
				 w.key("sign_offset_rad");
				 w.value(pt.sign_offset_rad);
				 w.key("sign_offset_deg");
				 w.value(pt.sign_offset_deg);
				 w.key("sign_start_jd_utc");
				 w.value(pt.sign_start_jd_utc);
				 w.key("sign_end_jd_utc");
				 w.value(pt.sign_end_jd_utc);
				 w.key("sign_start_utc_iso");
				 w.value(fmt_iso(pt.sign_start_jd_utc,0,true));
				 w.key("sign_end_utc_iso");
				 w.value(fmt_iso(pt.sign_end_jd_utc,0,true));
				 w.key("sign_start_loc_iso");
				 w.value(fmt_iso(pt.sign_start_jd_utc,res.tz_off,true));
				 w.key("sign_end_loc_iso");
				 w.value(fmt_iso(pt.sign_end_jd_utc,res.tz_off,true));
				 w.key("elapsed_sec");
				 w.value(pt.elapsed_sec);
				 w.key("elapsed_days");
				 w.value(days_from_sec(pt.elapsed_sec));
				 w.key("remain_sec");
				 w.value(pt.remain_sec);
				 w.key("remain_days");
				 w.value(days_from_sec(pt.remain_sec));
				 w.key("span_sec");
				 w.value(pt.span_sec);
				 w.key("span_days");
				 w.value(days_from_sec(pt.span_sec));
				 w.obj_end();
				 w.obj_end();
				 os<<"\n";
			 }},
			{"csv",[&](){
				 os<<"jd_utc,jd_tdb,utc_iso,loc_iso,sun_lam,sun_lam_deg,"
					  "sign_index,sign_order,sign_code,sign_name,term_code,"
					  "start_lambda_deg,end_lambda_deg,sign_offset_rad,"
					  "sign_offset_deg,sign_start_jd_utc,sign_end_jd_utc,"
					  "sign_start_utc_iso,sign_end_utc_iso,sign_start_loc_iso,"
					  "sign_end_loc_iso,elapsed_sec,elapsed_days,remain_sec,"
					  "remain_days,span_sec,span_days\n";
				 os<<format_num(pt.jd_utc)<<","<<format_num(pt.jd_tdb)<<","
				   <<csv_quote(fmt_iso(pt.jd_utc,0,true))<<","
				   <<csv_quote(fmt_iso(pt.jd_utc,res.tz_off,true))<<","
				   <<format_num(pt.sun_lam_rad)<<","
				   <<format_num(pt.sun_lam_deg)<<","
				   <<pt.sign_index<<","<<(pt.sign_index+1)<<","
				   <<csv_quote(pt.sign_code)<<","
				   <<csv_quote(zodiac_name(pt.sign_code))<<","
				   <<csv_quote(pt.term_code)<<","
				   <<format_num(def.start_lambda_rad*180.0/PI)<<","
				   <<format_num(def.end_lambda_rad*180.0/PI)<<","
				   <<format_num(pt.sign_offset_rad)<<","
				   <<format_num(pt.sign_offset_deg)<<","
				   <<format_num(pt.sign_start_jd_utc)<<","
				   <<format_num(pt.sign_end_jd_utc)<<","
				   <<csv_quote(fmt_iso(pt.sign_start_jd_utc,0,true))<<","
				   <<csv_quote(fmt_iso(pt.sign_end_jd_utc,0,true))<<","
				   <<csv_quote(fmt_iso(pt.sign_start_jd_utc,res.tz_off,true))<<","
				   <<csv_quote(fmt_iso(pt.sign_end_jd_utc,res.tz_off,true))<<","
				   <<format_num(pt.elapsed_sec)<<","
				   <<format_num(days_from_sec(pt.elapsed_sec))<<","
				   <<format_num(pt.remain_sec)<<","
				   <<format_num(days_from_sec(pt.remain_sec))<<","
				   <<format_num(pt.span_sec)<<","
				   <<format_num(days_from_sec(pt.span_sec))<<"\n";
			 }},
			{"txt",[&](){
				 os<<"tool=lunar format=txt type=zodiac mode=point tz_display="
				   <<opt.tz<<"\n";
				 os<<"input.time_raw="<<opt.time_raw<<"\n";
				 os<<"input.input_tz="<<res.tz_in<<"\n";
				 os<<"input.display_tz="<<opt.tz<<"\n";
				 os<<"input.jd_utc="<<format_num(pt.jd_utc)<<"\n";
				 os<<"input.jd_tdb="<<format_num(pt.jd_tdb)<<"\n";
				 os<<"input.utc_iso="<<fmt_iso(pt.jd_utc,0,true)<<"\n";
				 os<<"input.loc_iso="<<fmt_iso(pt.jd_utc,res.tz_off,true)<<"\n";
				 os<<"data.sun_lam="<<format_num(pt.sun_lam_rad)<<"\n";
				 os<<"data.sun_lam_deg="<<format_num(pt.sun_lam_deg)<<"\n";
				 os<<"data.sign_index="<<pt.sign_index<<"\n";
				 os<<"data.sign_order="<<(pt.sign_index+1)<<"\n";
				 os<<"data.sign_code="<<pt.sign_code<<"\n";
				 os<<"data.sign_name="<<zodiac_name(pt.sign_code)<<"\n";
				 os<<"data.term_code="<<pt.term_code<<"\n";
				 os<<"data.start_lambda_deg="
				   <<format_num(def.start_lambda_rad*180.0/PI)<<"\n";
				 os<<"data.end_lambda_deg="
				   <<format_num(def.end_lambda_rad*180.0/PI)<<"\n";
				 os<<"data.sign_offset_rad="<<format_num(pt.sign_offset_rad)<<"\n";
				 os<<"data.sign_offset_deg="<<format_num(pt.sign_offset_deg)<<"\n";
				 os<<"data.sign_start_jd_utc="
				   <<format_num(pt.sign_start_jd_utc)<<"\n";
				 os<<"data.sign_end_jd_utc="
				   <<format_num(pt.sign_end_jd_utc)<<"\n";
				 os<<"data.sign_start_utc_iso="
				   <<fmt_iso(pt.sign_start_jd_utc,0,true)<<"\n";
				 os<<"data.sign_end_utc_iso="
				   <<fmt_iso(pt.sign_end_jd_utc,0,true)<<"\n";
				 os<<"data.sign_start_loc_iso="
				   <<fmt_iso(pt.sign_start_jd_utc,res.tz_off,true)<<"\n";
				 os<<"data.sign_end_loc_iso="
				   <<fmt_iso(pt.sign_end_jd_utc,res.tz_off,true)<<"\n";
				 os<<"data.elapsed_sec="<<format_num(pt.elapsed_sec)<<"\n";
				 os<<"data.elapsed_days="
				   <<format_num(days_from_sec(pt.elapsed_sec))<<"\n";
				 os<<"data.remain_sec="<<format_num(pt.remain_sec)<<"\n";
				 os<<"data.remain_days="
				   <<format_num(days_from_sec(pt.remain_sec))<<"\n";
				 os<<"data.span_sec="<<format_num(pt.span_sec)<<"\n";
				 os<<"data.span_days="<<format_num(days_from_sec(pt.span_sec))
				   <<"\n";
			 }},
		};
		run_fmt(fmts,opt.format,"zodiac");
		return;
	}

	const SolarZodiacYearSummary&sum=res.year_sum;
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 write_zod_meta(w,opt.ephem,opt.tz,"year",true);
			 w.key("input");
			 w.obj_begin();
			 w.key("year");
			 w.value(opt.year);
			 w.key("display_tz");
			 w.value(opt.tz);
			 w.key("year_start_jd_utc");
			 w.value(sum.year_start_jd_utc);
			 w.key("year_end_jd_utc");
			 w.value(sum.year_end_jd_utc);
			 w.key("year_start_utc_iso");
			 w.value(fmt_iso(sum.year_start_jd_utc,0,true));
			 w.key("year_end_utc_iso");
			 w.value(fmt_iso(sum.year_end_jd_utc,0,true));
			 w.key("year_start_loc_iso");
			 w.value(fmt_iso(sum.year_start_jd_utc,res.tz_off,true));
			 w.key("year_end_loc_iso");
			 w.value(fmt_iso(sum.year_end_jd_utc,res.tz_off,true));
			 w.obj_end();
			 w.key("data");
			 w.obj_begin();
			 w.key("interval_count");
			 w.value(static_cast<int>(sum.intervals.size()));
			 w.key("intervals");
			 w.arr_begin();
			 for(const auto&item : sum.intervals){
				 wr_interval_json(w,item,res.tz_off);
			 }
			 w.arr_end();
			 w.obj_end();
			 w.obj_end();
			 os<<"\n";
		 }},
		{"csv",[&](){
			 os<<"year,sign_index,sign_order,sign_code,sign_name,term_code,"
				  "start_lambda_deg,end_lambda_deg,sign_start_jd_utc,"
				  "sign_end_jd_utc,sign_start_utc_iso,sign_end_utc_iso,"
				  "sign_start_loc_iso,sign_end_loc_iso,in_year_start_jd_utc,"
				  "in_year_end_jd_utc,in_year_start_utc_iso,in_year_end_utc_iso,"
				  "in_year_start_loc_iso,in_year_end_loc_iso,in_year_dur_sec,"
				  "in_year_dur_days,clipped_start,clipped_end\n";
			 for(const auto&item : sum.intervals){
				 const SolarZodiacDef&def=solar_zodiac_def(item.sign_index);
				 os<<opt.year<<","<<item.sign_index<<","<<(item.sign_index+1)<<","
				   <<csv_quote(item.sign_code)<<","
				   <<csv_quote(zodiac_name(item.sign_code))<<","
				   <<csv_quote(item.term_code)<<","
				   <<format_num(def.start_lambda_rad*180.0/PI)<<","
				   <<format_num(def.end_lambda_rad*180.0/PI)<<","
				   <<format_num(item.sign_start_jd_utc)<<","
				   <<format_num(item.sign_end_jd_utc)<<","
				   <<csv_quote(fmt_iso(item.sign_start_jd_utc,0,true))<<","
				   <<csv_quote(fmt_iso(item.sign_end_jd_utc,0,true))<<","
				   <<csv_quote(fmt_iso(item.sign_start_jd_utc,res.tz_off,true))
				   <<","
				   <<csv_quote(fmt_iso(item.sign_end_jd_utc,res.tz_off,true))
				   <<","
				   <<format_num(item.in_year_start_jd_utc)<<","
				   <<format_num(item.in_year_end_jd_utc)<<","
				   <<csv_quote(fmt_iso(item.in_year_start_jd_utc,0,true))<<","
				   <<csv_quote(fmt_iso(item.in_year_end_jd_utc,0,true))<<","
				   <<csv_quote(fmt_iso(item.in_year_start_jd_utc,res.tz_off,true))
				   <<","
				   <<csv_quote(fmt_iso(item.in_year_end_jd_utc,res.tz_off,true))
				   <<","
				   <<format_num(item.in_year_dur_sec)<<","
				   <<format_num(days_from_sec(item.in_year_dur_sec))<<","
				   <<(item.clipped_start?"1":"0")<<","
				   <<(item.clipped_end?"1":"0")<<"\n";
			 }
		 }},
		{"txt",[&](){
			 os<<"tool=lunar format=txt type=zodiac mode=year tz_display="
			   <<opt.tz<<"\n";
			 os<<"input.year="<<opt.year<<"\n";
			 os<<"input.year_start_jd_utc="<<format_num(sum.year_start_jd_utc)
			   <<"\n";
			 os<<"input.year_end_jd_utc="<<format_num(sum.year_end_jd_utc)
			   <<"\n";
			 os<<"input.year_start_utc_iso="<<fmt_iso(sum.year_start_jd_utc,0,true)
			   <<"\n";
			 os<<"input.year_end_utc_iso="<<fmt_iso(sum.year_end_jd_utc,0,true)
			   <<"\n";
			 os<<"input.year_start_loc_iso="
			   <<fmt_iso(sum.year_start_jd_utc,res.tz_off,true)<<"\n";
			 os<<"input.year_end_loc_iso="
			   <<fmt_iso(sum.year_end_jd_utc,res.tz_off,true)<<"\n";
			 os<<"sign_index\tsign_order\tsign_code\tsign_name\tterm_code\t"
				 "start_lambda_deg\tend_lambda_deg\tsign_start_utc_iso\t"
				 "sign_end_utc_iso\tin_year_start_loc_iso\tin_year_end_loc_iso\t"
				 "in_year_dur_sec\tin_year_dur_days\tclipped_start\tclipped_end\n";
			 for(const auto&item : sum.intervals){
				 const SolarZodiacDef&def=solar_zodiac_def(item.sign_index);
				 os<<item.sign_index<<"\t"<<(item.sign_index+1)<<"\t"
				   <<item.sign_code<<"\t"<<zodiac_name(item.sign_code)<<"\t"
				   <<item.term_code<<"\t"
				   <<format_num(def.start_lambda_rad*180.0/PI)<<"\t"
				   <<format_num(def.end_lambda_rad*180.0/PI)<<"\t"
				   <<fmt_iso(item.sign_start_jd_utc,0,true)<<"\t"
				   <<fmt_iso(item.sign_end_jd_utc,0,true)<<"\t"
				   <<fmt_iso(item.in_year_start_jd_utc,res.tz_off,true)<<"\t"
				   <<fmt_iso(item.in_year_end_jd_utc,res.tz_off,true)<<"\t"
				   <<format_num(item.in_year_dur_sec)<<"\t"
				   <<format_num(days_from_sec(item.in_year_dur_sec))<<"\t"
				   <<(item.clipped_start?"1":"0")<<"\t"
				   <<(item.clipped_end?"1":"0")<<"\n";
			 }
		 }},
	};
	run_fmt(fmts,opt.format,"zodiac");
}

}

int cmd_zodiac(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_zodiac();
		return 0;
	}
	ZodOpt opt=parse_zod(args);
	ZodRes res=run_zod(opt);
	OutTgt out=open_out(opt.out_path);
	write_zod(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
	return 0;
}
