#include "lunar/solar_zodiac.hpp"

namespace{

std::string csv_quote(const std::string&s);

double days_from_sec(double seconds){ return seconds/SEC_DAY; }

std::string zodiac_name(const std::string&code){
	return lunar::i18n::tr_solar_zodiac_name(code,code);
}

void write_zodiac_meta(JsonWriter&w,const std::string&ephem,
					   const std::string&tz,const std::string&mode,
					   bool year_mode){
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
							  "태양 별자리는 광행시 보정을 포함한 지심 태양 시황경으로 계산합니다."));
	if(year_mode){
		w.value(lunar::i18n::pick(
			"zodiac --year 模式下，--tz 同时定义显示时区与公历年裁剪窗口。",
			"In zodiac --year mode, --tz defines both the display timezone "
			"and the civil-year clipping window.",
			"zodiac --year では --tz が表示タイムゾーンと暦年切り出し窓を兼ねます。",
			"zodiac --year 모드에서 --tz 는 표시 시간대이자 양력 연도 절단 구간을 함께 정의합니다."));
	}else{
		w.value(lunar::i18n::pick(
			"zodiac --time 模式下，--tz 仅影响显示，不改变星座判定结果。",
			"In zodiac --time mode, --tz affects display only and does not "
			"change the sign result.",
			"zodiac --time では --tz は表示のみに影響し、星座判定は変わりません。",
			"zodiac --time 모드에서 --tz 는 표시만 바꾸며 별자리 판정은 바뀌지 않습니다."));
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

}

