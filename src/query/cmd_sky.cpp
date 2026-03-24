namespace{

std::string csv_quote(const std::string&s);

std::string csv_num_or_empty(double value){
	return std::isfinite(value)?format_num(value):"";
}

}

int cmd_sky(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_sky();
		return 0;
	}
	if(args.empty()){
		throw std::invalid_argument(
			"sky requires: <bsp> <time> --lat <deg> --lon <deg>");
	}

	InterCfg cfg=load_def();
	std::string ephem=args[0];
	std::string time_raw;
	std::string input_tz=cfg.default_tz;
	std::string tz=cfg.default_tz;
	std::string format=to_low(cfg.def_fmt);
	if(format!="txt"&&format!="json"&&format!="csv"){
		format="txt";
	}
	std::string out_path;
	bool pretty=cfg.def_prety;
	bool quiet=false;
	std::string mode_text="all";
	std::string pick_csv;
	double lat_deg=0.0;
	double lon_deg=0.0;
	double height_m=0.0;
	bool has_lat=false;
	bool has_lon=false;

	std::size_t i=1;
	if(i<args.size()&&!is_opt(args[i])){
		time_raw=args[i];
		++i;
	}
	const OptMap handlers={
		{"--time",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 time_raw=req_val(src,idx,opt);
		 }},
		{"--input-tz",[&](const std::vector<std::string>&src,std::size_t&idx,
						  const std::string&opt){
			 input_tz=req_val(src,idx,opt);
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
		{"--mode",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 mode_text=to_low(req_val(src,idx,opt));
		 }},
		{"--pick",[&](const std::vector<std::string>&src,std::size_t&idx,
					  const std::string&opt){
			 pick_csv=req_val(src,idx,opt);
		 }},
		{"--lat",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){
			 lat_deg=parse_double(req_val(src,idx,opt),"--lat");
			 has_lat=true;
		 }},
		{"--lon",[&](const std::vector<std::string>&src,std::size_t&idx,
					 const std::string&opt){
			 lon_deg=parse_double(req_val(src,idx,opt),"--lon");
			 has_lon=true;
		 }},
		{"--height",[&](const std::vector<std::string>&src,std::size_t&idx,
						const std::string&opt){
			 height_m=parse_double(req_val(src,idx,opt),"--height");
		 }},
	};

	for(;i<args.size();++i){
		const std::string&opt=args[i];
		if(opt=="-h"||opt=="--help"){
			use_sky();
			return 0;
		}
		apply_opt(handlers,args,i,opt,"sky");
	}

	if(time_raw.empty()){
		throw std::invalid_argument("sky requires a <time> or --time <time>");
	}
	if(has_lat!=has_lon){
		throw std::invalid_argument("sky requires both --lat and --lon");
	}
	if(!has_lat){
		throw std::invalid_argument("sky requires --lat and --lon");
	}
	chk_fmt(format,{"json","txt","csv"},"sky");

	const SkyMode mode=parse_sky_mode(mode_text);
	const SkyPick pick=make_sky_pick(mode,pick_csv);
	const IsoTime parsed=parse_iso(time_raw,input_tz);
	const int tz_disp=parse_tz(tz);
	const std::string tz_in=
		parsed.has_tz?fmt_tz(parsed.tz_off):fmt_tz(parse_tz(input_tz));

	AstroObs obs;
	obs.has_site=true;
	obs.lat_deg=lat_deg;
	obs.lon_deg=lon_deg;
	obs.h_m=height_m;

	EphRead eph(ephem);
	std::vector<SkyPos> sky=calc_sky_pos(eph,parsed.jd_utc,obs,pick);

	OutTgt out=open_out(out_path);
	const FmtMap fmt_handlers={
		{"json",[&](){
			 JsonWriter w(*out.stream,pretty);
			 w.obj_begin();
			 write_meta(w,ephem,tz,{"type=sky","topocentric=true"});
			 w.key("input");
			 w.obj_begin();
			 w.key("time_raw");
			 w.value(time_raw);
			 w.key("input_tz");
			 w.value(tz_in);
			 w.key("display_tz");
			 w.value(tz);
			 w.key("jd_utc");
			 w.value(parsed.jd_utc);
			 w.key("jd_tdb");
			 w.value(TimeScale::utc_to_tdb(parsed.jd_utc));
			 w.key("utc_iso");
			 w.value(fmt_iso(parsed.jd_utc,0,true));
			 w.key("loc_iso");
			 w.value(fmt_iso(parsed.jd_utc,tz_disp,true));
			 w.key("lat_deg");
			 w.value(lat_deg);
			 w.key("lon_deg");
			 w.value(lon_deg);
			 w.key("height_m");
			 w.value(height_m);
			 w.key("mode");
			 w.value(mode_text);
			 w.key("pick");
			 if(mode==SkyMode::Pick){
				 w.value(pick_csv);
			 }else{
				 w.null_val();
			 }
			 w.obj_end();
			 w.key("data");
			 w.arr_begin();
			 for(const auto&row : sky){
				 w.obj_begin();
				 w.key("kind");
				 w.value(row.kind);
				 w.key("code");
				 w.value(row.code);
				 w.key("name");
				 w.value(row.name);
				 w.key("region");
				 w.value(row.region);
				 w.key("is_solar_system");
				 w.value(row.is_solar_system);
				 w.key("is_juxing");
				 w.value(row.is_juxing);
				 w.key("mag_v");
				 wr_num_or_null(w,row.mag_v);
				 w.key("ra_deg");
				 w.value(row.ra_deg);
				 w.key("dec_deg");
				 w.value(row.dec_deg);
				 w.key("az_deg");
				 w.value(row.az_deg);
				 w.key("alt_deg");
				 w.value(row.alt_deg);
				 w.obj_end();
			 }
			 w.arr_end();
			 w.obj_end();
			 *out.stream<<"\n";
		 }},
		{"csv",[&](){
			 *out.stream<<"kind,code,name,region,is_solar_system,is_juxing,"
						  "mag_v,ra_deg,dec_deg,az_deg,alt_deg\n";
			 for(const auto&row : sky){
				 *out.stream<<csv_quote(row.kind)<<","
						   <<csv_quote(row.code)<<","
						   <<csv_quote(row.name)<<","
						   <<csv_quote(row.region)<<","
						   <<(row.is_solar_system?"1":"0")<<","
						   <<(row.is_juxing?"1":"0")<<","
						   <<csv_num_or_empty(row.mag_v)<<","
						   <<format_num(row.ra_deg)<<","
						   <<format_num(row.dec_deg)<<","
						   <<format_num(row.az_deg)<<","
						   <<format_num(row.alt_deg)<<"\n";
			 }
		 }},
		{"txt",[&](){
			 std::ostream&os=*out.stream;
			 os<<"tool=lunar format=txt type=sky tz_display="<<tz<<"\n";
			 os<<"input.time_raw="<<time_raw<<"\n";
			 os<<"input.input_tz="<<tz_in<<"\n";
			 os<<"input.display_tz="<<tz<<"\n";
			 os<<"input.jd_utc="<<format_num(parsed.jd_utc)<<"\n";
			 os<<"input.jd_tdb="
			   <<format_num(TimeScale::utc_to_tdb(parsed.jd_utc))<<"\n";
			 os<<"input.utc_iso="<<fmt_iso(parsed.jd_utc,0,true)<<"\n";
			 os<<"input.loc_iso="<<fmt_iso(parsed.jd_utc,tz_disp,true)<<"\n";
			 os<<"input.lat_deg="<<format_num(lat_deg)<<"\n";
			 os<<"input.lon_deg="<<format_num(lon_deg)<<"\n";
			 os<<"input.height_m="<<format_num(height_m)<<"\n";
			 os<<"input.mode="<<mode_text<<"\n";
			 os<<"input.pick="
			   <<(mode==SkyMode::Pick?pick_csv:"")<<"\n";
			 os<<"kind\tcode\tname\tregion\tis_solar_system\tis_juxing\tmag_v\t"
				 "ra_deg\tdec_deg\taz_deg\talt_deg\n";
			 for(const auto&row : sky){
				 os<<row.kind<<"\t"<<row.code<<"\t"<<row.name<<"\t"
				   <<row.region<<"\t"<<(row.is_solar_system?"1":"0")<<"\t"
				   <<(row.is_juxing?"1":"0")<<"\t"<<node_num(row.mag_v)<<"\t"
				   <<format_num(row.ra_deg)<<"\t"<<format_num(row.dec_deg)
				   <<"\t"<<format_num(row.az_deg)<<"\t"
				   <<format_num(row.alt_deg)<<"\n";
			 }
		 }},
	};
	run_fmt(fmt_handlers,format,"sky");
	note_out(out_path,quiet);
	return 0;
}

void use_sky(){
	std::cout
		<<"Usage:\n"
		<<"  lunar sky <bsp> <time> --lat <deg> --lon <deg>\n"
		<<"  lunar sky <bsp> --time <time> --lat <deg> --lon <deg>\n"
		<<"    [--height <m>] [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]\n"
		<<"    [--mode all|pick] [--pick sun,moon,Spica,HR5056,...]\n"
		<<"    [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar sky D:\\de442.bsp 2025-06-01T20:00:00+08:00 --lat 31.23 "
		  "--lon 121.47\n"
		<<"  lunar sky D:\\de442.bsp --time 2025-06-01T20:00 --input-tz +08:00 "
		  "--lat 31.23 --lon 121.47 --mode pick --pick sun,moon,Spica\n"
		<<"Notes:\n"
		<<"  Output is topocentric apparent position at the specified instant.\n"
		<<"  Solar-system targets are listed before catalog stars.\n";
}
