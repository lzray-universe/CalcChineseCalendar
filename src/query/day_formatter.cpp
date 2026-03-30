namespace lunar::core{

void format_day_output(std::ostream&os,const DayResult&result,
					   const std::string&format,bool pretty){
	EphRead eph(result.ephem);
	int tz_off=parse_tz(result.tz);

	auto write_json=[&](bool json_pretty){
		JsonWriter w(os,json_pretty);
		w.obj_begin();
		write_meta(w,result.ephem,result.tz,
				   {"type=day",lunar_day_rule_note(result.lunar_day_tz)});
		w.key("input");
		w.obj_begin();
		w.key("date");
		w.value(result.date_text);
		w.key("smp_time");
		w.value(result.at_time);
		w.key("lunar_day_tz");
		w.value(result.lunar_day_tz);
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
		  <<csv_quote(astro_summary)<<",";
		wr_hli_csv(os,result.at_data.hli,HliCsvLayout::Day);
		os<<"\n";
		return;
	}
	if(format=="txt"){
		os<<"tool=lunar format=txt type=day tz_display="<<result.tz<<"\n";
		os<<"input.date="<<result.date_text<<"\n";
		os<<"input.smp_time="<<result.at_time<<"\n";
		os<<"input.lunar_day_tz="<<result.lunar_day_tz<<"\n";
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
		wr_hli_txt(os,result.at_data.hli,HliTxtLayout::Day);
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
		wr_hli_hour_txt(os,result.at_data.hli);
		return;
	}
	throw std::invalid_argument("invalid --format for day: "+format);
}

}
