#include "lunar/star.hpp"

#include<algorithm>
#include<array>
#include<cctype>
#include<cmath>
#include<iomanip>
#include<limits>
#include<sstream>
#include<stdexcept>
#include<unordered_set>

#include "lunar/app_long.hpp"
#include "lunar/frames.hpp"
#include "lunar/time_scale.hpp"

namespace lunar{

namespace{

constexpr double kJ2000Jd=2451545.0;
constexpr double kDayPerYear=365.25;
constexpr double kAuPerPc=206264.80624709636;
constexpr double kPcPerAu=1.0/kAuPerPc;
constexpr double kSunDeflRad=1.97412574336e-8;
constexpr double kSunDeflSepMax=5.0*PI/180.0;
constexpr double kMoonRadiusKm=1737.4;
constexpr double kSunRadiusKm=695700.0;

double clamp_u(double v){
	return std::clamp(v,-1.0,1.0);
}

double norm2pi(double angle){
	double v=std::fmod(angle,TWO_PI);
	if(v<0.0){
		v+=TWO_PI;
	}
	return v;
}

double norm_pm_pi(double angle){
	double v=norm2pi(angle+PI);
	return v-PI;
}

double ang_sep(const Vec3&a,const Vec3&b){
	double na=a.norm();
	double nb=b.norm();
	if(!(na>0.0)||!(nb>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double c=Vec3::dot(a,b)/(na*nb);
	return std::acos(clamp_u(c));
}

Vec3 unit_or(const Vec3&v,const Vec3&fallback){
	double n=v.norm();
	if(!(n>0.0)){
		return fallback;
	}
	return v/n;
}

std::string low_ascii(std::string text){
	for(char&ch : text){
		ch=static_cast<char>(
			std::tolower(static_cast<unsigned char>(ch)));
	}
	return text;
}

std::string trim_copy(const std::string&text){
	std::size_t b=0;
	while(b<text.size()&&std::isspace(static_cast<unsigned char>(text[b]))){
		++b;
	}
	std::size_t e=text.size();
	while(e>b&&std::isspace(static_cast<unsigned char>(text[e-1]))){
		--e;
	}
	return text.substr(b,e-b);
}

std::vector<std::string> split_csv(const std::string&text){
	std::vector<std::string> out;
	std::size_t start=0;
	while(start<=text.size()){
		std::size_t pos=text.find(',',start);
		if(pos==std::string::npos){
			pos=text.size();
		}
		std::string token=trim_copy(text.substr(start,pos-start));
		if(!token.empty()){
			out.push_back(token);
		}
		if(pos==text.size()){
			break;
		}
		start=pos+1;
	}
	return out;
}

const char*nz(const char*s){
	return s?s:"";
}

std::string star_name(const StarRecord&st){
	if(*nz(st.zh)!='\0'){
		return st.zh;
	}
	if(*nz(st.en)!='\0'){
		return st.en;
	}
	return nz(st.id);
}

bool eq_low(const char*field,const std::string&target_low){
	if(target_low.empty()){
		return false;
	}
	return low_ascii(nz(field))==target_low;
}

bool eq_exact(const char*field,const std::string&target){
	if(target.empty()){
		return false;
	}
	return std::string(nz(field))==target;
}

bool star_match(const StarRecord&st,const std::string&token,
				const std::string&token_low){
	return eq_low(st.id,token_low)||eq_low(st.en,token_low)||
		   eq_low(st.bayer,token_low)||eq_low(st.region,token_low)||
		   eq_exact(st.zh,token)||eq_exact(st.ja,token)||
		   eq_exact(st.ko,token);
}

std::vector<const StarRecord*> select_stars(const StarPick&pick){
	std::vector<const StarRecord*> out;
	if(pick.mode==StarMode::Less){
		std::size_t n=std::min<std::size_t>(200,B_STARS_COUNT);
		out.reserve(n);
		for(std::size_t i=0;i<n;++i){
			out.push_back(&B_STARS[i]);
		}
		return out;
	}
	if(pick.mode==StarMode::All){
		out.reserve(B_STARS_COUNT);
		for(std::size_t i=0;i<B_STARS_COUNT;++i){
			out.push_back(&B_STARS[i]);
		}
		return out;
	}
	out.reserve(pick.picks.size());
	std::unordered_set<std::size_t> added;
	for(const std::string&tok : pick.picks){
		const std::string tok_low=low_ascii(tok);
		bool found=false;
		for(std::size_t i=0;i<B_STARS_COUNT;++i){
			if(!star_match(B_STARS[i],tok,tok_low)){
				continue;
			}
			found=true;
			if(added.insert(i).second){
				out.push_back(&B_STARS[i]);
			}
		}
		if(!found){
			throw std::invalid_argument("star not found in --astro-pick: "+tok);
		}
	}
	return out;
}

std::vector<const StarRecord*> occult_star_set(const StarPick&pick){
	return select_stars(pick);
}

Mat3 eq_true(double jd_tdb){
	Mat3 b=CoordTf::bias_mat();
	Mat3 p=PrecNut::prec_mat(jd_tdb);
	Mat3 n=PrecNut::nut_mat(jd_tdb);
	return n*p*b;
}

struct AppCtx{
	double jd_tdb=0.0;
	double dt_yr=0.0;
	Vec3 earth_pos_pc;
	Vec3 earth_vel_au_day;
	Vec3 sun_geo_au;
	Mat3 eq_true_mat;
	Vec3 moon_eq_u;
};

AppCtx make_ctx_tdb(EphRead&eph,double jd_tdb){
	AppCtx ctx;
	ctx.jd_tdb=jd_tdb;
	ctx.dt_yr=(jd_tdb-kJ2000Jd)/kDayPerYear;
	auto earth=eph.get_state(eph.EARTH,eph.SSB,jd_tdb);
	ctx.earth_pos_pc=earth.first*kPcPerAu;
	ctx.earth_vel_au_day=earth.second;
	ctx.sun_geo_au=eph.get_pos(eph.SUN,eph.EARTH,jd_tdb);
	ctx.eq_true_mat=eq_true(jd_tdb);
	Vec3 moon_geo=AberCorr::geo_app(eph,eph.MOON,jd_tdb,4);
	ctx.moon_eq_u=unit_or(ctx.eq_true_mat*moon_geo,Vec3(1.0,0.0,0.0));
	return ctx;
}

Vec3 apply_solar_deflection(const Vec3&u,const AppCtx&ctx){
	double rs=ctx.sun_geo_au.norm();
	if(!(rs>0.0)){
		return u;
	}
	Vec3 s=ctx.sun_geo_au/rs;
	double dot=clamp_u(Vec3::dot(u,s));
	if(!(dot>0.0)){
		return u;
	}
	double sep=std::acos(dot);
	if(!(sep<kSunDeflSepMax)){
		return u;
	}
	double den=1.0-dot;
	if(!(den>1e-12)){
		return u;
	}
	Vec3 d=(kSunDeflRad/rs)*(((dot*u)-s)/den);
	return unit_or(u+d,u);
}

Vec3 apply_annual_aberration(const Vec3&u,const AppCtx&ctx){
	Vec3 beta=ctx.earth_vel_au_day/C_AUDAY;
	double b2=Vec3::dot(beta,beta);
	if(!(b2<1.0)){
		return u;
	}
	double ginv=std::sqrt(std::max(0.0,1.0-b2));
	double ub=Vec3::dot(u,beta);
	double den=1.0+ub;
	if(std::fabs(den)<1e-15){
		return u;
	}
	Vec3 app=(ginv*u+beta+((ub*beta)/(1.0+ginv)))/den;
	return unit_or(app,u);
}

Vec3 star_eq_u(const StarRecord&st,const AppCtx&ctx){
	Vec3 p(st.x_pc+st.vx_pc_yr*ctx.dt_yr,st.y_pc+st.vy_pc_yr*ctx.dt_yr,
		   st.z_pc+st.vz_pc_yr*ctx.dt_yr);
	Vec3 geo_pc=p-ctx.earth_pos_pc;
	Vec3 u=unit_or(geo_pc,Vec3(1.0,0.0,0.0));
	u=apply_solar_deflection(u,ctx);
	u=apply_annual_aberration(u,ctx);
	return unit_or(ctx.eq_true_mat*u,Vec3(1.0,0.0,0.0));
}

void to_ra_dec_deg(const Vec3&u,double&ra_deg,double&dec_deg){
	double ra=std::atan2(u.y,u.x);
	if(ra<0.0){
		ra+=TWO_PI;
	}
	double dec=std::asin(clamp_u(u.z));
	ra_deg=ra*180.0/PI;
	dec_deg=dec*180.0/PI;
}

double body_radius_km(int id){
	switch(id){
		case 10:
			return kSunRadiusKm;
		case 301:
			return kMoonRadiusKm;
		case 199:
			return 2439.7;
		case 299:
			return 6051.8;
		case 499:
			return 3389.5;
		case 599:
			return 69911.0;
		case 699:
			return 58232.0;
		case 799:
			return 25362.0;
		case 899:
			return 24622.0;
		default:
			return 0.0;
	}
}

std::string body_zh(int id){
	switch(id){
		case 199:
			return "水星";
		case 299:
			return "金星";
		case 301:
			return "月球";
		case 499:
			return "火星";
		case 599:
			return "木星";
		case 699:
			return "土星";
		case 799:
			return "天王星";
		case 899:
			return "海王星";
		default:
			return "天体";
	}
}

std::string body_code(int id){
	switch(id){
		case 199:
			return "mercury";
		case 299:
			return "venus";
		case 301:
			return "moon";
		case 499:
			return "mars";
		case 599:
			return "jupiter";
		case 699:
			return "saturn";
		case 799:
			return "uranus";
		case 899:
			return "neptune";
		default:
			return "body";
	}
}

std::unordered_set<int> spk_obj_set(EphRead&eph){
	std::unordered_set<int> out;
	for(int id : eph.spk_objects()){
		out.insert(id);
	}
	return out;
}

bool has_spk_obj(const std::unordered_set<int>&ids,int id){
	return ids.find(id)!=ids.end();
}

double body_sd_rad(EphRead&eph,int id,double jd_tdb){
	double rkm=body_radius_km(id);
	if(!(rkm>0.0)){
		return 0.0;
	}
	Vec3 geo=eph.get_pos(id,eph.EARTH,jd_tdb);
	double dkm=geo.norm()*AU_KM;
	if(!(dkm>rkm)){
		return 0.0;
	}
	return std::asin(clamp_u(rkm/dkm));
}

Vec3 body_eq_u(EphRead&eph,int id,double jd_tdb){
	Mat3 m=eq_true(jd_tdb);
	Vec3 geo=AberCorr::geo_app(eph,id,jd_tdb,4);
	return unit_or(m*geo,Vec3(1.0,0.0,0.0));
}

double sep_sun_body(EphRead&eph,int id,double jd_tdb){
	Vec3 sun_u=body_eq_u(eph,eph.SUN,jd_tdb);
	Vec3 body_u=body_eq_u(eph,id,jd_tdb);
	return ang_sep(sun_u,body_u);
}

double sep_moon_body(EphRead&eph,int id,double jd_tdb){
	Vec3 moon_u=body_eq_u(eph,eph.MOON,jd_tdb);
	Vec3 body_u=body_eq_u(eph,id,jd_tdb);
	return ang_sep(moon_u,body_u);
}

double sep_moon_star(EphRead&eph,const StarRecord&st,double jd_tdb){
	AppCtx ctx=make_ctx_tdb(eph,jd_tdb);
	Vec3 star_u=star_eq_u(st,ctx);
	return ang_sep(star_u,ctx.moon_eq_u);
}

double sep_moon_star_ctx(const StarRecord&st,const AppCtx&ctx){
	Vec3 star_u=star_eq_u(st,ctx);
	return ang_sep(star_u,ctx.moon_eq_u);
}

double parabolic_peak(double x1,double y1,double x2,double y2,double x3,
					  double y3){
	double den=(x1-x2)*(x1-x3)*(x2-x3);
	if(std::fabs(den)<1e-18){
		return x2;
	}
	double a=(x3*(y2-y1)+x2*(y1-y3)+x1*(y3-y2))/den;
	double b=(x3*x3*(y1-y2)+x2*x2*(y3-y1)+x1*x1*(y2-y3))/den;
	if(std::fabs(a)<1e-18){
		return x2;
	}
	double xv=-b/(2.0*a);
	if(xv<x1||xv>x3){
		return x2;
	}
	return xv;
}

template<typename Fn>
double bisect_root(Fn&&fn,double a,double b,int it=64){
	double fa=fn(a);
	double fb=fn(b);
	if(!std::isfinite(fa)||!std::isfinite(fb)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	if(fa==0.0){
		return a;
	}
	if(fb==0.0){
		return b;
	}
	if(fa*fb>0.0){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double lo=a;
	double hi=b;
	double flo=fa;
	for(int i=0;i<it;++i){
		double mid=0.5*(lo+hi);
		double fm=fn(mid);
		if(!std::isfinite(fm)){
			return std::numeric_limits<double>::quiet_NaN();
		}
		if(std::fabs(fm)<1e-14||std::fabs(hi-lo)<1e-10){
			return mid;
		}
		if(fm*flo<=0.0){
			hi=mid;
		}else{
			lo=mid;
			flo=fm;
		}
	}
	return 0.5*(lo+hi);
}

double body_lam(AppLon&app,int id,double jd_tdb){
	RetProp st=AberCorr::geo_prop(app.eph,id,jd_tdb);
	Mat3 r=app.rot_mat(jd_tdb);
	Vec3 xec=r*st.X;
	double lam=std::atan2(xec.y,xec.x);
	if(lam<0.0){
		lam+=TWO_PI;
	}
	return lam;
}

std::string deg_note(const std::string&key,double value_deg){
	std::ostringstream oss;
	oss<<key<<"="<<std::fixed<<std::setprecision(3)<<value_deg;
	return oss.str();
}

void sort_uniq(std::vector<AstroEvt>&out){
	std::sort(out.begin(),out.end(),[](const AstroEvt&a,const AstroEvt&b){
		if(a.jd_utc!=b.jd_utc){
			return a.jd_utc<b.jd_utc;
		}
		if(a.kind!=b.kind){
			return a.kind<b.kind;
		}
		return a.code<b.code;
	});
	std::vector<AstroEvt> uniq;
	uniq.reserve(out.size());
	for(const auto&ev : out){
		if(!uniq.empty()){
			const AstroEvt&last=uniq.back();
			if(last.code==ev.code&&std::fabs(last.jd_utc-ev.jd_utc)<0.08){
				continue;
			}
		}
		uniq.push_back(ev);
	}
	out.swap(uniq);
}

void find_gelong(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
				 double ed_utc,const std::unordered_set<int>&spk_ids){
	AppLon app(eph);
	double t0=TimeScale::utc_to_tdb(st_utc)-5.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+5.0;
	const double step=0.25;
	const std::array<int,2> body_ids={199,299};
	for(int id : body_ids){
		if(!has_spk_obj(spk_ids,id)){
			continue;
		}
		std::vector<double> ts;
		std::vector<double> fs;
		ts.reserve(static_cast<std::size_t>((t1-t0)/step)+8);
		fs.reserve(ts.capacity());
		for(double t=t0;t<=t1+1e-12;t+=step){
			double ds=norm_pm_pi(body_lam(app,id,t)-app.sun_calc(t).first);
			ts.push_back(t);
			fs.push_back(std::fabs(ds));
		}
		for(std::size_t i=1;i+1<ts.size();++i){
			if(!(fs[i]>=fs[i-1]&&fs[i]>fs[i+1])){
				continue;
			}
			double tp=
				parabolic_peak(ts[i-1],fs[i-1],ts[i],fs[i],ts[i+1],fs[i+1]);
			double d=norm_pm_pi(body_lam(app,id,tp)-app.sun_calc(tp).first);
			double f=std::fabs(d);
			if(!(f>=5.0*PI/180.0)){
				continue;
			}
			double ju=TimeScale::tdb_to_utc(tp);
			if(ju<st_utc||ju>=ed_utc){
				continue;
			}
			bool east=d>0.0;
			AstroEvt ev;
			ev.kind="greatest_elongation";
			ev.code=(id==199)?"mercury_":"venus_";
			ev.code+=east?"east":"west";
			ev.name=body_zh(id)+(east?"东大距":"西大距");
			ev.jd_utc=ju;
			ev.detail=deg_note("elong_deg",f*180.0/PI);
			out.push_back(std::move(ev));
		}
	}
}

void find_quadrature(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
					 double ed_utc,const std::unordered_set<int>&ids){
	struct QDef{
		int id=0;
		double step=0.25;
	};
	const std::array<QDef,6> defs={
		QDef{301,0.08},QDef{499,0.25},QDef{599,0.25},
		QDef{699,0.25},QDef{799,0.25},QDef{899,0.25},
	};
	AppLon app(eph);
	double t0=TimeScale::utc_to_tdb(st_utc)-2.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+2.0;
	for(const auto&def : defs){
		if(!has_spk_obj(ids,def.id)){
			continue;
		}
		for(double target : {0.5*PI,-0.5*PI}){
			auto fn=[&](double t){
				double d=norm_pm_pi(body_lam(app,def.id,t)-app.sun_calc(t).first);
				return norm_pm_pi(d-target);
			};
			double tp=t0;
			double fp=fn(tp);
			for(double tc=t0+def.step;tc<=t1+1e-12;tc+=def.step){
				double fc=fn(tc);
				bool cross=(fp==0.0)||(fc==0.0)||(fp<0.0&&fc>0.0)||
						   (fp>0.0&&fc<0.0);
				if(cross&&std::fabs(fp)<1.5&&std::fabs(fc)<1.5){
					double tr=bisect_root(fn,tp,tc);
					if(std::isfinite(tr)){
						double ju=TimeScale::tdb_to_utc(tr);
						if(ju>=st_utc&&ju<ed_utc){
							bool east=target>0.0;
							AstroEvt ev;
							ev.kind="quadrature";
							ev.code=body_code(def.id);
							ev.code+=east?"_east":"_west";
							ev.name=body_zh(def.id)+(east?"东方照":"西方照");
							ev.jd_utc=ju;
							out.push_back(std::move(ev));
						}
					}
				}
				tp=tc;
				fp=fc;
			}
		}
	}
}

template<typename SepFn,typename ThFn,typename MkFn>
void scan_min_evt(std::vector<AstroEvt>&out,double t0,double t1,double step,
				  const SepFn&sep_fn,const ThFn&th_fn,const MkFn&mk_evt,
				  double st_utc,double ed_utc){
	double t_prev=t0;
	double f_prev=sep_fn(t_prev);
	double t_cur=t_prev+step;
	double f_cur=sep_fn(t_cur);
	for(double t_next=t_cur+step;t_next<=t1+1e-12;t_next+=step){
		double f_next=sep_fn(t_next);
		if(std::isfinite(f_prev)&&std::isfinite(f_cur)&&std::isfinite(f_next)&&
		   f_cur<=f_prev&&f_cur<f_next){
			double tp=
				parabolic_peak(t_prev,f_prev,t_cur,f_cur,t_next,f_next);
			double fp=sep_fn(tp);
			double th=th_fn(tp);
			if(std::isfinite(fp)&&std::isfinite(th)&&fp<=th){
				double ju=TimeScale::tdb_to_utc(tp);
				if(ju>=st_utc&&ju<ed_utc){
					out.push_back(mk_evt(ju,fp));
				}
			}
		}
		t_prev=t_cur;
		f_prev=f_cur;
		t_cur=t_next;
		f_cur=f_next;
	}
}

void find_transit(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
				  double ed_utc,const std::unordered_set<int>&ids){
	if(!has_spk_obj(ids,eph.SUN)){
		return;
	}
	double t0=TimeScale::utc_to_tdb(st_utc)-3.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+3.0;
	const double step=0.05;
	for(int id : {199,299}){
		if(!has_spk_obj(ids,id)){
			continue;
		}
		auto sep_fn=[&](double t){ return sep_sun_body(eph,id,t); };
		auto th_fn=[&](double t){
			return body_sd_rad(eph,eph.SUN,t)+body_sd_rad(eph,id,t);
		};
		auto mk_evt=[&](double ju,double sep){
			AstroEvt ev;
			ev.kind="transit";
			ev.code=(id==199)?"mercury_transit":"venus_transit";
			ev.name=body_zh(id)+"凌日";
			ev.jd_utc=ju;
			ev.detail=deg_note("sep_deg",sep*180.0/PI);
			return ev;
		};
		scan_min_evt(out,t0,t1,step,sep_fn,th_fn,mk_evt,st_utc,ed_utc);
	}
}

void find_occult(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
				 double ed_utc,const StarPick&pick,
				 const std::unordered_set<int>&ids){
	if(!has_spk_obj(ids,eph.MOON)){
		return;
	}
	double t0=TimeScale::utc_to_tdb(st_utc)-1.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+1.0;
	const double step_planet=0.05;
	for(int id : {199,299,499,599,699,799,899}){
		if(!has_spk_obj(ids,id)){
			continue;
		}
		auto sep_fn=[&](double t){ return sep_moon_body(eph,id,t); };
		auto th_fn=[&](double t){
			return body_sd_rad(eph,eph.MOON,t)+body_sd_rad(eph,id,t);
		};
		auto mk_evt=[&](double ju,double sep){
			AstroEvt ev;
			ev.kind="occultation";
			ev.code="moon_occult_"+body_code(id);
			ev.name="月掩"+body_zh(id);
			ev.jd_utc=ju;
			ev.detail=deg_note("sep_deg",sep*180.0/PI);
			return ev;
		};
		scan_min_evt(out,t0,t1,step_planet,sep_fn,th_fn,mk_evt,st_utc,ed_utc);
	}

	const std::vector<const StarRecord*> stars=occult_star_set(pick);
	const double step_star=0.08;
	if(stars.empty()){
		return;
	}

	std::vector<double> ts;
	for(double t=t0;t<=t1+1e-12;t+=step_star){
		ts.push_back(t);
	}
	if(ts.size()<3){
		return;
	}

	std::vector<AppCtx> ctxs;
	ctxs.reserve(ts.size());
	for(double t : ts){
		ctxs.push_back(make_ctx_tdb(eph,t));
	}

	for(const StarRecord*sp : stars){
		const StarRecord&st=*sp;
		std::vector<double> seps(ts.size(),std::numeric_limits<double>::quiet_NaN());
		for(std::size_t i=0;i<ts.size();++i){
			seps[i]=sep_moon_star_ctx(st,ctxs[i]);
		}
		for(std::size_t i=1;i+1<ts.size();++i){
			double f_prev=seps[i-1];
			double f_cur=seps[i];
			double f_next=seps[i+1];
			if(!std::isfinite(f_prev)||!std::isfinite(f_cur)||!std::isfinite(f_next)){
				continue;
			}
			if(!(f_cur<=f_prev&&f_cur<f_next)){
				continue;
			}
			double tp=parabolic_peak(ts[i-1],f_prev,ts[i],f_cur,ts[i+1],f_next);
			AppCtx ctx_peak=make_ctx_tdb(eph,tp);
			double sep=sep_moon_star_ctx(st,ctx_peak);
			double th=body_sd_rad(eph,eph.MOON,tp);
			if(!std::isfinite(sep)||!std::isfinite(th)||sep>th){
				continue;
			}
			double ju=TimeScale::tdb_to_utc(tp);
			if(ju<st_utc||ju>=ed_utc){
				continue;
			}
			AstroEvt ev;
			ev.kind="occultation";
			ev.code="moon_occult_"+std::string(nz(st.id));
			ev.name="月掩"+star_name(st);
			ev.jd_utc=ju;
			ev.detail=deg_note("sep_deg",sep*180.0/PI);
			out.push_back(std::move(ev));
		}
	}
}

void find_station(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
				  double ed_utc,const std::unordered_set<int>&ids){
	AppLon app(eph);
	double t0=TimeScale::utc_to_tdb(st_utc)-30.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+30.0;
	const double step=1.0;
	const double h=0.2;
	for(int id : {199,299,499,599,699,799,899}){
		if(!has_spk_obj(ids,id)){
			continue;
		}
		auto vel=[&](double t){
			double l1=body_lam(app,id,t-h);
			double l2=body_lam(app,id,t+h);
			return norm_pm_pi(l2-l1)/(2.0*h);
		};
		double tp=t0;
		double vp=vel(tp);
		for(double tc=t0+step;tc<=t1+1e-12;tc+=step){
			double vc=vel(tc);
			bool cross=(vp==0.0)||(vc==0.0)||(vp<0.0&&vc>0.0)||
					   (vp>0.0&&vc<0.0);
			if(cross&&std::fabs(vp)<0.2&&std::fabs(vc)<0.2){
				double tr=bisect_root(vel,tp,tc);
				if(std::isfinite(tr)){
					double v_before=vel(tr-0.2);
					double v_after=vel(tr+0.2);
					bool to_retro=(v_before>0.0&&v_after<0.0);
					bool to_prog=(v_before<0.0&&v_after>0.0);
					if(to_retro||to_prog){
						double ju=TimeScale::tdb_to_utc(tr);
						if(ju>=st_utc&&ju<ed_utc){
							AstroEvt ev;
							ev.kind="stationary";
							ev.code=body_code(id);
							ev.code+=to_retro?"_station_retro":"_station_prog";
							ev.name=body_zh(id)+(to_retro?"留(转逆)":"留(转顺)");
							ev.jd_utc=ju;
							ev.detail=std::string("state=")+
									  (to_retro?"retrograde":"prograde");
							out.push_back(std::move(ev));
						}
					}
				}
			}
			tp=tc;
			vp=vc;
		}
	}
}

}

StarMode parse_star_mode(const std::string&text){
	const std::string t=low_ascii(trim_copy(text));
	if(t.empty()||t=="less"){
		return StarMode::Less;
	}
	if(t=="all"){
		return StarMode::All;
	}
	if(t=="pick"){
		return StarMode::Pick;
	}
	throw std::invalid_argument(
		"invalid --astro-mode: "+text+" (expected less|all|pick)");
}

StarPick make_star_pick(StarMode mode,const std::string&pick_csv){
	StarPick pick;
	pick.mode=mode;
	if(mode==StarMode::Pick){
		pick.picks=split_csv(pick_csv);
		if(pick.picks.empty()){
			throw std::invalid_argument(
				"--astro-pick is required when --astro-mode=pick");
		}
	}
	return pick;
}

std::vector<StarApp> calc_star_app(EphRead&eph,double jd_utc,const StarPick&pick){
	double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	AppCtx ctx=make_ctx_tdb(eph,jd_tdb);
	const std::vector<const StarRecord*> stars=select_stars(pick);
	std::vector<StarApp> out;
	out.reserve(stars.size());
	for(const StarRecord*sp : stars){
		const StarRecord&st=*sp;
		Vec3 su=star_eq_u(st,ctx);
		double ra_deg=0.0;
		double dec_deg=0.0;
		to_ra_dec_deg(su,ra_deg,dec_deg);
		double sep=ang_sep(su,ctx.moon_eq_u)*180.0/PI;
		StarApp rec;
		rec.id=nz(st.id);
		rec.name=star_name(st);
		rec.region=nz(st.region);
		rec.mag_v=st.mag_v;
		rec.is_juxing=st.is_juxing;
		rec.ra_deg=ra_deg;
		rec.dec_deg=dec_deg;
		rec.sep_moon_deg=sep;
		out.push_back(std::move(rec));
	}
	return out;
}

MoonXg calc_moon_xg(EphRead&eph,double jd_utc){
	MoonXg out;
	double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	AppCtx ctx=make_ctx_tdb(eph,jd_tdb);
	bool found=false;
	double best=std::numeric_limits<double>::infinity();
	for(std::size_t i=0;i<B_STARS_COUNT;++i){
		const StarRecord&st=B_STARS[i];
		if(!st.is_juxing){
			continue;
		}
		Vec3 su=star_eq_u(st,ctx);
		double sep=ang_sep(su,ctx.moon_eq_u);
		if(sep<best){
			best=sep;
			out.region=nz(st.region);
			out.star_id=nz(st.id);
			out.star_name=star_name(st);
			found=true;
		}
	}
	if(!found&&B_STARS_COUNT>0){
		const StarRecord&st=B_STARS[0];
		out.region=nz(st.region);
		out.star_id=nz(st.id);
		out.star_name=star_name(st);
		best=std::numeric_limits<double>::quiet_NaN();
	}
	out.sep_deg=best*180.0/PI;
	return out;
}

std::vector<AstroEvt> calc_astro_evt(EphRead&eph,double jd_utc_start,
									 double jd_utc_end,const StarPick&pick){
	std::vector<AstroEvt> out;
	if(!std::isfinite(jd_utc_start)||!std::isfinite(jd_utc_end)||
	   !(jd_utc_end>jd_utc_start)){
		return out;
	}
	const std::unordered_set<int> ids=spk_obj_set(eph);
	find_gelong(out,eph,jd_utc_start,jd_utc_end,ids);
	find_quadrature(out,eph,jd_utc_start,jd_utc_end,ids);
	find_transit(out,eph,jd_utc_start,jd_utc_end,ids);
	find_occult(out,eph,jd_utc_start,jd_utc_end,pick,ids);
	find_station(out,eph,jd_utc_start,jd_utc_end,ids);
	sort_uniq(out);
	return out;
}

}
