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

void wr_zod_point_csv(CsvWriter&w,const SolarZodiacPoint&pt,
					  const SolarZodiacDef&def,int tz_off){
	w.write_raw("jd_utc",format_num(pt.jd_utc));
	w.write_raw("jd_tdb",format_num(pt.jd_tdb));
	w.write_field("utc_iso",fmt_iso(pt.jd_utc,0,true));
	w.write_field("loc_iso",fmt_iso(pt.jd_utc,tz_off,true));
	w.write_raw("sun_lam",format_num(pt.sun_lam_rad));
	w.write_raw("sun_lam_deg",format_num(pt.sun_lam_deg));
	w.write_field("sign_index",pt.sign_index);
	w.write_field("sign_order",pt.sign_index+1);
	w.write_field("sign_code",pt.sign_code);
	w.write_field("sign_name",zodiac_name(pt.sign_code));
	w.write_field("term_code",pt.term_code);
	w.write_raw("start_lambda_deg",format_num(def.start_lambda_rad*180.0/PI));
	w.write_raw("end_lambda_deg",format_num(def.end_lambda_rad*180.0/PI));
	w.write_raw("sign_offset_rad",format_num(pt.sign_offset_rad));
	w.write_raw("sign_offset_deg",format_num(pt.sign_offset_deg));
	w.write_raw("sign_start_jd_utc",format_num(pt.sign_start_jd_utc));
	w.write_raw("sign_end_jd_utc",format_num(pt.sign_end_jd_utc));
	w.write_field("sign_start_utc_iso",fmt_iso(pt.sign_start_jd_utc,0,true));
	w.write_field("sign_end_utc_iso",fmt_iso(pt.sign_end_jd_utc,0,true));
	w.write_field("sign_start_loc_iso",fmt_iso(pt.sign_start_jd_utc,tz_off,true));
	w.write_field("sign_end_loc_iso",fmt_iso(pt.sign_end_jd_utc,tz_off,true));
	w.write_raw("elapsed_sec",format_num(pt.elapsed_sec));
	w.write_raw("elapsed_days",format_num(days_from_sec(pt.elapsed_sec)));
	w.write_raw("remain_sec",format_num(pt.remain_sec));
	w.write_raw("remain_days",format_num(days_from_sec(pt.remain_sec)));
	w.write_raw("span_sec",format_num(pt.span_sec));
	w.write_raw("span_days",format_num(days_from_sec(pt.span_sec)));
	w.finish_row();
}

void wr_zod_year_csv(CsvWriter&w,int year,const SolarZodiacYearInterval&item,
					 int tz_off){
	const SolarZodiacDef&def=solar_zodiac_def(item.sign_index);
	w.write_field("year",year);
	w.write_field("sign_index",item.sign_index);
	w.write_field("sign_order",item.sign_index+1);
	w.write_field("sign_code",item.sign_code);
	w.write_field("sign_name",zodiac_name(item.sign_code));
	w.write_field("term_code",item.term_code);
	w.write_raw("start_lambda_deg",format_num(def.start_lambda_rad*180.0/PI));
	w.write_raw("end_lambda_deg",format_num(def.end_lambda_rad*180.0/PI));
	w.write_raw("sign_start_jd_utc",format_num(item.sign_start_jd_utc));
	w.write_raw("sign_end_jd_utc",format_num(item.sign_end_jd_utc));
	w.write_field("sign_start_utc_iso",fmt_iso(item.sign_start_jd_utc,0,true));
	w.write_field("sign_end_utc_iso",fmt_iso(item.sign_end_jd_utc,0,true));
	w.write_field("sign_start_loc_iso",
				  fmt_iso(item.sign_start_jd_utc,tz_off,true));
	w.write_field("sign_end_loc_iso",fmt_iso(item.sign_end_jd_utc,tz_off,true));
	w.write_raw("in_year_start_jd_utc",format_num(item.in_year_start_jd_utc));
	w.write_raw("in_year_end_jd_utc",format_num(item.in_year_end_jd_utc));
	w.write_field("in_year_start_utc_iso",
				  fmt_iso(item.in_year_start_jd_utc,0,true));
	w.write_field("in_year_end_utc_iso",fmt_iso(item.in_year_end_jd_utc,0,true));
	w.write_field("in_year_start_loc_iso",
				  fmt_iso(item.in_year_start_jd_utc,tz_off,true));
	w.write_field("in_year_end_loc_iso",
				  fmt_iso(item.in_year_end_jd_utc,tz_off,true));
	w.write_raw("in_year_dur_sec",format_num(item.in_year_dur_sec));
	w.write_raw("in_year_dur_days",format_num(days_from_sec(item.in_year_dur_sec)));
	w.write_field("clipped_start",item.clipped_start);
	w.write_field("clipped_end",item.clipped_end);
	w.finish_row();
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
	lunar::ArgParser parser;
	parser.add_value("--time",[&](const std::string&v){
			opt.time_raw=v;
			opt.has_time=true;
		})
		.add_value("--year",[&](const std::string&v){
			opt.year=parse_int(v,"--year");
			opt.has_year=true;
		})
		.add_value("--input-tz",[&](const std::string&v){
			opt.input_tz=v;
			has_input_tz=true;
		})
		.add_value("--tz",[&](const std::string&v){ opt.tz=v; })
		.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; });
	parser.parse_all(args,i,"zodiac");
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
				 CsvWriter csv(os);
				 wr_zod_point_csv(csv,pt,def,res.tz_off);
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
			 CsvWriter csv(os);
			 for(const auto&item : sum.intervals){
				 wr_zod_year_csv(csv,opt.year,item,res.tz_off);
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
