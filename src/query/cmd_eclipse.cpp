int cmd_eclipse(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_eclipse();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument("eclipse requires: <bsp> --near <YYYY-MM-DD>");
	}

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string kind_opt="lunar";
	std::string near_date;
	std::string stage_window="any";
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="json"&&format!="txt"&&format!="geojson"){
		format="json";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;

	double sample_minutes=2.0;
	bool point_refine=true;
	double point_lat=0.0;
	double point_lon=0.0;
	double point_height_m=0.0;
	bool has_point_lat=false;
	bool has_point_lon=false;
	bool has_point_height=false;
	bool point_refine_set=false;

	bool global_vis=false;
	std::string global_format="json";
	double grid_lat_step=10.0;
	double grid_lon_step=10.0;
	bool global_format_set=false;
	bool grid_lat_step_set=false;
	bool grid_lon_step_set=false;

	const OptMap handlers={
		{"--kind",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 kind_opt=to_low(req_val(src,idx,opt));
		 }},
		{"--near",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 near_date=req_val(src,idx,opt);
		 }},
		{"--stage",[&](const std::vector<std::string>&src,std::size_t&idx,
					   const std::string&opt){
			 stage_window=to_low(req_val(src,idx,opt));
		 }},
		{"--sample-min",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 sample_minutes=parse_double(req_val(src,idx,opt),opt);
		 }},
		{"--point-refine",[&](const std::vector<std::string>&src,std::size_t&idx,
							  const std::string&opt){
			 point_refine=parse_bool01(req_val(src,idx,opt),opt);
			 point_refine_set=true;
		 }},
		{"--point-lat",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 point_lat=parse_double(req_val(src,idx,opt),opt);
			 has_point_lat=true;
		 }},
		{"--point-lon",[&](const std::vector<std::string>&src,std::size_t&idx,
						   const std::string&opt){
			 point_lon=parse_double(req_val(src,idx,opt),opt);
			 has_point_lon=true;
		 }},
		{"--point-height",[&](const std::vector<std::string>&src,std::size_t&idx,
							  const std::string&opt){
			 point_height_m=parse_double(req_val(src,idx,opt),opt);
			 has_point_height=true;
		 }},
		{"--global-vis",[&](const std::vector<std::string>&src,std::size_t&idx,
							const std::string&opt){
			 global_vis=parse_bool01(req_val(src,idx,opt),opt);
		 }},
		{"--global",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 global_vis=parse_bool01(req_val(src,idx,opt),opt);
		 }},
		{"--global-format",[&](const std::vector<std::string>&src,
							   std::size_t&idx,const std::string&opt){
			 global_format=to_low(req_val(src,idx,opt));
			 global_format_set=true;
		 }},
		{"--grid-lat-step",[&](const std::vector<std::string>&src,
							   std::size_t&idx,const std::string&opt){
			 grid_lat_step=parse_double(req_val(src,idx,opt),opt);
			 grid_lat_step_set=true;
		 }},
		{"--grid-lon-step",[&](const std::vector<std::string>&src,
							   std::size_t&idx,const std::string&opt){
			 grid_lon_step=parse_double(req_val(src,idx,opt),opt);
			 grid_lon_step_set=true;
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

	for(std::size_t i=1;i<args.size();++i){
		const std::string&opt=args[i];
		apply_opt(handlers,args,i,opt,"eclipse");
	}

	if(near_date.empty()){
		throw std::invalid_argument("eclipse requires --near <YYYY-MM-DD>");
	}
	if(kind_opt!="lunar"&&kind_opt!="solar"){
		throw std::invalid_argument("--kind must be lunar or solar");
	}
	if(!(sample_minutes>0.0)){
		throw std::invalid_argument("--sample-min must be > 0");
	}
	if(has_point_lat!=has_point_lon){
		throw std::invalid_argument(
			"point visibility requires both --point-lat and --point-lon");
	}
	if((has_point_height||point_refine_set)&&!has_point_lat){
		throw std::invalid_argument(
			"--point-height/--point-refine require --point-lat and --point-lon");
	}
	if(global_format!="json"&&global_format!="geojson"){
		throw std::invalid_argument("--global-format must be json or geojson");
	}
	chk_fmt(format,{"json","txt","geojson"},"eclipse");
	if(format=="geojson"){
		global_vis=true;
	}
	if((global_format_set||grid_lat_step_set||grid_lon_step_set)&&!global_vis){
		throw std::invalid_argument(
			"--global-format/--grid-lat-step/--grid-lon-step require --global-vis 1 or --format geojson");
	}
	if(grid_lat_step_set&&!(grid_lat_step>0.0)){
		throw std::invalid_argument("--grid-lat-step must be > 0");
	}
	if(grid_lon_step_set&&!(grid_lon_step>0.0)){
		throw std::invalid_argument("--grid-lon-step must be > 0");
	}

	int y=0;
	int m=0;
	int d=0;
	std::tie(y,m,d)=parse_ymd(near_date);
	double near_jd_utc=greg2jd(y,m,d,0,0,0.0)-UTC8DAY;
	int tz_off=parse_tz(tz);

	EphRead eph(ephem);
	std::string ecl_kind=(kind_opt=="solar")?"solar_eclipse":"lunar_eclipse";
	EventRec ev=nearest_ecl(eph,near_jd_utc,tz_off,quiet,ecl_kind);
	double jd_tdb=
		std::isfinite(ev.jd_tdb)?ev.jd_tdb:TimeScale::utc_to_tdb(ev.jd_utc);
	OutTgt out=open_out(out_path);

	if(kind_opt=="lunar"){
		LunarEclipse ecl;
		if(!calc_lunar_eclipse(eph,jd_tdb,&ecl)||!ecl.has){
			throw std::runtime_error("failed to compute lunar eclipse details");
		}

		bool has_point_vis=false;
		LunarEclipsePointVis point_vis;
		if(has_point_lat){
			if(!lunar_eclipse_point_visibility(eph,ecl,stage_window,point_lat,
											   point_lon,point_height_m,
											   sample_minutes,point_refine,
											   &point_vis)){
				throw std::invalid_argument(
					"requested stage window is unavailable for this eclipse");
			}
			has_point_vis=true;
		}

		bool has_global_vis=false;
		LunarEclipseGlobalVis global_data;
		if(global_vis){
			if(!lunar_eclipse_global_visibility(eph,ecl,stage_window,grid_lat_step,
												grid_lon_step,sample_minutes,
												&global_data)){
				throw std::invalid_argument(
					"requested stage window is unavailable for this eclipse");
			}
			has_global_vis=true;
		}

		const FmtMap fmt_handlers={
			{"json",[&](){
				 JsonWriter w(*out.stream,pretty);
				 w.obj_begin();
				 write_meta(w,ephem,tz,{"type=eclipse"});
				 w.key("input");
				 w.obj_begin();
				 w.key("kind");
				 w.value(kind_opt);
				 w.key("near");
				 w.value(near_date);
				 w.key("stage_window");
				 w.value(stage_window);
				 w.key("sample_minutes");
				 w.value(sample_minutes);
				 w.key("global_vis");
				 w.value(global_vis);
				 w.key("global_format");
				 w.value(global_format);
				 if(has_point_lat){
					 w.key("point");
					 w.obj_begin();
					 w.key("lat_deg");
					 w.value(point_lat);
					 w.key("lon_deg");
					 w.value(point_lon);
					 w.key("height_m");
					 w.value(point_height_m);
					 w.key("refine");
					 w.value(point_refine);
					 w.obj_end();
				 }else{
					 w.key("point");
					 w.null_val();
				 }
				 w.obj_end();
				 w.key("data");
				 w.obj_begin();
				 w.key("event");
				 wr_ejson(w,ev,eph,false,tz_off);
				 w.key("lunar_eclipse");
				 wr_ecljson(w,ecl,ev.year,tz_off);
				 w.key("point_visibility");
				 if(has_point_vis){
					 wr_ptvis_json(w,point_vis,tz_off);
				 }else{
					 w.null_val();
				 }
				 w.key("global_visibility");
				 if(has_global_vis){
					 if(global_format=="geojson"){
						 wr_glbvis_geojson(w,global_data,tz_off);
					 }else{
						 wr_glbvis_json(w,global_data,tz_off);
					 }
				 }else{
					 w.null_val();
				 }
				 w.obj_end();
				 w.obj_end();
				 *out.stream<<"\n";
			 }},
			{"txt",[&](){
				 std::ostream&os=*out.stream;
				 os<<"tool=lunar format=txt type=eclipse tz_display="<<tz<<"\n";
				 os<<"input.kind="<<kind_opt<<"\n";
				 os<<"input.near="<<near_date<<"\n";
				 os<<"input.stage_window="<<stage_window<<"\n";
				 os<<"input.sample_minutes="<<format_num(sample_minutes)<<"\n";
				 os<<"data.event.kind="<<ev.kind<<"\n";
				 os<<"data.event.code="<<ev.code<<"\n";
				 os<<"data.event.name="<<ev.name<<"\n";
				 os<<"data.event.loc_iso="<<ev.loc_iso<<"\n";
				 os<<"[lunar_eclipse]\n";
				 os<<"type="<<ecl.type<<"\n";
				 auto out_num=[&](double v){
					 if(std::isfinite(v)){
						 os<<format_num(v);
					 }else{
						 os<<"null";
					 }
				 };
				 os<<"pen_mag=";
				 out_num(ecl.pen_mag);
				 os<<"\n";
				 os<<"umb_mag=";
				 out_num(ecl.umb_mag);
				 os<<"\n";
				 os<<"rp_re=";
				 out_num(ecl.rp_re);
				 os<<"\n";
				 os<<"ru_re=";
				 out_num(ecl.ru_re);
				 os<<"\n";
				 os<<"opp_rp_re=";
				 out_num(ecl.opp_rp_re);
				 os<<"\n";
				 os<<"opp_ru_re=";
				 out_num(ecl.opp_ru_re);
				 os<<"\n";
				 os<<"dur_pen_sec=";
				 out_num(ecl.dur_pen_sec);
				 os<<"\n";
				 os<<"dur_umb_sec=";
				 out_num(ecl.dur_umb_sec);
				 os<<"\n";
				 os<<"dur_tot_sec=";
				 out_num(ecl.dur_tot_sec);
				 os<<"\n";
				 os<<"dt_max_sec=";
				 out_num(ecl.dt_max_sec);
				 os<<"\n";
				 os<<"moon_dist_km=";
				 out_num(ecl.moon_dist_km);
				 os<<"\n";
				 os<<"gamma=";
				 out_num(ecl.gamma);
				 os<<"\n";
				 os<<"eps_deg=";
				 out_num(ecl.eps_deg);
				 os<<"\n";
				 os<<"sun_ra_deg=";
				 out_num(ecl.sun_geo.ra_deg);
				 os<<"\n";
				 os<<"sun_dec_deg=";
				 out_num(ecl.sun_geo.dec_deg);
				 os<<"\n";
				 os<<"sun_sd_deg=";
				 out_num(ecl.sun_geo.sd_deg);
				 os<<"\n";
				 os<<"sun_ehp_deg=";
				 out_num(ecl.sun_geo.ehp_deg);
				 os<<"\n";
				 os<<"moon_ra_deg=";
				 out_num(ecl.moon_geo.ra_deg);
				 os<<"\n";
				 os<<"moon_dec_deg=";
				 out_num(ecl.moon_geo.dec_deg);
				 os<<"\n";
				 os<<"moon_sd_deg=";
				 out_num(ecl.moon_geo.sd_deg);
				 os<<"\n";
				 os<<"moon_ehp_deg=";
				 out_num(ecl.moon_geo.ehp_deg);
				 os<<"\n";
				 os<<"lib_l_deg=";
				 out_num(ecl.lib.l_deg);
				 os<<"\n";
				 os<<"lib_b_deg=";
				 out_num(ecl.lib.b_deg);
				 os<<"\n";
				 os<<"lib_c_deg=";
				 out_num(ecl.lib.c_deg);
				 os<<"\n";
				 os<<"p1_loc="<<node_liso(ecl.jd_tdb_p1,tz_off)<<"\n";
				 os<<"u1_loc="<<node_liso(ecl.jd_tdb_u1,tz_off)<<"\n";
				 os<<"opp_loc="<<node_liso(ecl.jd_tdb_opp,tz_off)<<"\n";
				 os<<"max_loc="<<node_liso(ecl.jd_tdb_max,tz_off)<<"\n";
				 os<<"u4_loc="<<node_liso(ecl.jd_tdb_u4,tz_off)<<"\n";
				 os<<"p4_loc="<<node_liso(ecl.jd_tdb_p4,tz_off)<<"\n";
				 os<<"u2_loc="<<node_liso(ecl.jd_tdb_u2,tz_off)<<"\n";
				 os<<"u3_loc="<<node_liso(ecl.jd_tdb_u3,tz_off)<<"\n";
				 wr_node_kv(os,"p1",ecl.jd_tdb_p1,ecl.p1_meta);
				 wr_node_kv(os,"u1",ecl.jd_tdb_u1,ecl.u1_meta);
				 wr_node_kv(os,"u2",ecl.jd_tdb_u2,ecl.u2_meta);
				 wr_node_kv(os,"max",ecl.jd_tdb_max,ecl.max_meta);
				 wr_node_kv(os,"u3",ecl.jd_tdb_u3,ecl.u3_meta);
				 wr_node_kv(os,"u4",ecl.jd_tdb_u4,ecl.u4_meta);
				 wr_node_kv(os,"p4",ecl.jd_tdb_p4,ecl.p4_meta);
				 if(has_point_vis){
					 os<<"[point_visibility]\n";
					 os<<"visible="<<(point_vis.visible?"1":"0")<<"\n";
					 os<<"max_alt_deg="<<format_num(point_vis.max_alt_deg)<<"\n";
					 os<<"first_visible=";
					 if(std::isfinite(point_vis.first_jd_utc)){
						 os<<fmt_iso(point_vis.first_jd_utc,tz_off,true)<<"\n";
					 }else{
						 os<<"null\n";
					 }
					 os<<"last_visible=";
					 if(std::isfinite(point_vis.last_jd_utc)){
						 os<<fmt_iso(point_vis.last_jd_utc,tz_off,true)<<"\n";
					 }else{
						 os<<"null\n";
					 }
					 os<<"sample_count="<<point_vis.sample_count<<"\n";
				 }
				 if(has_global_vis){
					 os<<"[global_visibility]\n";
					 os<<"points="<<global_data.points.size()<<"\n";
					 os<<"lat\tlon\tmax_alt_deg\tfirst_visible\tlast_visible\n";
					 for(const auto&pt : global_data.points){
						 os<<format_num(pt.lat_deg)<<"\t"<<format_num(pt.lon_deg)
						   <<"\t"<<format_num(pt.max_alt_deg)<<"\t"
						   <<fmt_iso(pt.first_jd_utc,tz_off,true)<<"\t"
						   <<fmt_iso(pt.last_jd_utc,tz_off,true)<<"\n";
					 }
				 }
			 }},
			{"geojson",[&](){
				 if(!has_global_vis){
					 throw std::runtime_error("geojson output requires global visibility");
				 }
				 JsonWriter w(*out.stream,pretty);
				 wr_glbvis_geojson(w,global_data,tz_off);
				 *out.stream<<"\n";
			 }},
		};
		run_fmt(fmt_handlers,format,"eclipse");
		note_out(out_path,quiet);
		return 0;
	}

	SolarEclipse ecl;
	if(!calc_solar_eclipse(eph,jd_tdb,&ecl)||!ecl.has){
		throw std::runtime_error("failed to compute solar eclipse details");
	}

	bool has_point_vis=false;
	SolarEclipsePointVis point_vis;
	if(has_point_lat){
		if(!solar_eclipse_point_visibility(eph,ecl,stage_window,point_lat,point_lon,
										   point_height_m,sample_minutes,
										   point_refine,&point_vis)){
			throw std::invalid_argument(
				"requested stage window is unavailable for this eclipse");
		}
		has_point_vis=true;
	}

	bool has_global_vis=false;
	SolarEclipseGlobalVis global_data;
	if(global_vis){
		if(!solar_eclipse_global_visibility(eph,ecl,stage_window,grid_lat_step,
											grid_lon_step,sample_minutes,
											&global_data)){
			throw std::invalid_argument(
				"requested stage window is unavailable for this eclipse");
		}
		has_global_vis=true;
	}

	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,pretty);
			 w.obj_begin();
			 write_meta(w,ephem,tz,{"type=eclipse"});
			 w.key("input");
			 w.obj_begin();
			 w.key("kind");
			 w.value(kind_opt);
			 w.key("near");
			 w.value(near_date);
			 w.key("stage_window");
			 w.value(stage_window);
			 w.key("sample_minutes");
			 w.value(sample_minutes);
			 w.key("global_vis");
			 w.value(global_vis);
			 w.key("global_format");
			 w.value(global_format);
			 if(has_point_lat){
				 w.key("point");
				 w.obj_begin();
				 w.key("lat_deg");
				 w.value(point_lat);
				 w.key("lon_deg");
				 w.value(point_lon);
				 w.key("height_m");
				 w.value(point_height_m);
				 w.key("refine");
				 w.value(point_refine);
				 w.obj_end();
			 }else{
				 w.key("point");
				 w.null_val();
			 }
			 w.obj_end();
			 w.key("data");
			 w.obj_begin();
			 w.key("event");
			 wr_ejson(w,ev,eph,false,tz_off);
			 w.key("solar_eclipse");
			 wr_sol_ecljson(w,ecl,ev.year,tz_off);
			 w.key("point_visibility");
			 if(has_point_vis){
				 wr_sol_ptvis_json(w,point_vis,tz_off);
			 }else{
				 w.null_val();
			 }
			 w.key("global_visibility");
			 if(has_global_vis){
				 if(global_format=="geojson"){
					 wr_sol_glbvis_geojson(w,global_data,tz_off);
				 }else{
					 wr_sol_glbvis_json(w,global_data,tz_off);
				 }
			 }else{
				 w.null_val();
			 }
			 w.obj_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 auto out_num=[&](double v){
				 if(std::isfinite(v)){
					 os<<format_num(v);
				 }else{
					 os<<"null";
				 }
			 };
			 os<<"tool=lunar format=txt type=eclipse tz_display="<<tz<<"\n";
			 os<<"input.kind="<<kind_opt<<"\n";
			 os<<"input.near="<<near_date<<"\n";
			 os<<"input.stage_window="<<stage_window<<"\n";
			 os<<"input.sample_minutes="<<format_num(sample_minutes)<<"\n";
			 os<<"data.event.kind="<<ev.kind<<"\n";
			 os<<"data.event.code="<<ev.code<<"\n";
			 os<<"data.event.name="<<ev.name<<"\n";
			 os<<"data.event.loc_iso="<<ev.loc_iso<<"\n";
			 os<<"[solar_eclipse]\n";
			 os<<"type="<<ecl.type<<"\n";
			 os<<"mag=";
			 out_num(ecl.mag);
			 os<<"\n";
			 os<<"obscuration=";
			 out_num(ecl.obscuration);
			 os<<"\n";
			 os<<"gamma=";
			 out_num(ecl.gamma);
			 os<<"\n";
			 os<<"sep_max_deg=";
			 out_num(ecl.sep_max_deg);
			 os<<"\n";
			 os<<"sun_sd_max_deg=";
			 out_num(ecl.sun_sd_max_deg);
			 os<<"\n";
			 os<<"moon_sd_max_deg=";
			 out_num(ecl.moon_sd_max_deg);
			 os<<"\n";
			 os<<"sun_dist_km=";
			 out_num(ecl.sun_dist_km);
			 os<<"\n";
			 os<<"moon_dist_km=";
			 out_num(ecl.moon_dist_km);
			 os<<"\n";
			 os<<"dt_max_sec=";
			 out_num(ecl.dt_max_sec);
			 os<<"\n";
			 os<<"rp_re=";
			 out_num(ecl.rp_re);
			 os<<"\n";
			 os<<"ru_re=";
			 out_num(ecl.ru_re);
			 os<<"\n";
			 os<<"c1_loc="<<node_liso(ecl.jd_tdb_c1,tz_off)<<"\n";
			 os<<"c2_loc="<<node_liso(ecl.jd_tdb_c2,tz_off)<<"\n";
			 os<<"max_loc="<<node_liso(ecl.jd_tdb_max,tz_off)<<"\n";
			 os<<"c3_loc="<<node_liso(ecl.jd_tdb_c3,tz_off)<<"\n";
			 os<<"c4_loc="<<node_liso(ecl.jd_tdb_c4,tz_off)<<"\n";
			 if(has_point_vis){
				 os<<"[point_visibility]\n";
				 os<<"has_eclipse="<<(point_vis.has_eclipse?"1":"0")<<"\n";
				 os<<"visible="<<(point_vis.visible?"1":"0")<<"\n";
				 os<<"central="<<(point_vis.central?"1":"0")<<"\n";
				 os<<"max_mag="<<format_num(point_vis.max_mag)<<"\n";
				 os<<"max_obscuration="<<format_num(point_vis.max_obscuration)<<"\n";
				 os<<"max_sun_alt_deg="<<format_num(point_vis.max_sun_alt_deg)<<"\n";
				 os<<"c1_loc=";
				 if(std::isfinite(point_vis.c1_jd_utc)){
					 os<<fmt_iso(point_vis.c1_jd_utc,tz_off,true)<<"\n";
				 }else{
					 os<<"null\n";
				 }
				 os<<"c2_loc=";
				 if(std::isfinite(point_vis.c2_jd_utc)){
					 os<<fmt_iso(point_vis.c2_jd_utc,tz_off,true)<<"\n";
				 }else{
					 os<<"null\n";
				 }
				 os<<"max_loc=";
				 if(std::isfinite(point_vis.max_jd_utc)){
					 os<<fmt_iso(point_vis.max_jd_utc,tz_off,true)<<"\n";
				 }else{
					 os<<"null\n";
				 }
				 os<<"c3_loc=";
				 if(std::isfinite(point_vis.c3_jd_utc)){
					 os<<fmt_iso(point_vis.c3_jd_utc,tz_off,true)<<"\n";
				 }else{
					 os<<"null\n";
				 }
				 os<<"c4_loc=";
				 if(std::isfinite(point_vis.c4_jd_utc)){
					 os<<fmt_iso(point_vis.c4_jd_utc,tz_off,true)<<"\n";
				 }else{
					 os<<"null\n";
				 }
				 os<<"first_visible=";
				 if(std::isfinite(point_vis.first_jd_utc)){
					 os<<fmt_iso(point_vis.first_jd_utc,tz_off,true)<<"\n";
				 }else{
					 os<<"null\n";
				 }
				 os<<"last_visible=";
				 if(std::isfinite(point_vis.last_jd_utc)){
					 os<<fmt_iso(point_vis.last_jd_utc,tz_off,true)<<"\n";
				 }else{
					 os<<"null\n";
				 }
				 os<<"sample_count="<<point_vis.sample_count<<"\n";
			 }
			 if(has_global_vis){
				 os<<"[global_visibility]\n";
				 os<<"points="<<global_data.points.size()<<"\n";
				 os<<"lat\tlon\tmax_mag\tmax_sun_alt_deg\tfirst_visible\tlast_visible\n";
				 for(const auto&pt : global_data.points){
					 os<<format_num(pt.lat_deg)<<"\t"<<format_num(pt.lon_deg)<<"\t"
					   <<format_num(pt.max_mag)<<"\t"<<format_num(pt.max_sun_alt_deg)
					   <<"\t"<<fmt_iso(pt.first_jd_utc,tz_off,true)<<"\t"
					   <<fmt_iso(pt.last_jd_utc,tz_off,true)<<"\n";
				 }
			 }
		 }},
		{"geojson",[&](){
			 if(!has_global_vis){
				 throw std::runtime_error("geojson output requires global visibility");
			 }
			 JsonWriter w(*out.stream,pretty);
			 wr_sol_glbvis_geojson(w,global_data,tz_off);
			 *out.stream<<"\n";
		 }},
	};
	run_fmt(fmt_handlers,format,"eclipse");
	note_out(out_path,quiet);
	return 0;
}

