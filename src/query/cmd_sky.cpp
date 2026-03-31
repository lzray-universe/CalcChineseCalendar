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

void wr_sky_csv(CsvWriter&w,const SkyPos&row){
	w.write_field("kind",row.kind);
	w.write_field("code",row.code);
	w.write_field("name",row.name);
	w.write_field("region",row.region);
	w.write_field("is_solar_system",row.is_solar_system);
	w.write_field("is_juxing",row.is_juxing);
	w.write_raw("mag_v",csv_num_or_empty(row.mag_v));
	w.write_raw("ra_deg",format_num(row.ra_deg));
	w.write_raw("dec_deg",format_num(row.dec_deg));
	w.write_raw("az_deg",format_num(row.az_deg));
	w.write_raw("alt_deg",format_num(row.alt_deg));
	w.finish_row();
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
	lunar::ArgParser parser;
	parser.add_flag("-h",[&](){ opt.help=true; })
		.add_flag("--help",[&](){ opt.help=true; })
		.add_value("--time",[&](const std::string&v){ opt.time_raw=v; })
		.add_value("--input-tz",[&](const std::string&v){ opt.input_tz=v; })
		.add_value("--tz",[&](const std::string&v){ opt.tz=v; })
		.add_value("--format",[&](const std::string&v){ opt.format=to_low(v); })
		.add_value("--out",[&](const std::string&v){ opt.out_path=v; })
		.add_value("--pretty",[&](const std::string&v){
			opt.pretty=parse_bool01(v,"--pretty");
		})
		.add_flag("--quiet",[&](){ opt.quiet=true; })
		.add_value("--mode",[&](const std::string&v){ opt.mode_text=to_low(v); })
		.add_value("--pick",[&](const std::string&v){ opt.pick_csv=v; })
		.add_value("--lat",[&](const std::string&v){
			opt.lat_deg=parse_double(v,"--lat");
			has_lat=true;
		})
		.add_value("--lon",[&](const std::string&v){
			opt.lon_deg=parse_double(v,"--lon");
			has_lon=true;
		})
		.add_value("--height",[&](const std::string&v){
			opt.height_m=parse_double(v,"--height");
		});

	for(;idx<args.size();++idx){
		if(!parser.parse_one(args,idx,"sky")){
			throw std::invalid_argument("unknown option for sky: "+args[idx]);
		}
		if(opt.help){
			return opt;
		}
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
			 CsvWriter csv(os);
			 for(const auto&row : res.rows){
				 wr_sky_csv(csv,row);
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
