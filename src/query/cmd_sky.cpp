namespace{

struct SkyOpt{
	std::string ephem;
	bool help=false;
	std::string time_raw;
	std::string input_tz;
	std::string tz;
	std::string format;
	std::string out_path;
	bool pretty=false;
	bool quiet=false;
	std::string mode_text;
	std::string pick_csv;
	double lat_deg=0.0;
	double lon_deg=0.0;
	double height_m=0.0;
};

struct SkyRes{
	IsoTime parsed;
	std::string tz_in;
	int tz_off=0;
	SkyMode mode=SkyMode::All;
	std::vector<SkyPos> rows;
};

std::string csv_num_or_empty(double value){
	return std::isfinite(value)?format_num(value):"";
}

SkyOpt parse_sky(const std::vector<std::string>&args){
	if(args.empty()){
		throw std::invalid_argument(
			"sky requires: <bsp> <time> --lat <deg> --lon <deg>");
	}

	InterCfg cfg=load_def();
	SkyOpt opt;
	opt.ephem=args[0];
	opt.input_tz=cfg.default_tz;
	opt.tz=cfg.default_tz;
	opt.format=to_low(cfg.def_fmt);
	if(opt.format!="txt"&&opt.format!="json"&&opt.format!="csv"){
		opt.format="txt";
	}
	opt.pretty=cfg.def_prety;
	opt.mode_text="all";

	bool has_lat=false;
	bool has_lon=false;
	std::size_t idx=1;
	if(idx<args.size()&&!is_opt(args[idx])){
		opt.time_raw=args[idx];
		++idx;
	}

	const OptMap handlers={
		{"--time",[&](const std::vector<std::string>&src,std::size_t&i,
					  const std::string&tag){
			 opt.time_raw=req_val(src,i,tag);
		 }},
		{"--input-tz",[&](const std::vector<std::string>&src,std::size_t&i,
						  const std::string&tag){
			 opt.input_tz=req_val(src,i,tag);
		 }},
		{"--tz",[&](const std::vector<std::string>&src,std::size_t&i,
					const std::string&tag){ opt.tz=req_val(src,i,tag); }},
		{"--format",[&](const std::vector<std::string>&src,std::size_t&i,
						const std::string&tag){
			 opt.format=to_low(req_val(src,i,tag));
		 }},
		{"--out",[&](const std::vector<std::string>&src,std::size_t&i,
					 const std::string&tag){ opt.out_path=req_val(src,i,tag); }},
		{"--pretty",[&](const std::vector<std::string>&src,std::size_t&i,
						const std::string&tag){
			 opt.pretty=parse_bool01(req_val(src,i,tag),"--pretty");
		 }},
		{"--quiet",[&](const std::vector<std::string>&,std::size_t&,
					   const std::string&){ opt.quiet=true; }},
		{"--mode",[&](const std::vector<std::string>&src,std::size_t&i,
					  const std::string&tag){
			 opt.mode_text=to_low(req_val(src,i,tag));
		 }},
		{"--pick",[&](const std::vector<std::string>&src,std::size_t&i,
					  const std::string&tag){
			 opt.pick_csv=req_val(src,i,tag);
		 }},
		{"--lat",[&](const std::vector<std::string>&src,std::size_t&i,
					 const std::string&tag){
			 opt.lat_deg=parse_double(req_val(src,i,tag),"--lat");
			 has_lat=true;
		 }},
		{"--lon",[&](const std::vector<std::string>&src,std::size_t&i,
					 const std::string&tag){
			 opt.lon_deg=parse_double(req_val(src,i,tag),"--lon");
			 has_lon=true;
		 }},
		{"--height",[&](const std::vector<std::string>&src,std::size_t&i,
						const std::string&tag){
			 opt.height_m=parse_double(req_val(src,i,tag),"--height");
		 }},
	};

	for(;idx<args.size();++idx){
		const std::string&tag=args[idx];
		if(tag=="-h"||tag=="--help"){
			opt.help=true;
			return opt;
		}
		apply_opt(handlers,args,idx,tag,"sky");
	}

	if(opt.time_raw.empty()){
		throw std::invalid_argument("sky requires a <time> or --time <time>");
	}
	if(has_lat!=has_lon||!has_lat){
		throw std::invalid_argument("sky requires --lat and --lon");
	}
	chk_fmt(opt.format,{"json","txt","csv"},"sky");
	return opt;
}

SkyRes run_sky(const SkyOpt&opt){
	SkyRes res;
	res.mode=parse_sky_mode(opt.mode_text);
	SkyPick pick=make_sky_pick(res.mode,opt.pick_csv);
	res.parsed=parse_iso(opt.time_raw,opt.input_tz);
	res.tz_off=parse_tz(opt.tz);
	res.tz_in=
		res.parsed.has_tz?fmt_tz(res.parsed.tz_off):fmt_tz(parse_tz(opt.input_tz));

	AstroObs obs;
	obs.has_site=true;
	obs.lat_deg=opt.lat_deg;
	obs.lon_deg=opt.lon_deg;
	obs.h_m=opt.height_m;

	EphRead eph(opt.ephem);
	res.rows=calc_sky_pos(eph,res.parsed.jd_utc,obs,pick);
	return res;
}

void write_sky(std::ostream&os,const SkyOpt&opt,const SkyRes&res){
	const double jd_tdb=TimeScale::utc_to_tdb(res.parsed.jd_utc);
	const FmtMap fmts={
		{"json",[&](){
			 JsonWriter w(os,opt.pretty);
			 w.obj_begin();
			 write_meta(w,opt.ephem,opt.tz,{"type=sky","topocentric=true"});
			 w.key("input");
			 w.obj_begin();
			 w.key("time_raw");
			 w.value(opt.time_raw);
			 w.key("input_tz");
			 w.value(res.tz_in);
			 w.key("display_tz");
			 w.value(opt.tz);
			 w.key("jd_utc");
			 w.value(res.parsed.jd_utc);
			 w.key("jd_tdb");
			 w.value(jd_tdb);
			 w.key("utc_iso");
			 w.value(fmt_iso(res.parsed.jd_utc,0,true));
			 w.key("loc_iso");
			 w.value(fmt_iso(res.parsed.jd_utc,res.tz_off,true));
			 w.key("lat_deg");
			 w.value(opt.lat_deg);
			 w.key("lon_deg");
			 w.value(opt.lon_deg);
			 w.key("height_m");
			 w.value(opt.height_m);
			 w.key("mode");
			 w.value(opt.mode_text);
			 w.key("pick");
			 if(res.mode==SkyMode::Pick){
				 w.value(opt.pick_csv);
			 }else{
				 w.null_val();
			 }
			 w.obj_end();
			 w.key("data");
			 w.arr_begin();
			 for(const auto&row : res.rows){
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
			 os<<"\n";
		 }},
		{"csv",[&](){
			 os<<"kind,code,name,region,is_solar_system,is_juxing,mag_v,"
				  "ra_deg,dec_deg,az_deg,alt_deg\n";
			 for(const auto&row : res.rows){
				 os<<csv_quote(row.kind)<<","
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
			 os<<"tool=lunar format=txt type=sky tz_display="<<opt.tz<<"\n";
			 os<<"input.time_raw="<<opt.time_raw<<"\n";
			 os<<"input.input_tz="<<res.tz_in<<"\n";
			 os<<"input.display_tz="<<opt.tz<<"\n";
			 os<<"input.jd_utc="<<format_num(res.parsed.jd_utc)<<"\n";
			 os<<"input.jd_tdb="<<format_num(jd_tdb)<<"\n";
			 os<<"input.utc_iso="<<fmt_iso(res.parsed.jd_utc,0,true)<<"\n";
			 os<<"input.loc_iso="<<fmt_iso(res.parsed.jd_utc,res.tz_off,true)
			   <<"\n";
			 os<<"input.lat_deg="<<format_num(opt.lat_deg)<<"\n";
			 os<<"input.lon_deg="<<format_num(opt.lon_deg)<<"\n";
			 os<<"input.height_m="<<format_num(opt.height_m)<<"\n";
			 os<<"input.mode="<<opt.mode_text<<"\n";
			 os<<"input.pick="<<(res.mode==SkyMode::Pick?opt.pick_csv:"")
			   <<"\n";
			 os<<"kind\tcode\tname\tregion\tis_solar_system\tis_juxing\tmag_v\t"
				 "ra_deg\tdec_deg\taz_deg\talt_deg\n";
			 for(const auto&row : res.rows){
				 os<<row.kind<<"\t"<<row.code<<"\t"<<row.name<<"\t"
				   <<row.region<<"\t"<<(row.is_solar_system?"1":"0")<<"\t"
				   <<(row.is_juxing?"1":"0")<<"\t"<<node_num(row.mag_v)<<"\t"
				   <<format_num(row.ra_deg)<<"\t"<<format_num(row.dec_deg)
				   <<"\t"<<format_num(row.az_deg)<<"\t"
				   <<format_num(row.alt_deg)<<"\n";
			 }
		 }},
	};
	run_fmt(fmts,opt.format,"sky");
}

}

int cmd_sky(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_sky();
		return 0;
	}

	SkyOpt opt=parse_sky(args);
	if(opt.help){
		use_sky();
		return 0;
	}
	SkyRes res=run_sky(opt);
	OutTgt out=open_out(opt.out_path);
	write_sky(*out.stream,opt,res);
	note_out(opt.out_path,opt.quiet);
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