int cmd_zodiac(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_zodiac();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"zodiac requires: <bsp> (--time <time> | --year <year>)");
	}

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string time_raw;
	int year=0;
	bool has_time=false;
	bool has_year=false;
	std::string input_tz=cfg.default_tz;
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	bool has_input_tz=false;

	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		time_raw=args[i];
		has_time=true;
		++i;
	}
	const OptMap handlers={
		{"--time",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 time_raw=req_val(src,idx,opt);
			 has_time=true;
		 }},
		{"--year",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 year=parse_int(req_val(src,idx,opt),"--year");
			 has_year=true;
		 }},
		{"--input-tz",[&](const std::vector<std::string>&src,std::size_t&idx,
						  const std::string&opt){
			 input_tz=req_val(src,idx,opt);
			 has_input_tz=true;
		 }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&idx,
					const std::string&opt){ tz=req_val(src,idx,opt); }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 format=to_low(req_val(src,idx,opt));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){ out_path=req_val(src,idx,opt); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 pretty=parse_bool01(req_val(src,idx,opt),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ quiet=true; }},
	};

	for(;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="-h"||opt=="--help"){
			use_zodiac();
			return 0;
		}
		apply_opt(handlers,args,i,opt,"zodiac");
	}

	if(has_time==has_year){
		throw std::invalid_argument(
			"zodiac requires exactly one of --time <time> or --year <year>");
	}
	if(has_year&&has_input_tz){
		throw std::invalid_argument(
			"zodiac --input-tz is only valid together with --time");
	}
	chk_fmt(format,{"json","txt","csv"},"zodiac");

	const int tz_off=parse_tz(tz);
	EphRead eph(ephem);
	OutTgt out=open_out(out_path);

	if(has_time){
		const IsoTime parsed=parse_iso(time_raw,input_tz);
		const SolarZodiacPoint point=calc_solar_zodiac_at(eph,parsed.jd_utc);
		const SolarZodiacDef&def=solar_zodiac_def(point.sign_index);
		const std::string tz_in=
			parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(input_tz));
		const FmtMap fmt_handlers={
			{"json",[&](){
				 JsonWriter w(*out.stream,pretty);
				 w.obj_begin();
				 write_zodiac_meta(w,ephem,tz,"point",false);
				 w.key("input");
				 w.obj_begin();
				 w.key("time_raw");
				 w.value(time_raw);
				 w.key("input_tz");
				 w.value(tz_in);
				 w.key("display_tz");
				 w.value(tz);
				 w.key("jd_utc");
				 w.value(point.jd_utc);
				 w.key("jd_tdb");
				 w.value(point.jd_tdb);
				 w.key("utc_iso");
				 w.value(fmt_iso(point.jd_utc,0,true));
				 w.key("loc_iso");
				 w.value(fmt_iso(point.jd_utc,tz_off,true));
				 w.obj_end();
				 w.key("data");
				 w.obj_begin();
				 w.key("sun_lam");
				 w.value(point.sun_lam_rad);
				 w.key("sun_lam_deg");
				 w.value(point.sun_lam_deg);
				 w.key("sign_index");
				 w.value(point.sign_index);
				 w.key("sign_order");
				 w.value(point.sign_index+1);
				 w.key("sign_code");
				 w.value(point.sign_code);
				 w.key("sign_name");
				 w.value(zodiac_name(point.sign_code));
				 w.key("term_code");
				 w.value(point.term_code);
				 w.key("start_lambda_deg");
				 w.value(def.start_lambda_rad*180.0/PI);
				 w.key("end_lambda_deg");
				 w.value(def.end_lambda_rad*180.0/PI);
				 w.key("sign_offset_rad");
				 w.value(point.sign_offset_rad);
				 w.key("sign_offset_deg");
				 w.value(point.sign_offset_deg);
				 w.key("sign_start_jd_utc");
				 w.value(point.sign_start_jd_utc);
				 w.key("sign_end_jd_utc");
				 w.value(point.sign_end_jd_utc);
				 w.key("sign_start_utc_iso");
				 w.value(fmt_iso(point.sign_start_jd_utc,0,true));
				 w.key("sign_end_utc_iso");
				 w.value(fmt_iso(point.sign_end_jd_utc,0,true));
				 w.key("sign_start_loc_iso");
				 w.value(fmt_iso(point.sign_start_jd_utc,tz_off,true));
				 w.key("sign_end_loc_iso");
				 w.value(fmt_iso(point.sign_end_jd_utc,tz_off,true));
				 w.key("elapsed_sec");
				 w.value(point.elapsed_sec);
				 w.key("elapsed_days");
				 w.value(days_from_sec(point.elapsed_sec));
				 w.key("remain_sec");
				 w.value(point.remain_sec);
				 w.key("remain_days");
				 w.value(days_from_sec(point.remain_sec));
				 w.key("span_sec");
				 w.value(point.span_sec);
				 w.key("span_days");
				 w.value(days_from_sec(point.span_sec));
				 w.obj_end();
				 w.obj_end();
				 *out.stream<<"\n";
			 }},
			{"csv",[&](){
				 *out.stream<<"jd_utc,jd_tdb,utc_iso,loc_iso,sun_lam,sun_lam_deg,"
							  "sign_index,sign_order,sign_code,sign_name,term_code,"
							  "start_lambda_deg,end_lambda_deg,sign_offset_rad,"
							  "sign_offset_deg,sign_start_jd_utc,sign_end_jd_utc,"
							  "sign_start_utc_iso,sign_end_utc_iso,"
							  "sign_start_loc_iso,sign_end_loc_iso,elapsed_sec,"
							  "elapsed_days,remain_sec,remain_days,span_sec,span_days\n";
				 *out.stream<<format_num(point.jd_utc)<<","
						   <<format_num(point.jd_tdb)<<","
						   <<csv_quote(fmt_iso(point.jd_utc,0,true))<<","
						   <<csv_quote(fmt_iso(point.jd_utc,tz_off,true))<<","
						   <<format_num(point.sun_lam_rad)<<","
						   <<format_num(point.sun_lam_deg)<<","
						   <<point.sign_index<<","<<(point.sign_index+1)<<","
						   <<csv_quote(point.sign_code)<<","
						   <<csv_quote(zodiac_name(point.sign_code))<<","
						   <<csv_quote(point.term_code)<<","
						   <<format_num(def.start_lambda_rad*180.0/PI)<<","
						   <<format_num(def.end_lambda_rad*180.0/PI)<<","
						   <<format_num(point.sign_offset_rad)<<","
						   <<format_num(point.sign_offset_deg)<<","
						   <<format_num(point.sign_start_jd_utc)<<","
						   <<format_num(point.sign_end_jd_utc)<<","
						   <<csv_quote(fmt_iso(point.sign_start_jd_utc,0,true))
						   <<","
						   <<csv_quote(fmt_iso(point.sign_end_jd_utc,0,true))
						   <<","
						   <<csv_quote(fmt_iso(point.sign_start_jd_utc,tz_off,true))
						   <<","
						   <<csv_quote(fmt_iso(point.sign_end_jd_utc,tz_off,true))
						   <<","
						   <<format_num(point.elapsed_sec)<<","
						   <<format_num(days_from_sec(point.elapsed_sec))<<","
						   <<format_num(point.remain_sec)<<","
						   <<format_num(days_from_sec(point.remain_sec))<<","
						   <<format_num(point.span_sec)<<","
						   <<format_num(days_from_sec(point.span_sec))<<"\n";
			 }},
			{"txt",[&](){
				 std::ostream&os=*out.stream;
				 os<<"tool=lunar format=txt type=zodiac mode=point tz_display="
				   <<tz<<"\n";
				 os<<"input.time_raw="<<time_raw<<"\n";
				 os<<"input.input_tz="<<tz_in<<"\n";
				 os<<"input.display_tz="<<tz<<"\n";
				 os<<"input.jd_utc="<<format_num(point.jd_utc)<<"\n";
				 os<<"input.jd_tdb="<<format_num(point.jd_tdb)<<"\n";
				 os<<"input.utc_iso="<<fmt_iso(point.jd_utc,0,true)<<"\n";
				 os<<"input.loc_iso="<<fmt_iso(point.jd_utc,tz_off,true)<<"\n";
				 os<<"data.sun_lam="<<format_num(point.sun_lam_rad)<<"\n";
				 os<<"data.sun_lam_deg="<<format_num(point.sun_lam_deg)<<"\n";
				 os<<"data.sign_index="<<point.sign_index<<"\n";
				 os<<"data.sign_order="<<(point.sign_index+1)<<"\n";
				 os<<"data.sign_code="<<point.sign_code<<"\n";
				 os<<"data.sign_name="<<zodiac_name(point.sign_code)<<"\n";
				 os<<"data.term_code="<<point.term_code<<"\n";
				 os<<"data.start_lambda_deg="
				   <<format_num(def.start_lambda_rad*180.0/PI)<<"\n";
				 os<<"data.end_lambda_deg="
				   <<format_num(def.end_lambda_rad*180.0/PI)<<"\n";
				 os<<"data.sign_offset_rad="
				   <<format_num(point.sign_offset_rad)<<"\n";
				 os<<"data.sign_offset_deg="
				   <<format_num(point.sign_offset_deg)<<"\n";
				 os<<"data.sign_start_jd_utc="
				   <<format_num(point.sign_start_jd_utc)<<"\n";
				 os<<"data.sign_end_jd_utc="
				   <<format_num(point.sign_end_jd_utc)<<"\n";
				 os<<"data.sign_start_utc_iso="
				   <<fmt_iso(point.sign_start_jd_utc,0,true)<<"\n";
				 os<<"data.sign_end_utc_iso="
				   <<fmt_iso(point.sign_end_jd_utc,0,true)<<"\n";
				 os<<"data.sign_start_loc_iso="
				   <<fmt_iso(point.sign_start_jd_utc,tz_off,true)<<"\n";
				 os<<"data.sign_end_loc_iso="
				   <<fmt_iso(point.sign_end_jd_utc,tz_off,true)<<"\n";
				 os<<"data.elapsed_sec="<<format_num(point.elapsed_sec)<<"\n";
				 os<<"data.elapsed_days="
				   <<format_num(days_from_sec(point.elapsed_sec))<<"\n";
				 os<<"data.remain_sec="<<format_num(point.remain_sec)<<"\n";
				 os<<"data.remain_days="
				   <<format_num(days_from_sec(point.remain_sec))<<"\n";
				 os<<"data.span_sec="<<format_num(point.span_sec)<<"\n";
				 os<<"data.span_days="
				   <<format_num(days_from_sec(point.span_sec))<<"\n";
				 os<<"note.algorithm="
				   <<lunar::i18n::pick(
						  "地心太阳视黄经，已含光行时修正。",
						  "Apparent geocentric solar ecliptic longitude with "
						  "light-time correction.",
						  "光行時補正込みの地心太陽視黄経です。",
						  "광행시 보정을 포함한 지심 태양 시황경입니다.")
				   <<"\n";
			 }},
		};
		run_fmt(fmt_handlers,format,"zodiac");
		note_out(out_path,quiet);
		return 0;
	}

	const SolarZodiacYearSummary summary=
		calc_solar_zodiac_year(eph,year,tz_off);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,pretty);
			 w.obj_begin();
			 write_zodiac_meta(w,ephem,tz,"year",true);
			 w.key("input");
			 w.obj_begin();
			 w.key("year");
			 w.value(year);
			 w.key("display_tz");
			 w.value(tz);
			 w.key("year_start_jd_utc");
			 w.value(summary.year_start_jd_utc);
			 w.key("year_end_jd_utc");
			 w.value(summary.year_end_jd_utc);
			 w.key("year_start_utc_iso");
			 w.value(fmt_iso(summary.year_start_jd_utc,0,true));
			 w.key("year_end_utc_iso");
			 w.value(fmt_iso(summary.year_end_jd_utc,0,true));
			 w.key("year_start_loc_iso");
			 w.value(fmt_iso(summary.year_start_jd_utc,tz_off,true));
			 w.key("year_end_loc_iso");
			 w.value(fmt_iso(summary.year_end_jd_utc,tz_off,true));
			 w.obj_end();
			 w.key("data");
			 w.obj_begin();
			 w.key("interval_count");
			 w.value(static_cast<int>(summary.intervals.size()));
			 w.key("intervals");
			 w.arr_begin();
			 for(const auto&item : summary.intervals){
				 wr_interval_json(w,item,tz_off);
			 }
			 w.arr_end();
			 w.obj_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"csv",[&](){
			 *out.stream<<"year,sign_index,sign_order,sign_code,sign_name,"
						  "term_code,start_lambda_deg,end_lambda_deg,"
						  "sign_start_jd_utc,sign_end_jd_utc,sign_start_utc_iso,"
						  "sign_end_utc_iso,sign_start_loc_iso,sign_end_loc_iso,"
						  "in_year_start_jd_utc,in_year_end_jd_utc,"
						  "in_year_start_utc_iso,in_year_end_utc_iso,"
						  "in_year_start_loc_iso,in_year_end_loc_iso,"
						  "in_year_dur_sec,in_year_dur_days,clipped_start,"
						  "clipped_end\n";
			 for(const auto&item : summary.intervals){
				 const SolarZodiacDef&def=solar_zodiac_def(item.sign_index);
				 *out.stream<<year<<","<<item.sign_index<<","
						   <<(item.sign_index+1)<<","
						   <<csv_quote(item.sign_code)<<","
						   <<csv_quote(zodiac_name(item.sign_code))<<","
						   <<csv_quote(item.term_code)<<","
						   <<format_num(def.start_lambda_rad*180.0/PI)<<","
						   <<format_num(def.end_lambda_rad*180.0/PI)<<","
						   <<format_num(item.sign_start_jd_utc)<<","
						   <<format_num(item.sign_end_jd_utc)<<","
						   <<csv_quote(fmt_iso(item.sign_start_jd_utc,0,true))
						   <<","
						   <<csv_quote(fmt_iso(item.sign_end_jd_utc,0,true))
						   <<","
						   <<csv_quote(fmt_iso(item.sign_start_jd_utc,tz_off,true))
						   <<","
						   <<csv_quote(fmt_iso(item.sign_end_jd_utc,tz_off,true))
						   <<","
						   <<format_num(item.in_year_start_jd_utc)<<","
						   <<format_num(item.in_year_end_jd_utc)<<","
						   <<csv_quote(fmt_iso(item.in_year_start_jd_utc,0,true))
						   <<","
						   <<csv_quote(fmt_iso(item.in_year_end_jd_utc,0,true))
						   <<","
						   <<csv_quote(fmt_iso(item.in_year_start_jd_utc,tz_off,true))
						   <<","
						   <<csv_quote(fmt_iso(item.in_year_end_jd_utc,tz_off,true))
						   <<","
						   <<format_num(item.in_year_dur_sec)<<","
						   <<format_num(days_from_sec(item.in_year_dur_sec))
						   <<","<<(item.clipped_start?"1":"0")<<","
						   <<(item.clipped_end?"1":"0")<<"\n";
			 }
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=zodiac mode=year tz_display="
			   <<tz<<"\n";
			 os<<"input.year="<<year<<"\n";
			 os<<"input.year_start_jd_utc="
			   <<format_num(summary.year_start_jd_utc)<<"\n";
			 os<<"input.year_end_jd_utc="
			   <<format_num(summary.year_end_jd_utc)<<"\n";
			 os<<"input.year_start_utc_iso="
			   <<fmt_iso(summary.year_start_jd_utc,0,true)<<"\n";
			 os<<"input.year_end_utc_iso="
			   <<fmt_iso(summary.year_end_jd_utc,0,true)<<"\n";
			 os<<"input.year_start_loc_iso="
			   <<fmt_iso(summary.year_start_jd_utc,tz_off,true)<<"\n";
			 os<<"input.year_end_loc_iso="
			   <<fmt_iso(summary.year_end_jd_utc,tz_off,true)<<"\n";
			 os<<"note.tz="
			   <<lunar::i18n::pick(
					  "--tz 同时定义显示时区与该年持续时间的裁剪窗口。",
					  "--tz defines both the display timezone and the year "
					  "clipping window for durations.",
					  "--tz は表示タイムゾーンと年内継続時間の切り出し窓を兼ねます。",
					  "--tz 는 표시 시간대이자 연내 지속시간 절단 구간을 함께 정의합니다.")
			   <<"\n";
			 os<<"sign_index\tsign_order\tsign_code\tsign_name\tterm_code\t"
				 "start_lambda_deg\tend_lambda_deg\tsign_start_utc_iso\t"
				 "sign_end_utc_iso\tin_year_start_loc_iso\t"
				 "in_year_end_loc_iso\tin_year_dur_sec\t"
				 "in_year_dur_days\tclipped_start\tclipped_end\n";
			 for(const auto&item : summary.intervals){
				 const SolarZodiacDef&def=solar_zodiac_def(item.sign_index);
				 os<<item.sign_index<<"\t"<<(item.sign_index+1)<<"\t"
				   <<item.sign_code<<"\t"<<zodiac_name(item.sign_code)<<"\t"
				   <<item.term_code<<"\t"
				   <<format_num(def.start_lambda_rad*180.0/PI)<<"\t"
				   <<format_num(def.end_lambda_rad*180.0/PI)<<"\t"
				   <<fmt_iso(item.sign_start_jd_utc,0,true)<<"\t"
				   <<fmt_iso(item.sign_end_jd_utc,0,true)<<"\t"
				   <<fmt_iso(item.in_year_start_jd_utc,tz_off,true)<<"\t"
				   <<fmt_iso(item.in_year_end_jd_utc,tz_off,true)<<"\t"
				   <<format_num(item.in_year_dur_sec)<<"\t"
				   <<format_num(days_from_sec(item.in_year_dur_sec))<<"\t"
				   <<(item.clipped_start?"1":"0")<<"\t"
				   <<(item.clipped_end?"1":"0")<<"\n";
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"zodiac");
	note_out(out_path,quiet);
	return 0;
}
