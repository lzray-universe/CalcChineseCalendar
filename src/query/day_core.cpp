namespace lunar::core{

DayResult compute_day(const DayComputeOptions&opt){
	int y=0;
	int m=0;
	int d=0;
	std::tie(y,m,d)=parse_ymd(opt.date_text);
	int hh=12;
	int mm=0;
	double ss=0.0;
	parse_hms(opt.at_time,hh,mm,ss);

	int tz_off=parse_tz(opt.tz);
	double smp_jdutc=greg2jd(y,m,d,hh,mm,ss)-UTC8DAY;
	double day_sutc=cst_midjd(y,m,d);
	double day_eutc=day_sutc+1.0;

	StarPick astro_pick;
	if(opt.include_astro){
		StarMode mode=parse_star_mode(opt.astro_mode_text);
		astro_pick=make_star_pick(mode,opt.astro_pick_csv);
	}
	AstroObs astro_obs;
	if(opt.has_astro_site){
		astro_obs.has_site=true;
		astro_obs.lat_deg=opt.astro_lat_deg;
		astro_obs.lon_deg=opt.astro_lon_deg;
		astro_obs.h_m=opt.astro_height_m;
	}

	EphRead eph(opt.ephem);
	QueryCache cache(eph);

	DayResult out;
	out.ephem=opt.ephem;
	out.date_text=opt.date_text;
	out.at_time=opt.at_time;
	out.tz=opt.tz;
	out.hli_lon_deg=opt.hli_lon_deg;
	out.inc_astro=opt.include_astro;
	out.astro_mode_text=opt.astro_mode_text;
	out.astro_pick_csv=opt.astro_pick_csv;
	out.astro_obs=astro_obs;
	out.at_data=at_fromjd(eph,smp_jdutc,tz_off,opt.tz,opt.date_text+"T"+opt.at_time,
						  "+08:00",false,false,0.0,opt.hli_lon_deg,&cache);

	if(opt.include_events){
		std::set<int> years={y-1,y,y+1};
		std::vector<EventRec> all=
			col_eyrs(eph,years,tz_off,opt.quiet?nullptr:&std::cerr);
		for(const auto&ev : all){
			if(ev.jd_utc>=day_sutc&&ev.jd_utc<day_eutc){
				out.day_events.push_back(ev);
			}
		}
		std::sort(out.day_events.begin(),out.day_events.end(),
				  [](const EventRec&a,const EventRec&b){
					  return a.jd_utc<b.jd_utc;
				  });
	}

	if(opt.include_astro){
		std::vector<AstroEvt> raw=
			calc_astro_evt(eph,day_sutc,day_eutc,astro_pick,astro_obs);
		out.astro_events.reserve(raw.size());
		for(const auto&ev : raw){
			out.astro_events.push_back(mk_astro_rec(ev,tz_off));
		}
	}

	return out;
}

}
