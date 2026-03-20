namespace lunar::core{

namespace{

std::string day_hex_mask64(std::uint64_t value){
	std::ostringstream oss;
	oss<<"0x"<<std::hex<<std::nouppercase<<std::setw(16)<<std::setfill('0')
	   <<value;
	return oss.str();
}

std::string day_join_code_pipe(const std::vector<int>&codes){
	std::string out;
	for(std::size_t i=0;i<codes.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=std::to_string(codes[i]);
	}
	return out;
}

std::string day_join_mask_hex_pipe(const std::array<std::uint64_t,2>&masks){
	std::string out;
	for(std::size_t i=0;i<masks.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=day_hex_mask64(masks[i]);
	}
	return out;
}

template<class Fn>
std::string day_join_hour_pipe(const std::vector<HliHour>&hours,Fn&&fn){
	std::string out;
	for(std::size_t i=0;i<hours.size();++i){
		if(i!=0){
			out+="|";
		}
		out+=fn(hours[i]);
	}
	return out;
}

}

void format_day_output(std::ostream&os,const DayResult&result,
					   const std::string&format,bool pretty){
	EphRead eph(result.ephem);
	int tz_off=parse_tz(result.tz);

	auto write_json=[&](bool json_pretty){
		JsonWriter w(os,json_pretty);
		w.obj_begin();
		write_meta(w,result.ephem,result.tz,
				   {"type=day",lunar::i18n::day_rule_note()});
		w.key("input");
		w.obj_begin();
		w.key("date");
		w.value(result.date_text);
		w.key("smp_time");
		w.value(result.at_time);
		w.key("smp_jdutc");
		w.value(result.at_data.jd_utc);
		w.key("lon_deg");
		w.value(result.hli_lon_deg);
		w.key("astro");
		w.value(result.inc_astro);
		w.key("astro_mode");
		if(result.inc_astro){
			w.value(result.astro_mode_text);
		}else{
			w.null_val();
		}
		w.key("astro_pick");
		if(result.inc_astro&&to_low(result.astro_mode_text)=="pick"){
			w.value(result.astro_pick_csv);
		}else{
			w.null_val();
		}
		w.key("astro_site");
		w.value(result.astro_obs.has_site);
		w.key("astro_lat_deg");
		if(result.astro_obs.has_site){
			w.value(result.astro_obs.lat_deg);
		}else{
			w.null_val();
		}
		w.key("astro_lon_deg");
		if(result.astro_obs.has_site){
			w.value(result.astro_obs.lon_deg);
		}else{
			w.null_val();
		}
		w.key("astro_height_m");
		if(result.astro_obs.has_site){
			w.value(result.astro_obs.h_m);
		}else{
			w.null_val();
		}
		w.obj_end();
		w.key("data");
		w.obj_begin();
		w.key("lunar_date");
		wr_ljson(w,result.at_data.lunar_date);
		w.key("huangli");
		wr_hli_json(w,result.at_data.hli);
		w.key("ill_pct");
		w.value(result.at_data.ill_pct);
		w.key("phase_name");
		w.value(result.at_data.phase_name);
		w.key("moon_xg");
		w.obj_begin();
		w.key("region");
		w.value(result.at_data.moon_xg.region);
		w.key("star_id");
		w.value(result.at_data.moon_xg.star_id);
		w.key("star_name");
		w.value(result.at_data.moon_xg.star_name);
		w.key("sep_deg");
		w.value(result.at_data.moon_xg.sep_deg);
		w.obj_end();
		w.key("smp_uiso");
		w.value(result.at_data.utc_iso);
		w.key("smp_liso");
		w.value(result.at_data.local_iso);
		w.key("events");
		w.arr_begin();
		for(const auto&ev : result.day_events){
			wr_ejson(w,ev,eph);
		}
		w.arr_end();
		w.key("astro_events");
		w.arr_begin();
		for(const auto&ev : result.astro_events){
			wr_ejson(w,ev,eph);
		}
		w.arr_end();
		w.obj_end();
		w.obj_end();
		os<<"\n";
	};

	if(format=="json"){
		write_json(pretty);
		return;
	}
	if(format=="jsonl"){
		write_json(false);
		return;
	}
	if(format=="csv"){
		std::vector<std::string> ev_names;
		ev_names.reserve(result.day_events.size());
		for(const auto&ev : result.day_events){
			ev_names.push_back(ev.name);
		}
		std::vector<std::string> astro_names;
		astro_names.reserve(result.astro_events.size());
		for(const auto&ev : result.astro_events){
			astro_names.push_back(ev.name);
		}
		std::string summary=join_pipe(ev_names);
		std::string astro_summary=join_pipe(astro_names);
		os<<"date,lun_label,lun_mlab,is_leap,lunar_"
			 "day,ill_pct,phase_name,moon_xg_region,moon_xg_star,"
			 "moon_xg_sep_deg,smp_tlociso,ev_sum,astro_ev_sum,"
			 "y_lun_gz,y_lchun_gz,y_rule_gz,m_gz,d_gz,h_gz,h_true_gz,rule_profile,rule_profile_code,"
			 "y_lun_index,y_lun_stem,y_lun_branch,y_lchun_index,y_lchun_stem,y_lchun_branch,"
			 "y_rule_index,y_rule_stem,y_rule_branch,m_gz_index,m_gz_stem,m_gz_branch,"
			 "d_gz_index,d_gz_stem,d_gz_branch,h_gz_index,h_gz_stem,h_gz_branch,"
			 "h_true_gz_index,h_true_gz_stem,h_true_gz_branch,"
			 "year_boundary,year_boundary_code,month_boundary,month_boundary_code,leap_month_mode,leap_month_mode_code,day_boundary,day_boundary_code,jianchu,jianchu_code,"
			 "bazi_clock,bazi_true,duty_god,duty_god_code,duty_is_yellow,duty_tag,clash,"
			 "chong_sha,zodiac_day,six_he,"
			 "three_he,pengzu,nayin,wuxing_day,fetal_god,meridian,"
			 "lucky_dir,wealth_dir,mascot_dir,sun_noble_dir,"
			 "moon_noble_dir,xiu28,xiu28_code,xiu28_mod28,xiu28_mod28_code,"
			 "xiu_star,yi_ji_level,yi_ji_rule,yi_ji_rule_code,"
			 "good_gods,bad_gods,yi,ji,"
			 "rule_profile_key,year_boundary_key,month_boundary_key,leap_month_mode_key,day_boundary_key,"
			 "duty_tag_code,clash_branch_code,sha_dir_code,zodiac_day_code,six_he_branch_code,"
			 "three_he_group_code,nayin_code,fetal_god_code,meridian_code,lucky_dir_code,"
			 "wealth_dir_code,mascot_dir_code,sun_noble_dir_code,moon_noble_dir_code,"
			 "good_god_codes,bad_god_codes,yi_codes,ji_codes,"
			 "good_god_mask_hex,bad_god_mask_hex,yi_mask_hex,ji_mask_hex,"
			 "hour_slots,hour_slot_indexes,hour_gzs,hour_gz_indexes,hour_lucks,hour_is_bad\n";
		os<<csv_quote(result.date_text)<<","
		  <<csv_quote(result.at_data.lunar_date.lun_label)<<","
		  <<csv_quote(result.at_data.lunar_date.lun_mlab)<<","
		  <<(result.at_data.lunar_date.is_leap?"1":"0")<<","
		  <<result.at_data.lunar_date.lunar_day<<","
		  <<format_num(result.at_data.ill_pct)<<","
		  <<csv_quote(result.at_data.phase_name)<<","
		  <<csv_quote(result.at_data.moon_xg.region)<<","
		  <<csv_quote(result.at_data.moon_xg.star_name)<<","
		  <<format_num(result.at_data.moon_xg.sep_deg)<<","
		  <<csv_quote(result.at_data.local_iso)<<","
		  <<csv_quote(summary)<<","
		  <<csv_quote(astro_summary)<<","
		  <<csv_quote(result.at_data.hli.y_lun.text)<<","
		  <<csv_quote(result.at_data.hli.y_lchun.text)<<","
		  <<csv_quote(result.at_data.hli.y_rule.text)<<","
		  <<csv_quote(result.at_data.hli.m_gz.text)<<","
		  <<csv_quote(result.at_data.hli.d_gz.text)<<","
		  <<csv_quote(result.at_data.hli.h_gz.text)<<","
		  <<csv_quote(result.at_data.hli.h_gz_true.text)<<","
		  <<csv_quote(result.at_data.hli.rule_profile)<<","
		  <<result.at_data.hli.rule_profile_code<<","
		  <<gz_index_of(result.at_data.hli.y_lun)<<","
		  <<result.at_data.hli.y_lun.stem<<","
		  <<result.at_data.hli.y_lun.branch<<","
		  <<gz_index_of(result.at_data.hli.y_lchun)<<","
		  <<result.at_data.hli.y_lchun.stem<<","
		  <<result.at_data.hli.y_lchun.branch<<","
		  <<gz_index_of(result.at_data.hli.y_rule)<<","
		  <<result.at_data.hli.y_rule.stem<<","
		  <<result.at_data.hli.y_rule.branch<<","
		  <<gz_index_of(result.at_data.hli.m_gz)<<","
		  <<result.at_data.hli.m_gz.stem<<","
		  <<result.at_data.hli.m_gz.branch<<","
		  <<gz_index_of(result.at_data.hli.d_gz)<<","
		  <<result.at_data.hli.d_gz.stem<<","
		  <<result.at_data.hli.d_gz.branch<<","
		  <<gz_index_of(result.at_data.hli.h_gz)<<","
		  <<result.at_data.hli.h_gz.stem<<","
		  <<result.at_data.hli.h_gz.branch<<","
		  <<gz_index_of(result.at_data.hli.h_gz_true)<<","
		  <<result.at_data.hli.h_gz_true.stem<<","
		  <<result.at_data.hli.h_gz_true.branch<<","
		  <<csv_quote(result.at_data.hli.year_boundary_text)<<","
		  <<result.at_data.hli.year_boundary_code<<","
		  <<csv_quote(result.at_data.hli.month_boundary_text)<<","
		  <<result.at_data.hli.month_boundary_code<<","
		  <<csv_quote(result.at_data.hli.leap_month_mode_text)<<","
		  <<result.at_data.hli.leap_month_mode_code<<","
		  <<csv_quote(result.at_data.hli.day_boundary_text)<<","
		  <<result.at_data.hli.day_boundary_code<<","
		  <<csv_quote(result.at_data.hli.jianchu)<<","
		  <<result.at_data.hli.jianchu_code<<","
		  <<csv_quote(result.at_data.hli.bazi_clock)<<","
		  <<csv_quote(result.at_data.hli.bazi_true)<<","
		  <<csv_quote(result.at_data.hli.duty_god)<<","
		  <<result.at_data.hli.duty_god_code<<","
		  <<(result.at_data.hli.duty_is_yellow?"1":"0")<<","
		  <<csv_quote(result.at_data.hli.duty_tag)<<","
		  <<csv_quote(result.at_data.hli.clash)<<","
		  <<csv_quote(result.at_data.hli.chong_sha)<<","
		  <<csv_quote(result.at_data.hli.zodiac_day)<<","
		  <<csv_quote(result.at_data.hli.six_he)<<","
		  <<csv_quote(result.at_data.hli.three_he)<<","
		  <<csv_quote(result.at_data.hli.pengzu)<<","
		  <<csv_quote(result.at_data.hli.nayin)<<","
		  <<csv_quote(result.at_data.hli.wx_day)<<","
		  <<csv_quote(result.at_data.hli.fetal_god)<<","
		  <<csv_quote(result.at_data.hli.meridian)<<","
		  <<csv_quote(result.at_data.hli.lucky_dir)<<","
		  <<csv_quote(result.at_data.hli.wealth_dir)<<","
		  <<csv_quote(result.at_data.hli.mascot_dir)<<","
		  <<csv_quote(result.at_data.hli.sun_noble_dir)<<","
		  <<csv_quote(result.at_data.hli.moon_noble_dir)<<","
		  <<csv_quote(result.at_data.hli.xiu28)<<","
		  <<result.at_data.hli.xiu28_code<<","
		  <<csv_quote(result.at_data.hli.xiu28_mod28)<<","
		  <<result.at_data.hli.xiu28_mod28_code<<","
		  <<csv_quote(result.at_data.hli.xiu_id)<<","
		  <<result.at_data.hli.yi_ji_level<<","
		  <<csv_quote(result.at_data.hli.yi_ji_rule)<<","
		  <<result.at_data.hli.yi_ji_rule_code<<","
		  <<csv_quote(join_pipe(result.at_data.hli.good_gods))<<","
		  <<csv_quote(join_pipe(result.at_data.hli.bad_gods))<<","
		  <<csv_quote(join_pipe(result.at_data.hli.yi))<<","
		  <<csv_quote(join_pipe(result.at_data.hli.ji))<<","
		  <<csv_quote(hli_profile_key(static_cast<HliProfileCode>(
				 result.at_data.hli.rule_profile_code)))<<","
		  <<csv_quote(hli_year_boundary_key(static_cast<HliYearBoundary>(
				 result.at_data.hli.year_boundary_code)))<<","
		  <<csv_quote(hli_month_boundary_key(static_cast<HliMonthBoundary>(
				 result.at_data.hli.month_boundary_code)))<<","
		  <<csv_quote(hli_leap_month_mode_key(static_cast<HliLeapMonthMode>(
				 result.at_data.hli.leap_month_mode_code)))<<","
		  <<csv_quote(hli_day_boundary_key(static_cast<HliDayBoundary>(
				 result.at_data.hli.day_boundary_code)))<<","
		  <<result.at_data.hli.duty_tag_code<<","
		  <<result.at_data.hli.clash_branch_code<<","
		  <<result.at_data.hli.sha_dir_code<<","
		  <<result.at_data.hli.zodiac_day_code<<","
		  <<result.at_data.hli.six_he_branch_code<<","
		  <<result.at_data.hli.three_he_group_code<<","
		  <<result.at_data.hli.nayin_code<<","
		  <<result.at_data.hli.fetal_god_code<<","
		  <<result.at_data.hli.meridian_code<<","
		  <<result.at_data.hli.lucky_dir_code<<","
		  <<result.at_data.hli.wealth_dir_code<<","
		  <<result.at_data.hli.mascot_dir_code<<","
		  <<result.at_data.hli.sun_noble_dir_code<<","
		  <<result.at_data.hli.moon_noble_dir_code<<","
		  <<csv_quote(day_join_code_pipe(result.at_data.hli.good_god_codes))<<","
		  <<csv_quote(day_join_code_pipe(result.at_data.hli.bad_god_codes))<<","
		  <<csv_quote(day_join_code_pipe(result.at_data.hli.yi_codes))<<","
		  <<csv_quote(day_join_code_pipe(result.at_data.hli.ji_codes))<<","
		  <<csv_quote(day_hex_mask64(result.at_data.hli.good_god_mask))<<","
		  <<csv_quote(day_hex_mask64(result.at_data.hli.bad_god_mask))<<","
		  <<csv_quote(day_join_mask_hex_pipe(result.at_data.hli.yi_mask))<<","
		  <<csv_quote(day_join_mask_hex_pipe(result.at_data.hli.ji_mask))<<","
		  <<csv_quote(day_join_hour_pipe(result.at_data.hli.hour_jx,
								 [](const HliHour&x){ return x.slot; }))<<","
		  <<csv_quote(day_join_hour_pipe(result.at_data.hli.hour_jx,
								 [](const HliHour&x){
									 return std::to_string(x.slot_index);
								 }))<<","
		  <<csv_quote(day_join_hour_pipe(result.at_data.hli.hour_jx,
								 [](const HliHour&x){ return x.gz; }))<<","
		  <<csv_quote(day_join_hour_pipe(result.at_data.hli.hour_jx,
								 [](const HliHour&x){
									 return std::to_string(x.gz_index);
								 }))<<","
		  <<csv_quote(day_join_hour_pipe(result.at_data.hli.hour_jx,
								 [](const HliHour&x){ return x.luck; }))<<","
		  <<csv_quote(day_join_hour_pipe(result.at_data.hli.hour_jx,
								 [](const HliHour&x){
									 return x.is_bad?"1":"0";
								 }))<<"\n";
		return;
	}
	if(format=="txt"){
		os<<"tool=lunar format=txt type=day tz_display="<<result.tz<<"\n";
		os<<"input.date="<<result.date_text<<"\n";
		os<<"input.smp_time="<<result.at_time<<"\n";
		os<<"input.lon_deg="<<format_num(result.hli_lon_deg)<<"\n";
		os<<"input.astro="<<(result.inc_astro?"1":"0")<<"\n";
		os<<"input.astro_mode="<<result.astro_mode_text<<"\n";
		os<<"input.astro_pick="<<result.astro_pick_csv<<"\n";
		os<<"input.astro_site="<<(result.astro_obs.has_site?"1":"0")<<"\n";
		os<<"input.astro_lat_deg="
		  <<(result.astro_obs.has_site?format_num(result.astro_obs.lat_deg):"null")
		  <<"\n";
		os<<"input.astro_lon_deg="
		  <<(result.astro_obs.has_site?format_num(result.astro_obs.lon_deg):"null")
		  <<"\n";
		os<<"input.astro_height_m="
		  <<(result.astro_obs.has_site?format_num(result.astro_obs.h_m):"null")
		  <<"\n";
		os<<"data.lun_label="<<result.at_data.lunar_date.lun_label<<"\n";
		os<<"data.ill_pct="<<format_num(result.at_data.ill_pct)<<"\n";
		os<<"data.phase_name="<<result.at_data.phase_name<<"\n";
		os<<"data.moon_xg.region="<<result.at_data.moon_xg.region<<"\n";
		os<<"data.moon_xg.star_id="<<result.at_data.moon_xg.star_id<<"\n";
		os<<"data.moon_xg.star_name="<<result.at_data.moon_xg.star_name<<"\n";
		os<<"data.moon_xg.sep_deg="<<format_num(result.at_data.moon_xg.sep_deg)
		  <<"\n";
		os<<"data.hli.y_lun="<<result.at_data.hli.y_lun.text<<"\n";
		os<<"data.hli.y_lun_index="<<gz_index_of(result.at_data.hli.y_lun)<<"\n";
		os<<"data.hli.y_lun_stem="<<result.at_data.hli.y_lun.stem<<"\n";
		os<<"data.hli.y_lun_branch="<<result.at_data.hli.y_lun.branch<<"\n";
		os<<"data.hli.y_lchun="<<result.at_data.hli.y_lchun.text<<"\n";
		os<<"data.hli.y_lchun_index="<<gz_index_of(result.at_data.hli.y_lchun)
		  <<"\n";
		os<<"data.hli.y_lchun_stem="<<result.at_data.hli.y_lchun.stem<<"\n";
		os<<"data.hli.y_lchun_branch="<<result.at_data.hli.y_lchun.branch
		  <<"\n";
		os<<"data.hli.y_rule="<<result.at_data.hli.y_rule.text<<"\n";
		os<<"data.hli.y_rule_index="<<gz_index_of(result.at_data.hli.y_rule)
		  <<"\n";
		os<<"data.hli.y_rule_stem="<<result.at_data.hli.y_rule.stem<<"\n";
		os<<"data.hli.y_rule_branch="<<result.at_data.hli.y_rule.branch<<"\n";
		os<<"data.hli.month="<<result.at_data.hli.m_gz.text<<"\n";
		os<<"data.hli.month_index="<<gz_index_of(result.at_data.hli.m_gz)
		  <<"\n";
		os<<"data.hli.month_stem="<<result.at_data.hli.m_gz.stem<<"\n";
		os<<"data.hli.month_branch="<<result.at_data.hli.m_gz.branch<<"\n";
		os<<"data.hli.day="<<result.at_data.hli.d_gz.text<<"\n";
		os<<"data.hli.day_index="<<gz_index_of(result.at_data.hli.d_gz)<<"\n";
		os<<"data.hli.day_stem="<<result.at_data.hli.d_gz.stem<<"\n";
		os<<"data.hli.day_branch="<<result.at_data.hli.d_gz.branch<<"\n";
		os<<"data.hli.hour="<<result.at_data.hli.h_gz.text<<"\n";
		os<<"data.hli.hour_index="<<gz_index_of(result.at_data.hli.h_gz)
		  <<"\n";
		os<<"data.hli.hour_stem="<<result.at_data.hli.h_gz.stem<<"\n";
		os<<"data.hli.hour_branch="<<result.at_data.hli.h_gz.branch<<"\n";
		os<<"data.hli.hour_true="<<result.at_data.hli.h_gz_true.text<<"\n";
		os<<"data.hli.hour_true_index="
		  <<gz_index_of(result.at_data.hli.h_gz_true)<<"\n";
		os<<"data.hli.hour_true_stem="<<result.at_data.hli.h_gz_true.stem
		  <<"\n";
		os<<"data.hli.hour_true_branch="<<result.at_data.hli.h_gz_true.branch
		  <<"\n";
		os<<"data.hli.bazi_clock="<<result.at_data.hli.bazi_clock<<"\n";
		os<<"data.hli.bazi_true="<<result.at_data.hli.bazi_true<<"\n";
		os<<"data.hli.rule_profile="<<result.at_data.hli.rule_profile<<"\n";
		os<<"data.hli.rule_profile_code="<<result.at_data.hli.rule_profile_code<<"\n";
		os<<"data.hli.rule_profile_key="
		  <<hli_profile_key(
				 static_cast<HliProfileCode>(result.at_data.hli.rule_profile_code))
		  <<"\n";
		os<<"data.hli.year_boundary="<<result.at_data.hli.year_boundary_text<<"\n";
		os<<"data.hli.year_boundary_code="<<result.at_data.hli.year_boundary_code<<"\n";
		os<<"data.hli.year_boundary_key="
		  <<hli_year_boundary_key(static_cast<HliYearBoundary>(
				 result.at_data.hli.year_boundary_code))
		  <<"\n";
		os<<"data.hli.month_boundary="<<result.at_data.hli.month_boundary_text<<"\n";
		os<<"data.hli.month_boundary_code="<<result.at_data.hli.month_boundary_code<<"\n";
		os<<"data.hli.month_boundary_key="
		  <<hli_month_boundary_key(static_cast<HliMonthBoundary>(
				 result.at_data.hli.month_boundary_code))
		  <<"\n";
		os<<"data.hli.leap_month_mode="<<result.at_data.hli.leap_month_mode_text<<"\n";
		os<<"data.hli.leap_month_mode_code="<<result.at_data.hli.leap_month_mode_code<<"\n";
		os<<"data.hli.leap_month_mode_key="
		  <<hli_leap_month_mode_key(static_cast<HliLeapMonthMode>(
				 result.at_data.hli.leap_month_mode_code))
		  <<"\n";
		os<<"data.hli.day_boundary="<<result.at_data.hli.day_boundary_text<<"\n";
		os<<"data.hli.day_boundary_code="<<result.at_data.hli.day_boundary_code<<"\n";
		os<<"data.hli.day_boundary_key="
		  <<hli_day_boundary_key(
				 static_cast<HliDayBoundary>(result.at_data.hli.day_boundary_code))
		  <<"\n";
		os<<"data.hli.jianchu="<<result.at_data.hli.jianchu<<"\n";
		os<<"data.hli.jianchu_code="<<result.at_data.hli.jianchu_code<<"\n";
		os<<"data.hli.duty_god="<<result.at_data.hli.duty_god<<"\n";
		os<<"data.hli.duty_god_code="<<result.at_data.hli.duty_god_code<<"\n";
		os<<"data.hli.duty_is_yellow="
		  <<(result.at_data.hli.duty_is_yellow?"1":"0")<<"\n";
		os<<"data.hli.duty_tag="<<result.at_data.hli.duty_tag<<"\n";
		os<<"data.hli.duty_tag_code="<<result.at_data.hli.duty_tag_code<<"\n";
		os<<"data.hli.clash="<<result.at_data.hli.clash<<"\n";
		os<<"data.hli.clash_branch_code="
		  <<result.at_data.hli.clash_branch_code<<"\n";
		os<<"data.hli.chong_sha="<<result.at_data.hli.chong_sha<<"\n";
		os<<"data.hli.sha_dir_code="<<result.at_data.hli.sha_dir_code<<"\n";
		os<<"data.hli.zodiac_day="<<result.at_data.hli.zodiac_day<<"\n";
		os<<"data.hli.zodiac_day_code="<<result.at_data.hli.zodiac_day_code
		  <<"\n";
		os<<"data.hli.six_he="<<result.at_data.hli.six_he<<"\n";
		os<<"data.hli.six_he_branch_code="
		  <<result.at_data.hli.six_he_branch_code<<"\n";
		os<<"data.hli.three_he="<<result.at_data.hli.three_he<<"\n";
		os<<"data.hli.three_he_group_code="
		  <<result.at_data.hli.three_he_group_code<<"\n";
		os<<"data.hli.pengzu="<<result.at_data.hli.pengzu<<"\n";
		os<<"data.hli.nayin="<<result.at_data.hli.nayin<<"\n";
		os<<"data.hli.nayin_code="<<result.at_data.hli.nayin_code<<"\n";
		os<<"data.hli.wuxing_day="<<result.at_data.hli.wx_day<<"\n";
		os<<"data.hli.fetal_god="<<result.at_data.hli.fetal_god<<"\n";
		os<<"data.hli.fetal_god_code="<<result.at_data.hli.fetal_god_code
		  <<"\n";
		os<<"data.hli.meridian="<<result.at_data.hli.meridian<<"\n";
		os<<"data.hli.meridian_code="<<result.at_data.hli.meridian_code<<"\n";
		os<<"data.hli.lucky_dir="<<result.at_data.hli.lucky_dir<<"\n";
		os<<"data.hli.lucky_dir_code="<<result.at_data.hli.lucky_dir_code
		  <<"\n";
		os<<"data.hli.wealth_dir="<<result.at_data.hli.wealth_dir<<"\n";
		os<<"data.hli.wealth_dir_code="<<result.at_data.hli.wealth_dir_code
		  <<"\n";
		os<<"data.hli.mascot_dir="<<result.at_data.hli.mascot_dir<<"\n";
		os<<"data.hli.mascot_dir_code="<<result.at_data.hli.mascot_dir_code
		  <<"\n";
		os<<"data.hli.sun_noble_dir="<<result.at_data.hli.sun_noble_dir<<"\n";
		os<<"data.hli.sun_noble_dir_code="
		  <<result.at_data.hli.sun_noble_dir_code<<"\n";
		os<<"data.hli.moon_noble_dir="<<result.at_data.hli.moon_noble_dir<<"\n";
		os<<"data.hli.moon_noble_dir_code="
		  <<result.at_data.hli.moon_noble_dir_code<<"\n";
		os<<"data.hli.xiu28="<<result.at_data.hli.xiu28<<"\n";
		os<<"data.hli.xiu28_code="<<result.at_data.hli.xiu28_code<<"\n";
		os<<"data.hli.xiu28_mod28="<<result.at_data.hli.xiu28_mod28<<"\n";
		os<<"data.hli.xiu28_mod28_code="<<result.at_data.hli.xiu28_mod28_code<<"\n";
		os<<"data.hli.xiu_star="<<result.at_data.hli.xiu_id<<"\n";
		os<<"data.hli.yi_ji_level="<<result.at_data.hli.yi_ji_level<<"\n";
		os<<"data.hli.yi_ji_rule="<<result.at_data.hli.yi_ji_rule<<"\n";
		os<<"data.hli.yi_ji_rule_code="<<result.at_data.hli.yi_ji_rule_code<<"\n";
		os<<"data.hli.good_gods="<<join_pipe(result.at_data.hli.good_gods)<<"\n";
		os<<"data.hli.bad_gods="<<join_pipe(result.at_data.hli.bad_gods)<<"\n";
		os<<"data.hli.yi="<<join_pipe(result.at_data.hli.yi)<<"\n";
		os<<"data.hli.ji="<<join_pipe(result.at_data.hli.ji)<<"\n";
		os<<"data.hli.good_god_codes="
		  <<day_join_code_pipe(result.at_data.hli.good_god_codes)<<"\n";
		os<<"data.hli.bad_god_codes="
		  <<day_join_code_pipe(result.at_data.hli.bad_god_codes)<<"\n";
		os<<"data.hli.yi_codes="
		  <<day_join_code_pipe(result.at_data.hli.yi_codes)<<"\n";
		os<<"data.hli.ji_codes="
		  <<day_join_code_pipe(result.at_data.hli.ji_codes)<<"\n";
		os<<"data.hli.good_god_mask_hex="
		  <<day_hex_mask64(result.at_data.hli.good_god_mask)<<"\n";
		os<<"data.hli.bad_god_mask_hex="
		  <<day_hex_mask64(result.at_data.hli.bad_god_mask)<<"\n";
		os<<"data.hli.yi_mask_hex="
		  <<day_join_mask_hex_pipe(result.at_data.hli.yi_mask)<<"\n";
		os<<"data.hli.ji_mask_hex="
		  <<day_join_mask_hex_pipe(result.at_data.hli.ji_mask)<<"\n";
		os<<"data.smp_liso="<<result.at_data.local_iso<<"\n";
		os<<"[events]\n";
		os<<"kind\tcode\tname\tjd_utc\ttm_liso\n";
		for(const auto&ev : result.day_events){
			os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"
			  <<format_num(ev.jd_utc)<<"\t"<<ev.loc_iso<<"\n";
		}
		os<<"[astro_events]\n";
		os<<"kind\tcode\tname\tjd_utc\ttm_liso\n";
		for(const auto&ev : result.astro_events){
			os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"
			  <<format_num(ev.jd_utc)<<"\t"<<ev.loc_iso<<"\n";
		}
		os<<"[hour_jx]\n";
		os<<"slot\tslot_index\tgz\tgz_index\tluck\tis_bad\n";
		for(const auto&x : result.at_data.hli.hour_jx){
			os<<x.slot<<"\t"<<x.slot_index<<"\t"<<x.gz<<"\t"<<x.gz_index
			  <<"\t"<<x.luck<<"\t"<<(x.is_bad?"1":"0")<<"\n";
		}
		return;
	}
	throw std::invalid_argument("invalid --format for day: "+format);
}

}
