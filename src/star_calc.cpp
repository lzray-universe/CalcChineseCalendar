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
#include "lunar/i18n.hpp"
#include "lunar/precnut_core.hpp"
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
constexpr double kDegPerRad=180.0/PI;
constexpr double kRadPerDeg=PI/180.0;
constexpr double kEarthRotationRateRadPerDay=1.00273781191135448*TWO_PI;

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
	return i18n::tr_star_name(st);
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

struct ObsSite{
	bool on=false;
	Vec3 ecef;
	Vec3 up_ecef;
	Vec3 beta_ecef;
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
	return i18n::tr_body_name(id);
}

std::string body_code(int id){
	switch(id){
		case 10:
			return "sun";
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

bool finite_vec(const Vec3&v){
	return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}

Vec3 geodetic_to_ecef(double lat_deg,double lon_deg,double h_m){
	constexpr double a_m=6378137.0;
	constexpr double inv_f=298.257223563;
	const double f=1.0/inv_f;
	const double e2=f*(2.0-f);

	double lat=lat_deg*kRadPerDeg;
	double lon=lon_deg*kRadPerDeg;
	double s_lat=std::sin(lat);
	double c_lat=std::cos(lat);
	double s_lon=std::sin(lon);
	double c_lon=std::cos(lon);

	double n=a_m/std::sqrt(1.0-e2*s_lat*s_lat);
	double x=(n+h_m)*c_lat*c_lon;
	double y=(n+h_m)*c_lat*s_lon;
	double z=(n*(1.0-e2)+h_m)*s_lat;

	double au_m=AU_KM*1000.0;
	return Vec3(x/au_m,y/au_m,z/au_m);
}

Vec3 up_ecef_geo(double lat_deg,double lon_deg){
	double lat=lat_deg*kRadPerDeg;
	double lon=lon_deg*kRadPerDeg;
	double c_lat=std::cos(lat);
	return Vec3(c_lat*std::cos(lon),c_lat*std::sin(lon),std::sin(lat));
}

Vec3 observer_beta_ecef(const Vec3&obs_ecef){
	Vec3 vel_au_day(-kEarthRotationRateRadPerDay*obs_ecef.y,
					kEarthRotationRateRadPerDay*obs_ecef.x,0.0);
	return vel_au_day/C_AUDAY;
}

Vec3 apply_diurnal_aberration(const Vec3&dir,const Vec3&obs_beta){
	double beta2=Vec3::dot(obs_beta,obs_beta);
	if(!(beta2>0.0)){
		return dir;
	}
	double ginv=std::sqrt(std::max(0.0,1.0-beta2));
	double nb=Vec3::dot(dir,obs_beta);
	double den=1.0+nb;
	if(!(den>0.0)){
		return dir;
	}
	Vec3 d=(ginv*dir)+obs_beta+((nb/(1.0+ginv))*obs_beta);
	d=d/den;
	return unit_or(d,dir);
}

Mat3 ecef_rot(double jd_tdb){
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	double uta=std::floor(jd_utc);
	double utb=jd_utc-uta;
	double tta=std::floor(jd_tdb);
	double ttb=jd_tdb-tta;
	double gast=lunar::precnut::gst06a(uta,utb,tta,ttb);
	return CoordTf::R3(gast);
}

ObsSite make_obs_site(const AstroObs&obs){
	if(!obs.has_site){
		return ObsSite{};
	}
	if(!std::isfinite(obs.lat_deg)||!std::isfinite(obs.lon_deg)||
	   !std::isfinite(obs.h_m)){
		throw std::invalid_argument("astro observer coordinates must be finite");
	}
	if(obs.lat_deg<-90.0||obs.lat_deg>90.0){
		throw std::invalid_argument("astro latitude out of range [-90,90]");
	}
	if(obs.lon_deg<-180.0||obs.lon_deg>180.0){
		throw std::invalid_argument("astro longitude out of range [-180,180]");
	}
	ObsSite out;
	out.on=true;
	out.ecef=geodetic_to_ecef(obs.lat_deg,obs.lon_deg,obs.h_m);
	out.up_ecef=up_ecef_geo(obs.lat_deg,obs.lon_deg);
	out.beta_ecef=observer_beta_ecef(out.ecef);
	return out;
}

bool topo_body_dir_sd(EphRead&eph,int phys_id,int eph_id,double jd_tdb,
					  const ObsSite&obs,Vec3&u_ecef,double&sd_rad){
	Vec3 geo=AberCorr::geo_app(eph,eph_id,jd_tdb,4);
	if(!finite_vec(geo)){
		return false;
	}
	Vec3 eq=eq_true(jd_tdb)*geo;
	Vec3 body_ecef=ecef_rot(jd_tdb)*eq;
	Vec3 topo=body_ecef-obs.ecef;
	double rn=topo.norm();
	if(!(rn>0.0)){
		return false;
	}
	u_ecef=unit_or(topo,Vec3(1.0,0.0,0.0));
	u_ecef=apply_diurnal_aberration(u_ecef,obs.beta_ecef);
	double r_km=body_radius_km(phys_id);
	if(r_km>0.0){
		sd_rad=std::asin(clamp_u(r_km/(rn*AU_KM)));
	}else{
		sd_rad=0.0;
	}
	return std::isfinite(sd_rad);
}

bool topo_star_dir(EphRead&eph,const StarRecord&st,double jd_tdb,
				   const ObsSite&obs,Vec3&u_ecef){
	AppCtx ctx=make_ctx_tdb(eph,jd_tdb);
	Vec3 u_eq=star_eq_u(st,ctx);
	u_ecef=ecef_rot(jd_tdb)*u_eq;
	u_ecef=unit_or(u_ecef,Vec3(1.0,0.0,0.0));
	u_ecef=apply_diurnal_aberration(u_ecef,obs.beta_ecef);
	return finite_vec(u_ecef);
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

int bary_id(int id){
	switch(id){
		case 199:
			return 1;
		case 299:
			return 2;
		case 399:
			return 3;
		case 499:
			return 4;
		case 599:
			return 5;
		case 699:
			return 6;
		case 799:
			return 7;
		case 899:
			return 8;
		default:
			return 0;
	}
}

int eph_id_or_zero(const std::unordered_set<int>&ids,int id,bool allow_bary){
	if(has_spk_obj(ids,id)){
		return id;
	}
	if(!allow_bary){
		return 0;
	}
	int bid=bary_id(id);
	if(bid>0&&has_spk_obj(ids,bid)){
		return bid;
	}
	return 0;
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
		int eph_id=eph_id_or_zero(spk_ids,id,true);
		if(eph_id==0){
			continue;
		}
		std::vector<double> ts;
		std::vector<double> fs;
		ts.reserve(static_cast<std::size_t>((t1-t0)/step)+8);
		fs.reserve(ts.capacity());
		for(double t=t0;t<=t1+1e-12;t+=step){
			double ds=norm_pm_pi(body_lam(app,eph_id,t)-app.sun_calc(t).first);
			ts.push_back(t);
			fs.push_back(std::fabs(ds));
		}
		for(std::size_t i=1;i+1<ts.size();++i){
			if(!(fs[i]>=fs[i-1]&&fs[i]>fs[i+1])){
				continue;
			}
			double tp=
				parabolic_peak(ts[i-1],fs[i-1],ts[i],fs[i],ts[i+1],fs[i+1]);
			double d=norm_pm_pi(body_lam(app,eph_id,tp)-app.sun_calc(tp).first);
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

struct GeoBody{
	int id=0;
	int eph_id=0;
	std::string code;
	std::string zh;
};

enum class AspType{
	Conj,
	Opp,
	Quad,
};

struct AspDef{
	AspType type=AspType::Conj;
	double target=0.0;
	bool east=false;
};

std::string code_token(const std::string&text){
	std::string low=low_ascii(text);
	std::string out;
	out.reserve(low.size());
	for(char ch : low){
		unsigned char uc=static_cast<unsigned char>(ch);
		if(std::isalnum(uc)){
			out.push_back(ch);
		}else if(out.empty()||out.back()!='_'){
			out.push_back('_');
		}
	}
	while(!out.empty()&&out.back()=='_'){
		out.pop_back();
	}
	return out;
}

double lerp_root(double t0,double f0,double t1,double f1){
	if(!std::isfinite(t0)||!std::isfinite(t1)||!std::isfinite(f0)||
	   !std::isfinite(f1)||!(t1>t0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	if(f0==f1){
		return 0.5*(t0+t1);
	}
	double t=t0+(-f0)*(t1-t0)/(f1-f0);
	if(t<t0||t>t1){
		return 0.5*(t0+t1);
	}
	return t;
}

bool zero_cross(double f0,double f1){
	return (f0==0.0)||(f1==0.0)||(f0<0.0&&f1>0.0)||(f0>0.0&&f1<0.0);
}

std::vector<GeoBody> geo_body_list(const std::unordered_set<int>&ids){
	const std::array<int,9> req={10,301,199,299,499,599,699,799,899};
	std::vector<GeoBody> out;
	out.reserve(req.size());
	for(int id : req){
		int eph_id=0;
		if(id==10){
			eph_id=10;
		}else if(id==301){
			eph_id=eph_id_or_zero(ids,id,false);
		}else{
			eph_id=eph_id_or_zero(ids,id,true);
		}
		if(id!=10&&eph_id==0){
			continue;
		}
		GeoBody b;
		b.id=id;
		b.eph_id=eph_id;
		b.code=body_code(id);
		b.zh=body_zh(id);
		out.push_back(std::move(b));
	}
	return out;
}

double geo_body_lam(AppLon&app,const GeoBody&b,double jd_tdb){
	if(b.id==10){
		return app.sun_calc(jd_tdb).first;
	}
	return body_lam(app,b.eph_id,jd_tdb);
}

std::pair<std::size_t,std::size_t> orient_pair_idx(const std::vector<GeoBody>&b,
												   std::size_t i,std::size_t j){
	const GeoBody&a=b[i];
	const GeoBody&c=b[j];
	if(a.id==10){
		return {j,i};
	}
	if(c.id==10){
		return {i,j};
	}
	if(a.id==301&&c.id!=10){
		return {i,j};
	}
	if(c.id==301&&a.id!=10){
		return {j,i};
	}
	if(a.id<=c.id){
		return {i,j};
	}
	return {j,i};
}

std::string rel_obj_name(const GeoBody&b){
	return b.id==10?"日":b.zh;
}

AstroEvt mk_body_aspect_evt(const GeoBody&obj,const GeoBody&ref,const AspDef&asp,
							double jd_utc){
	AstroEvt ev;
	ev.jd_utc=jd_utc;
	switch(asp.type){
		case AspType::Conj:
			ev.kind="conjunction";
			if(ref.id==10){
				ev.code=obj.code+"_conjunction";
				ev.name=obj.zh+"合日";
			}else{
				ev.code=obj.code+"_"+ref.code+"_conjunction";
				ev.name=obj.zh+"合"+ref.zh;
			}
			break;
		case AspType::Opp:
			ev.kind="opposition";
			if(ref.id==10){
				ev.code=obj.code+"_opposition";
				ev.name=obj.zh+"冲日";
			}else{
				ev.code=obj.code+"_"+ref.code+"_opposition";
				ev.name=obj.zh+"冲"+ref.zh;
			}
			break;
		case AspType::Quad:
			ev.kind="quadrature";
			if(ref.id==10){
				ev.code=obj.code+(asp.east?"_east":"_west");
				ev.name=obj.zh+(asp.east?"东方照":"西方照");
			}else{
				ev.code=obj.code+"_"+ref.code+(asp.east?"_east":"_west");
				ev.name=obj.zh+(asp.east?"东方照":"西方照")+ref.zh;
			}
			break;
	}
	return ev;
}

AstroEvt mk_star_body_aspect_evt(const StarRecord&st,const std::string&tok,
								 const GeoBody&body,const AspDef&asp,double jd_utc){
	AstroEvt ev;
	ev.jd_utc=jd_utc;
	const std::string nm=star_name(st);
	const std::string rel=rel_obj_name(body);
	switch(asp.type){
		case AspType::Conj:
			ev.kind="conjunction";
			ev.code="star_"+tok+"_"+body.code+"_conjunction";
			ev.name=nm+"合"+rel;
			break;
		case AspType::Opp:
			ev.kind="opposition";
			ev.code="star_"+tok+"_"+body.code+"_opposition";
			ev.name=nm+"冲"+rel;
			break;
		case AspType::Quad:
			ev.kind="quadrature";
			ev.code="star_"+tok+"_"+body.code+(asp.east?"_east":"_west");
			ev.name=nm+(asp.east?"东方照":"西方照")+rel;
			break;
	}
	return ev;
}

void find_body_aspect(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
					  double ed_utc,const std::unordered_set<int>&ids){
	const std::vector<GeoBody> bodies=geo_body_list(ids);
	if(bodies.size()<2){
		return;
	}
	const std::array<AspDef,4> aspects={
		AspDef{AspType::Conj,0.0,false},
		AspDef{AspType::Opp,PI,false},
		AspDef{AspType::Quad,0.5*PI,true},
		AspDef{AspType::Quad,-0.5*PI,false},
	};
	double t0=TimeScale::utc_to_tdb(st_utc)-2.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+2.0;
	const double step=0.05;
	std::vector<double> ts;
	for(double t=t0;t<=t1+1e-12;t+=step){
		ts.push_back(t);
	}
	if(ts.size()<2){
		return;
	}
	AppLon app(eph);
	std::vector<std::vector<double>> lam(
		bodies.size(),std::vector<double>(ts.size(),std::numeric_limits<double>::quiet_NaN()));
	for(std::size_t b=0;b<bodies.size();++b){
		for(std::size_t i=0;i<ts.size();++i){
			lam[b][i]=geo_body_lam(app,bodies[b],ts[i]);
		}
	}
	for(std::size_t i=0;i<bodies.size();++i){
		for(std::size_t j=i+1;j<bodies.size();++j){
			auto idx=orient_pair_idx(bodies,i,j);
			std::size_t oi=idx.first;
			std::size_t ri=idx.second;
			for(const auto&asp : aspects){
				for(std::size_t k=1;k<ts.size();++k){
					double d0=norm_pm_pi(lam[oi][k-1]-lam[ri][k-1]);
					double d1=norm_pm_pi(lam[oi][k]-lam[ri][k]);
					if(!std::isfinite(d0)||!std::isfinite(d1)){
						continue;
					}
					double f0=norm_pm_pi(d0-asp.target);
					double f1=norm_pm_pi(d1-asp.target);
					if(!zero_cross(f0,f1)||std::fabs(f0)>1.7||std::fabs(f1)>1.7){
						continue;
					}
					double tr=lerp_root(ts[k-1],f0,ts[k],f1);
					if(!std::isfinite(tr)){
						continue;
					}
					double ju=TimeScale::tdb_to_utc(tr);
					if(ju<st_utc||ju>=ed_utc){
						continue;
					}
					out.push_back(mk_body_aspect_evt(bodies[oi],bodies[ri],asp,ju));
				}
			}
		}
	}
}

void find_star_body_aspect(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
						   double ed_utc,const StarPick&pick,
						   const std::unordered_set<int>&ids){
	const std::vector<const StarRecord*> stars=select_stars(pick);
	if(stars.empty()){
		return;
	}
	const std::vector<GeoBody> bodies=geo_body_list(ids);
	if(bodies.empty()){
		return;
	}
	const std::array<AspDef,4> aspects={
		AspDef{AspType::Conj,0.0,false},
		AspDef{AspType::Opp,PI,false},
		AspDef{AspType::Quad,0.5*PI,true},
		AspDef{AspType::Quad,-0.5*PI,false},
	};
	double t0=TimeScale::utc_to_tdb(st_utc)-2.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+2.0;
	const double step=(pick.mode==StarMode::All)?0.10:0.05;
	std::vector<double> ts;
	for(double t=t0;t<=t1+1e-12;t+=step){
		ts.push_back(t);
	}
	if(ts.size()<2){
		return;
	}
	AppLon app(eph);
	std::vector<AppCtx> ctxs;
	ctxs.reserve(ts.size());
	std::vector<Mat3> rots;
	rots.reserve(ts.size());
	for(double t : ts){
		ctxs.push_back(make_ctx_tdb(eph,t));
		rots.push_back(app.rot_mat(t));
	}
	std::vector<std::vector<double>> lam_body(
		bodies.size(),std::vector<double>(ts.size(),std::numeric_limits<double>::quiet_NaN()));
	for(std::size_t b=0;b<bodies.size();++b){
		for(std::size_t i=0;i<ts.size();++i){
			lam_body[b][i]=geo_body_lam(app,bodies[b],ts[i]);
		}
	}
	for(const StarRecord*sp : stars){
		const StarRecord&st=*sp;
		std::string tok=code_token(nz(st.id));
		if(tok.empty()){
			tok=code_token(star_name(st));
		}
		if(tok.empty()){
			tok="star";
		}
		std::vector<double> lam_star(ts.size(),std::numeric_limits<double>::quiet_NaN());
		for(std::size_t i=0;i<ts.size();++i){
			Vec3 su=star_eq_u(st,ctxs[i]);
			Vec3 xec=rots[i]*su;
			double lm=std::atan2(xec.y,xec.x);
			if(lm<0.0){
				lm+=TWO_PI;
			}
			lam_star[i]=lm;
		}
		for(std::size_t b=0;b<bodies.size();++b){
			for(const auto&asp : aspects){
				for(std::size_t k=1;k<ts.size();++k){
					double d0=norm_pm_pi(lam_star[k-1]-lam_body[b][k-1]);
					double d1=norm_pm_pi(lam_star[k]-lam_body[b][k]);
					if(!std::isfinite(d0)||!std::isfinite(d1)){
						continue;
					}
					double f0=norm_pm_pi(d0-asp.target);
					double f1=norm_pm_pi(d1-asp.target);
					if(!zero_cross(f0,f1)||std::fabs(f0)>1.7||std::fabs(f1)>1.7){
						continue;
					}
					double tr=lerp_root(ts[k-1],f0,ts[k],f1);
					if(!std::isfinite(tr)){
						continue;
					}
					double ju=TimeScale::tdb_to_utc(tr);
					if(ju<st_utc||ju>=ed_utc){
						continue;
					}
					out.push_back(mk_star_body_aspect_evt(st,tok,bodies[b],asp,ju));
				}
			}
		}
	}
}

template<typename GapFn>
double find_contact_root_back(const GapFn&gap_fn,double t_center,double t_floor,
							  double step){
	if(!(step>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double t_hi=t_center;
	double g_hi=gap_fn(t_hi);
	if(!std::isfinite(g_hi)||g_hi>0.0){
		return std::numeric_limits<double>::quiet_NaN();
	}
	for(double t=t_center-step;t>=t_floor-1e-12;t-=step){
		double g=gap_fn(t);
		if(!std::isfinite(g)){
			continue;
		}
		if(g==0.0){
			return t;
		}
		if(g>0.0&&g_hi<=0.0){
			return bisect_root(gap_fn,t,t_hi);
		}
		t_hi=t;
		g_hi=g;
	}
	return std::numeric_limits<double>::quiet_NaN();
}

template<typename GapFn>
double find_contact_root_fwd(const GapFn&gap_fn,double t_center,double t_ceiling,
							 double step){
	if(!(step>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double t_lo=t_center;
	double g_lo=gap_fn(t_lo);
	if(!std::isfinite(g_lo)||g_lo>0.0){
		return std::numeric_limits<double>::quiet_NaN();
	}
	for(double t=t_center+step;t<=t_ceiling+1e-12;t+=step){
		double g=gap_fn(t);
		if(!std::isfinite(g)){
			continue;
		}
		if(g==0.0){
			return t;
		}
		if(g_lo<=0.0&&g>0.0){
			return bisect_root(gap_fn,t_lo,t);
		}
		t_lo=t;
		g_lo=g;
	}
	return std::numeric_limits<double>::quiet_NaN();
}

void push_contact_evt(std::vector<AstroEvt>&out,const std::string&kind,
					  const std::string&base_code,const std::string&base_name,
					  const char*phase_code,const char*phase_zh,double jd_utc,
					  double sep_rad,double st_utc,double ed_utc){
	if(!std::isfinite(jd_utc)||jd_utc<st_utc||jd_utc>=ed_utc){
		return;
	}
	AstroEvt ev;
	ev.kind=kind;
	ev.code=base_code+"_"+phase_code;
	ev.name=base_name+phase_zh;
	ev.jd_utc=jd_utc;
	if(std::isfinite(sep_rad)){
		ev.detail=deg_note("sep_deg",sep_rad*kDegPerRad);
	}
	out.push_back(std::move(ev));
}

void find_transit(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
				  double ed_utc,const std::unordered_set<int>&ids,
				  const ObsSite&obs){
	if(!obs.on){
		return;
	}
	int sun_eph_id=eph_id_or_zero(ids,eph.SUN,false);
	if(sun_eph_id==0){
		return;
	}
	double t0=TimeScale::utc_to_tdb(st_utc)-3.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+3.0;
	const double step=0.02;
	const double root_step=0.004;
	for(int id : {199,299}){
		int eph_id=eph_id_or_zero(ids,id,true);
		if(eph_id==0){
			continue;
		}
		auto geom=[&](double t,double&sep,double&gap){
			Vec3 sun_u;
			Vec3 body_u;
			double sun_sd=0.0;
			double body_sd=0.0;
			// Guard against false "transit" near superior conjunction:
			// only accept when the inner planet is truly in front of the Sun.
			if(!topo_body_dir_sd(eph,eph.SUN,sun_eph_id,t,obs,sun_u,sun_sd)||
			   !topo_body_dir_sd(eph,id,eph_id,t,obs,body_u,body_sd)){
				return false;
			}
			Vec3 sun_geo=eph.get_pos(sun_eph_id,eph.EARTH,t);
			Vec3 body_geo=eph.get_pos(eph_id,eph.EARTH,t);
			double sun_range=sun_geo.norm();
			double body_range=body_geo.norm();
			if(!std::isfinite(sun_range)||!std::isfinite(body_range)){
				return false;
			}
			if(!(body_range<sun_range)){
				return false;
			}
			sep=ang_sep(sun_u,body_u);
			double lim=sun_sd+body_sd;
			gap=sep-lim;
			return std::isfinite(sep)&&std::isfinite(gap);
		};
		auto sep_fn=[&](double t){
			double sep=0.0;
			double gap=0.0;
			if(!geom(t,sep,gap)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			return sep;
		};
		auto gap_fn=[&](double t){
			double sep=0.0;
			double gap=0.0;
			if(!geom(t,sep,gap)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			return gap;
		};
		double t_prev=t0;
		double f_prev=sep_fn(t_prev);
		double t_cur=t_prev+step;
		double f_cur=sep_fn(t_cur);
		double last_tmax=std::numeric_limits<double>::quiet_NaN();
		for(double t_next=t_cur+step;t_next<=t1+1e-12;t_next+=step){
			double f_next=sep_fn(t_next);
			if(std::isfinite(f_prev)&&std::isfinite(f_cur)&&std::isfinite(f_next)&&
			   f_cur<=f_prev&&f_cur<f_next){
				double t_max=
					parabolic_peak(t_prev,f_prev,t_cur,f_cur,t_next,f_next);
				double sep_max=sep_fn(t_max);
				double gap_max=gap_fn(t_max);
				if(std::isfinite(sep_max)&&std::isfinite(gap_max)&&gap_max<=0.0){
					if(std::isfinite(last_tmax)&&
					   std::fabs(last_tmax-t_max)<(1.5*step)){
						t_prev=t_cur;
						f_prev=f_cur;
						t_cur=t_next;
						f_cur=f_next;
						continue;
					}
					last_tmax=t_max;
					double t_in=find_contact_root_back(gap_fn,t_max,t0,root_step);
					double t_out=find_contact_root_fwd(gap_fn,t_max,t1,root_step);
					const std::string base_code=
						(id==199)?"mercury_transit":"venus_transit";
					const std::string base_name=body_zh(id)+"凌日";
					push_contact_evt(out,"transit",base_code,base_name,"ingress",
									 "入",TimeScale::tdb_to_utc(t_in),sep_fn(t_in),
									 st_utc,ed_utc);
					push_contact_evt(out,"transit",base_code,base_name,"max","最大",
									 TimeScale::tdb_to_utc(t_max),sep_max,st_utc,
									 ed_utc);
					push_contact_evt(out,"transit",base_code,base_name,"egress",
									 "出",TimeScale::tdb_to_utc(t_out),
									 sep_fn(t_out),st_utc,ed_utc);
				}
			}
			t_prev=t_cur;
			f_prev=f_cur;
			t_cur=t_next;
			f_cur=f_next;
		}
	}
}

void find_occult(std::vector<AstroEvt>&out,EphRead&eph,double st_utc,
				 double ed_utc,const StarPick&pick,
				 const std::unordered_set<int>&ids,const ObsSite&obs){
	if(!obs.on){
		return;
	}
	int moon_eph_id=eph_id_or_zero(ids,eph.MOON,false);
	if(moon_eph_id==0){
		return;
	}
	double t0=TimeScale::utc_to_tdb(st_utc)-1.0;
	double t1=TimeScale::utc_to_tdb(ed_utc)+1.0;
	const double step_planet=0.02;
	const double root_step_planet=0.004;
	for(int id : {199,299,499,599,699,799,899}){
		int eph_id=eph_id_or_zero(ids,id,true);
		if(eph_id==0){
			continue;
		}
		auto geom=[&](double t,double&sep,double&gap){
			Vec3 moon_u;
			Vec3 body_u;
			double moon_sd=0.0;
			double body_sd=0.0;
			if(!topo_body_dir_sd(eph,eph.MOON,moon_eph_id,t,obs,moon_u,moon_sd)||
			   !topo_body_dir_sd(eph,id,eph_id,t,obs,body_u,body_sd)){
				return false;
			}
			sep=ang_sep(moon_u,body_u);
			double lim=moon_sd+body_sd;
			gap=sep-lim;
			return std::isfinite(sep)&&std::isfinite(gap);
		};
		auto sep_fn=[&](double t){
			double sep=0.0;
			double gap=0.0;
			if(!geom(t,sep,gap)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			return sep;
		};
		auto gap_fn=[&](double t){
			double sep=0.0;
			double gap=0.0;
			if(!geom(t,sep,gap)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			return gap;
		};
		double t_prev=t0;
		double f_prev=sep_fn(t_prev);
		double t_cur=t_prev+step_planet;
		double f_cur=sep_fn(t_cur);
		double last_tmax=std::numeric_limits<double>::quiet_NaN();
		for(double t_next=t_cur+step_planet;t_next<=t1+1e-12;t_next+=step_planet){
			double f_next=sep_fn(t_next);
			if(std::isfinite(f_prev)&&std::isfinite(f_cur)&&std::isfinite(f_next)&&
			   f_cur<=f_prev&&f_cur<f_next){
				double t_max=
					parabolic_peak(t_prev,f_prev,t_cur,f_cur,t_next,f_next);
				double sep_max=sep_fn(t_max);
				double gap_max=gap_fn(t_max);
				if(std::isfinite(sep_max)&&std::isfinite(gap_max)&&gap_max<=0.0){
					if(std::isfinite(last_tmax)&&
					   std::fabs(last_tmax-t_max)<(1.5*step_planet)){
						t_prev=t_cur;
						f_prev=f_cur;
						t_cur=t_next;
						f_cur=f_next;
						continue;
					}
					last_tmax=t_max;
					double t_in=
						find_contact_root_back(gap_fn,t_max,t0,root_step_planet);
					double t_out=
						find_contact_root_fwd(gap_fn,t_max,t1,root_step_planet);
					const std::string base_code="moon_occult_"+body_code(id);
					const std::string base_name="月掩"+body_zh(id);
					push_contact_evt(out,"occultation",base_code,base_name,
									 "ingress","入",TimeScale::tdb_to_utc(t_in),
									 sep_fn(t_in),st_utc,ed_utc);
					push_contact_evt(out,"occultation",base_code,base_name,"max",
									 "最大",TimeScale::tdb_to_utc(t_max),sep_max,
									 st_utc,ed_utc);
					push_contact_evt(out,"occultation",base_code,base_name,
									 "egress","出",TimeScale::tdb_to_utc(t_out),
									 sep_fn(t_out),st_utc,ed_utc);
				}
			}
			t_prev=t_cur;
			f_prev=f_cur;
			t_cur=t_next;
			f_cur=f_next;
		}
	}

	const std::vector<const StarRecord*> stars=occult_star_set(pick);
	if(stars.empty()){
		return;
	}
	const double step_star=0.03;
	const double root_step_star=0.0025;
	std::vector<double> ts;
	for(double t=t0;t<=t1+1e-12;t+=step_star){
		ts.push_back(t);
	}
	if(ts.size()<3){
		return;
	}
	struct TopoStarCtx{
		AppCtx app;
		Mat3 ecef_m;
		Vec3 moon_u;
		double moon_sd=std::numeric_limits<double>::quiet_NaN();
		bool ok=false;
	};
	std::vector<TopoStarCtx> ctxs;
	ctxs.reserve(ts.size());
	for(double t : ts){
		TopoStarCtx c;
		c.app=make_ctx_tdb(eph,t);
		c.ecef_m=ecef_rot(t);
		c.ok=topo_body_dir_sd(eph,eph.MOON,moon_eph_id,t,obs,c.moon_u,c.moon_sd);
		ctxs.push_back(c);
	}
	auto star_sep_idx=[&](const StarRecord&st,std::size_t idx){
		const TopoStarCtx&c=ctxs[idx];
		if(!c.ok){
			return std::numeric_limits<double>::quiet_NaN();
		}
		Vec3 u_eq=star_eq_u(st,c.app);
		Vec3 u_ecef=c.ecef_m*u_eq;
		u_ecef=unit_or(u_ecef,Vec3(1.0,0.0,0.0));
		u_ecef=apply_diurnal_aberration(u_ecef,obs.beta_ecef);
		return ang_sep(c.moon_u,u_ecef);
	};
	for(const StarRecord*sp : stars){
		const StarRecord&st=*sp;
		std::vector<double> seps(ts.size(),std::numeric_limits<double>::quiet_NaN());
		for(std::size_t i=0;i<ts.size();++i){
			seps[i]=star_sep_idx(st,i);
		}
		auto sep_fn=[&](double t){
			TopoStarCtx c;
			c.app=make_ctx_tdb(eph,t);
			c.ecef_m=ecef_rot(t);
			if(!topo_body_dir_sd(eph,eph.MOON,moon_eph_id,t,obs,c.moon_u,c.moon_sd)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			Vec3 u_eq=star_eq_u(st,c.app);
			Vec3 u_ecef=c.ecef_m*u_eq;
			u_ecef=unit_or(u_ecef,Vec3(1.0,0.0,0.0));
			u_ecef=apply_diurnal_aberration(u_ecef,obs.beta_ecef);
			return ang_sep(c.moon_u,u_ecef);
		};
		auto gap_fn=[&](double t){
			TopoStarCtx c;
			c.app=make_ctx_tdb(eph,t);
			c.ecef_m=ecef_rot(t);
			if(!topo_body_dir_sd(eph,eph.MOON,moon_eph_id,t,obs,c.moon_u,c.moon_sd)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			Vec3 u_eq=star_eq_u(st,c.app);
			Vec3 u_ecef=c.ecef_m*u_eq;
			u_ecef=unit_or(u_ecef,Vec3(1.0,0.0,0.0));
			u_ecef=apply_diurnal_aberration(u_ecef,obs.beta_ecef);
			double sep=ang_sep(c.moon_u,u_ecef);
			return sep-c.moon_sd;
		};
		double last_tmax=std::numeric_limits<double>::quiet_NaN();
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
			double t_max=parabolic_peak(ts[i-1],f_prev,ts[i],f_cur,ts[i+1],f_next);
			double sep_max=sep_fn(t_max);
			double gap_max=gap_fn(t_max);
			if(!std::isfinite(sep_max)||!std::isfinite(gap_max)||gap_max>0.0){
				continue;
			}
			if(std::isfinite(last_tmax)&&std::fabs(last_tmax-t_max)<(1.5*step_star)){
				continue;
			}
			last_tmax=t_max;
			double t_in=find_contact_root_back(gap_fn,t_max,t0,root_step_star);
			double t_out=find_contact_root_fwd(gap_fn,t_max,t1,root_step_star);
			const std::string base_code="moon_occult_"+std::string(nz(st.id));
			const std::string base_name="月掩"+star_name(st);
			push_contact_evt(out,"occultation",base_code,base_name,"ingress","入",
							 TimeScale::tdb_to_utc(t_in),sep_fn(t_in),st_utc,
							 ed_utc);
			push_contact_evt(out,"occultation",base_code,base_name,"max","最大",
							 TimeScale::tdb_to_utc(t_max),sep_max,st_utc,ed_utc);
			push_contact_evt(out,"occultation",base_code,base_name,"egress","出",
							 TimeScale::tdb_to_utc(t_out),sep_fn(t_out),st_utc,
							 ed_utc);
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
		int eph_id=eph_id_or_zero(ids,id,true);
		if(eph_id==0){
			continue;
		}
		auto vel=[&](double t){
			double l1=body_lam(app,eph_id,t-h);
			double l2=body_lam(app,eph_id,t+h);
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
									 double jd_utc_end,const StarPick&pick,
									 const AstroObs&obs){
	std::vector<AstroEvt> out;
	if(!std::isfinite(jd_utc_start)||!std::isfinite(jd_utc_end)||
	   !(jd_utc_end>jd_utc_start)){
		return out;
	}
	const std::unordered_set<int> ids=spk_obj_set(eph);
	const ObsSite site=make_obs_site(obs);
	find_gelong(out,eph,jd_utc_start,jd_utc_end,ids);
	find_body_aspect(out,eph,jd_utc_start,jd_utc_end,ids);
	find_star_body_aspect(out,eph,jd_utc_start,jd_utc_end,pick,ids);
	if(site.on){
		find_transit(out,eph,jd_utc_start,jd_utc_end,ids,site);
		find_occult(out,eph,jd_utc_start,jd_utc_end,pick,ids,site);
	}
	find_station(out,eph,jd_utc_start,jd_utc_end,ids);
	sort_uniq(out);
	return out;
}

}
