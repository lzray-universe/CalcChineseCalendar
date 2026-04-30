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
		CsvWriter csv(os);
		csv.write_field("date",result.date_text);
		csv.write_field("lun_label",result.at_data.lunar_date.lun_label);
		csv.write_field("lun_mlab",result.at_data.lunar_date.lun_mlab);
		csv.write_field("is_leap",result.at_data.lunar_date.is_leap);
		csv.write_field("lunar_day",result.at_data.lunar_date.lunar_day);
		csv.write_raw("ill_pct",format_num(result.at_data.ill_pct));
		csv.write_field("phase_name",result.at_data.phase_name);
		csv.write_field("moon_xg_region",result.at_data.moon_xg.region);
		csv.write_field("moon_xg_star",result.at_data.moon_xg.star_name);
		csv.write_raw("moon_xg_sep_deg",format_num(result.at_data.moon_xg.sep_deg));
		csv.write_field("smp_uiso",result.at_data.utc_iso);
		csv.write_field("smp_liso",result.at_data.local_iso);
		csv.write_field("ev_sum",summary);
		csv.write_field("astro_ev_sum",astro_summary);
		wr_hli_csv(csv,result.at_data.hli,HliCsvLayout::Day);
		csv.finish_row();
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
		os<<"data.smp_uiso="<<result.at_data.utc_iso<<"\n";
		os<<"data.smp_liso="<<result.at_data.local_iso<<"\n";
		os<<"[events]\n";
		os<<"kind\tcode\tname\tyear\tjd_tdb\tjd_utc\tutc_iso\tloc_iso\n";
		for(const auto&ev : result.day_events){
			os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t"
			  <<node_num(event_jd_tdb(ev))<<"\t"<<format_num(ev.jd_utc)<<"\t"
			  <<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
		}
		os<<"[astro_events]\n";
		os<<"kind\tcode\tname\tyear\tjd_tdb\tjd_utc\tutc_iso\tloc_iso\n";
		for(const auto&ev : result.astro_events){
			os<<ev.kind<<"\t"<<ev.code<<"\t"<<ev.name<<"\t"<<ev.year<<"\t"
			  <<node_num(event_jd_tdb(ev))<<"\t"<<format_num(ev.jd_utc)<<"\t"
			  <<ev.utc_iso<<"\t"<<ev.loc_iso<<"\n";
		}
		wr_hli_hour_txt(os,result.at_data.hli);
		return;
	}
	throw std::invalid_argument("invalid --format for day: "+format);
}

}
