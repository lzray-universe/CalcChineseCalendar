#include "lunar/solar_eclipse.hpp"

#include "lunar/app_long.hpp"
#include "lunar/format.hpp"
#include "lunar/frames.hpp"
#include "lunar/i18n.hpp"
#include "lunar/precnut_core.hpp"
#include "lunar/time_scale.hpp"

#include<algorithm>
#include<cctype>
#include<cmath>
#include<functional>
#include<limits>
#include<stdexcept>
#include<vector>

namespace{

constexpr double kReKm=6378.1366;
constexpr double kReEqKm=6378.137;
constexpr double kRePolKm=6356.7523142;
constexpr double kRsKm=695700.0;
constexpr double kRmKm=1737.4;
constexpr double kDegPerRad=180.0/PI;
constexpr double kRadPerDeg=PI/180.0;
constexpr double kEarthRotationRateRadPerDay=1.00273781191135448*TWO_PI;

constexpr double kReA=kReKm/AU_KM;
constexpr double kReEqA=kReEqKm/AU_KM;
constexpr double kRePolA=kRePolKm/AU_KM;
constexpr double kRsA=kRsKm/AU_KM;
constexpr double kRmA=kRmKm/AU_KM;

struct Bracket{
	double left=0.0;
	double right=0.0;
	double f_left=0.0;
	double f_right=0.0;
};

struct GeoEval{
	Vec3 sun;
	Vec3 moon;
	Vec3 axis;
	Vec3 dvec;
	double sep=std::numeric_limits<double>::quiet_NaN();
	double sd_sun=std::numeric_limits<double>::quiet_NaN();
	double sd_moon=std::numeric_limits<double>::quiet_NaN();
	double outer=std::numeric_limits<double>::quiet_NaN();
	double inner=std::numeric_limits<double>::quiet_NaN();
	double mag=std::numeric_limits<double>::quiet_NaN();
	double obscuration=std::numeric_limits<double>::quiet_NaN();
	double sun_dist_km=std::numeric_limits<double>::quiet_NaN();
	double moon_dist_km=std::numeric_limits<double>::quiet_NaN();
	double x=std::numeric_limits<double>::quiet_NaN();
	double d=std::numeric_limits<double>::quiet_NaN();
	double D=std::numeric_limits<double>::quiet_NaN();
	double rp=std::numeric_limits<double>::quiet_NaN();
	double ru=std::numeric_limits<double>::quiet_NaN();
	double gamma=std::numeric_limits<double>::quiet_NaN();
};

struct PointObserver{
	Vec3 ecef;
	Vec3 up_ecef;
	Vec3 beta_ecef;
};

struct BodyEcefState{
	Vec3 sun_ecef;
	Vec3 moon_ecef;
};

struct TopoEval{
	double sep=std::numeric_limits<double>::quiet_NaN();
	double sd_sun=std::numeric_limits<double>::quiet_NaN();
	double sd_moon=std::numeric_limits<double>::quiet_NaN();
	double outer=std::numeric_limits<double>::quiet_NaN();
	double inner=std::numeric_limits<double>::quiet_NaN();
	double mag=std::numeric_limits<double>::quiet_NaN();
	double obscuration=std::numeric_limits<double>::quiet_NaN();
	double sun_alt_deg=std::numeric_limits<double>::quiet_NaN();
};

bool finite_vec(const Vec3&v){
	return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}

double clamp_unit(double v){
	if(v>1.0){
		return 1.0;
	}
	if(v<-1.0){
		return -1.0;
	}
	return v;
}

std::string to_low(std::string s){
	for(char&c : s){
		c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return s;
}

Mat3 eq_true_mat(double jd_tdb){
	Mat3 P=PrecNut::prec_mat(jd_tdb);
	Mat3 N=PrecNut::nut_mat(jd_tdb);
	return N*P*CoordTf::bias_mat();
}

Vec3 cross_vec(const Vec3&a,const Vec3&b){
	return Vec3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);
}

bool build_shadow_plane_basis(const Vec3&axis_eq,Vec3&e1,Vec3&e2){
	double an=axis_eq.norm();
	if(!(an>0.0)){
		return false;
	}
	Vec3 u=axis_eq/an;
	Vec3 k(0.0,0.0,1.0);
	Vec3 v=k-Vec3::dot(k,u)*u;
	double vn=v.norm();
	if(!(vn>1e-15)){
		Vec3 ref(1.0,0.0,0.0);
		if(std::fabs(Vec3::dot(ref,u))>0.9){
			ref=Vec3(0.0,1.0,0.0);
		}
		v=ref-Vec3::dot(ref,u)*u;
		vn=v.norm();
		if(!(vn>1e-15)){
			return false;
		}
	}
	e1=v/vn;
	e2=cross_vec(u,e1);
	double e2n=e2.norm();
	if(!(e2n>0.0)){
		return false;
	}
	e2=e2/e2n;
	return finite_vec(e1)&&finite_vec(e2);
}

bool earth_projected_axes_solar(const Vec3&axis_eq,double&re1,double&re2){
	double an=axis_eq.norm();
	if(!(an>0.0)){
		return false;
	}
	Vec3 u=axis_eq/an;
	double mu=std::fabs(clamp_unit(u.z));
	re1=std::sqrt(kReEqA*kReEqA*mu*mu+kRePolA*kRePolA*(1.0-mu*mu));
	re2=kReEqA;
	return std::isfinite(re1)&&std::isfinite(re2);
}

double ellipse_constraint(double x,double y,double a2,double b2,double lambda){
	double da=lambda+a2;
	double db=lambda+b2;
	if(!(da>0.0)||!(db>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	return (a2*x*x)/(da*da)+(b2*y*y)/(db*db)-1.0;
}

double ellipse_signed_distance(double x,double y,double a,double b){
	if(!(a>0.0)||!(b>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double q=(x*x)/(a*a)+(y*y)/(b*b);
	bool inside=q<=1.0;
	double a2=a*a;
	double b2=b*b;
	double lo=0.0;
	double hi=0.0;
	double f_lo=0.0;
	double f_hi=0.0;
	if(inside){
		double eps=std::max(1e-30,1e-12*std::min(a2,b2));
		lo=-std::min(a2,b2)+eps;
		hi=0.0;
		f_lo=ellipse_constraint(x,y,a2,b2,lo);
		f_hi=ellipse_constraint(x,y,a2,b2,hi);
		if(!std::isfinite(f_lo)||!std::isfinite(f_hi)||!(f_lo>=0.0&&f_hi<=0.0)){
			return std::numeric_limits<double>::quiet_NaN();
		}
	}else{
		lo=0.0;
		hi=std::max(a2,b2);
		f_lo=ellipse_constraint(x,y,a2,b2,lo);
		f_hi=ellipse_constraint(x,y,a2,b2,hi);
		if(!std::isfinite(f_lo)||!std::isfinite(f_hi)){
			return std::numeric_limits<double>::quiet_NaN();
		}
		int guard=0;
		while(f_hi>0.0&&guard<80){
			hi*=2.0;
			f_hi=ellipse_constraint(x,y,a2,b2,hi);
			if(!std::isfinite(f_hi)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			++guard;
		}
		if(!(f_lo>=0.0&&f_hi<=0.0)){
			return std::numeric_limits<double>::quiet_NaN();
		}
	}

	for(int i=0;i<72;++i){
		double mid=0.5*(lo+hi);
		double f_mid=ellipse_constraint(x,y,a2,b2,mid);
		if(!std::isfinite(f_mid)){
			return std::numeric_limits<double>::quiet_NaN();
		}
		if(std::fabs(f_mid)<=1e-16){
			lo=mid;
			hi=mid;
			break;
		}
		if(f_lo*f_mid<=0.0){
			hi=mid;
			f_hi=f_mid;
		}else{
			lo=mid;
			f_lo=f_mid;
		}
		(void)f_hi;
	}

	double lambda=0.5*(lo+hi);
	double da=lambda+a2;
	double db=lambda+b2;
	if(!(da>0.0)||!(db>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double xe=(a2*x)/da;
	double ye=(b2*y)/db;
	double d=std::hypot(x-xe,y-ye);
	return inside ? -d : d;
}

double circle_overlap_area(double r1,double r2,double d){
	if(!(r1>0.0)||!(r2>0.0)||!std::isfinite(d)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	if(d>=r1+r2){
		return 0.0;
	}
	double dr=std::fabs(r1-r2);
	if(d<=dr){
		double r=std::min(r1,r2);
		return PI*r*r;
	}

	double c1=(d*d+r1*r1-r2*r2)/(2.0*d*r1);
	double c2=(d*d+r2*r2-r1*r1)/(2.0*d*r2);
	c1=clamp_unit(c1);
	c2=clamp_unit(c2);

	double a1=r1*r1*std::acos(c1);
	double a2=r2*r2*std::acos(c2);
	double a3=0.5*std::sqrt(std::max(0.0,(-d+r1+r2)*(d+r1-r2)*(d-r1+r2)*(d+r1+r2)));
	return a1+a2-a3;
}

double signed_gamma_re(const Vec3&axis,const Vec3&dvec,double jd_tdb){
	double d=dvec.norm();
	if(!(d>0.0)){
		return 0.0;
	}
	double gamma_abs=d/kReA;

	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 axis_eq=eq*axis;
	Vec3 dvec_eq=eq*dvec;
	double an=axis_eq.norm();
	if(!(an>0.0)){
		return gamma_abs;
	}
	Vec3 axis_u=axis_eq/an;
	Vec3 north_eq(0.0,0.0,1.0);
	Vec3 n_proj=north_eq-Vec3::dot(north_eq,axis_u)*axis_u;
	double nn=n_proj.norm();
	if(!(nn>0.0)){
		return gamma_abs;
	}
	n_proj=n_proj/nn;
	if(Vec3::dot(dvec_eq,n_proj)<0.0){
		return -gamma_abs;
	}
	return gamma_abs;
}

bool eval_geo(EphRead&eph,double jd_tdb,GeoEval&g){
	Vec3 sun=raw_vec(AberCorr::geo_app(eph,eph.SUN,jd_tdb,3));
	Vec3 moon=raw_vec(AberCorr::geo_app(eph,eph.MOON,jd_tdb,3));
	if(!finite_vec(sun)||!finite_vec(moon)){
		return false;
	}

	double sun_n=sun.norm();
	double moon_n=moon.norm();
	if(!(sun_n>0.0)||!(moon_n>0.0)){
		return false;
	}

	Vec3 us=sun/sun_n;
	Vec3 um=moon/moon_n;
	double cs=clamp_unit(Vec3::dot(us,um));
	double sep=std::acos(cs);
	double sd_sun=std::asin(clamp_unit(kRsA/sun_n));
	double sd_moon=std::asin(clamp_unit(kRmA/moon_n));

	Vec3 moon_to_sun=sun-moon;
	double D=moon_to_sun.norm();
	if(!(D>0.0)){
		return false;
	}
	Vec3 axis=(-1.0/D)*moon_to_sun;
	Vec3 r=(-1.0)*moon;
	double x=Vec3::dot(r,axis);
	Vec3 dvec=r-x*axis;
	double d=dvec.norm();
	double rp=kRmA+x*(kRsA+kRmA)/D;
	double ru=kRmA-x*(kRsA-kRmA)/D;

	double outer=sep-(sd_sun+sd_moon);
	double inner=sep-std::fabs(sd_moon-sd_sun);
	double overlap=sd_sun+sd_moon-sep;
	double mag=std::numeric_limits<double>::quiet_NaN();
	if(sd_sun>0.0){
		mag=overlap/(2.0*sd_sun);
		if(mag<0.0){
			mag=0.0;
		}
	}
	double obsc=0.0;
	if(overlap>0.0){
		double area=circle_overlap_area(sd_sun,sd_moon,sep);
		if(std::isfinite(area)&&sd_sun>0.0){
			obsc=area/(PI*sd_sun*sd_sun);
			if(obsc<0.0){
				obsc=0.0;
			}
			if(obsc>1.0){
				obsc=1.0;
			}
		}
	}

	g.sun=sun;
	g.moon=moon;
	g.axis=axis;
	g.dvec=dvec;
	g.sep=sep;
	g.sd_sun=sd_sun;
	g.sd_moon=sd_moon;
	g.outer=outer;
	g.inner=inner;
	g.mag=mag;
	g.obscuration=obsc;
	g.sun_dist_km=sun_n*AU_KM;
	g.moon_dist_km=moon_n*AU_KM;
	g.x=x;
	g.d=d;
	g.D=D;
	g.rp=rp;
	g.ru=ru;
	g.gamma=signed_gamma_re(axis,(-1.0)*dvec,jd_tdb);
	return std::isfinite(g.sep)&&std::isfinite(g.sd_sun)&&std::isfinite(g.sd_moon)&&
		   std::isfinite(g.outer)&&std::isfinite(g.inner)&&std::isfinite(g.mag)&&
		   std::isfinite(g.obscuration)&&std::isfinite(g.sun_dist_km)&&
		   std::isfinite(g.moon_dist_km)&&std::isfinite(g.rp)&&
		   std::isfinite(g.ru)&&std::isfinite(g.gamma);
}

bool eval_global_metrics(EphRead&eph,double jd_tdb,double&f_any,double&f_cen,
						 GeoEval*gout=nullptr){
	GeoEval g;
	if(!eval_geo(eph,jd_tdb,g)){
		return false;
	}
	if(!(g.x>0.0)){
		return false;
	}

	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 axis_eq=eq*g.axis;
	Vec3 dvec_eq=eq*g.dvec;

	Vec3 e1;
	Vec3 e2;
	if(!build_shadow_plane_basis(axis_eq,e1,e2)){
		return false;
	}

	double re1=0.0;
	double re2=0.0;
	if(!earth_projected_axes_solar(axis_eq,re1,re2)){
		return false;
	}

	Vec3 shadow_center=(-1.0)*dvec_eq;
	double x=Vec3::dot(shadow_center,e1);
	double y=Vec3::dot(shadow_center,e2);
	double dist=ellipse_signed_distance(x,y,re1,re2);
	if(!std::isfinite(dist)){
		return false;
	}

	f_any=dist-g.rp;
	f_cen=dist-std::fabs(g.ru);
	if(!std::isfinite(f_any)||!std::isfinite(f_cen)){
		return false;
	}

	if(gout!=nullptr){
		*gout=g;
	}
	return true;
}

bool find_global_metric_minimum(EphRead&eph,double jd_near,bool central,
								double*jd_min,double*f_min,GeoEval*out){
	if(jd_min==nullptr||f_min==nullptr||out==nullptr||!std::isfinite(jd_near)){
		return false;
	}

	constexpr double span=2.0;
	constexpr double step=1.0/24.0;
	double best_t=std::numeric_limits<double>::quiet_NaN();
	double best_f=std::numeric_limits<double>::infinity();
	GeoEval best_g;

	for(double t=jd_near-span;t<=jd_near+span+1e-12;t+=step){
		double f_any=std::numeric_limits<double>::quiet_NaN();
		double f_cen=std::numeric_limits<double>::quiet_NaN();
		GeoEval g;
		if(!eval_global_metrics(eph,t,f_any,f_cen,&g)){
			continue;
		}
		double f=central?f_cen:f_any;
		if(f<best_f){
			best_f=f;
			best_t=t;
			best_g=g;
		}
	}
	if(!std::isfinite(best_t)||!std::isfinite(best_f)){
		return false;
	}

	double left=std::max(jd_near-span,best_t-step);
	double right=std::min(jd_near+span,best_t+step);
	if(!(right>left)){
		left=best_t-2.0*step;
		right=best_t+2.0*step;
	}

	auto fn=[&](double jd) -> double{
		double f_any=std::numeric_limits<double>::quiet_NaN();
		double f_cen=std::numeric_limits<double>::quiet_NaN();
		if(!eval_global_metrics(eph,jd,f_any,f_cen,nullptr)){
			return std::numeric_limits<double>::infinity();
		}
		return central?f_cen:f_any;
	};

	const double gr=(std::sqrt(5.0)-1.0)*0.5;
	double c=right-gr*(right-left);
	double d=left+gr*(right-left);
	double fc=fn(c);
	double fd=fn(d);

	for(int i=0;i<80&&right-left>1e-10;++i){
		if(fc<=fd){
			right=d;
			d=c;
			fd=fc;
			c=right-gr*(right-left);
			fc=fn(c);
		}else{
			left=c;
			c=d;
			fc=fd;
			d=left+gr*(right-left);
			fd=fn(d);
		}
	}

	double t_min=0.5*(left+right);
	double f_any=std::numeric_limits<double>::quiet_NaN();
	double f_cen=std::numeric_limits<double>::quiet_NaN();
	GeoEval g_min;
	if(!eval_global_metrics(eph,t_min,f_any,f_cen,&g_min)){
		return false;
	}

	*jd_min=t_min;
	*f_min=central?f_cen:f_any;
	*out=g_min;
	return true;
}

bool find_sep_minimum(EphRead&eph,double jd_near,double*jd_min,GeoEval*out){
	if(jd_min==nullptr||out==nullptr||!std::isfinite(jd_near)){
		return false;
	}

	constexpr double span=2.0;
	constexpr double step=1.0/24.0;
	double best_t=std::numeric_limits<double>::quiet_NaN();
	double best_sep=std::numeric_limits<double>::infinity();

	for(double t=jd_near-span;t<=jd_near+span+1e-12;t+=step){
		GeoEval g;
		if(!eval_geo(eph,t,g)){
			continue;
		}
		if(g.sep<best_sep){
			best_sep=g.sep;
			best_t=t;
		}
	}
	if(!std::isfinite(best_t)||!std::isfinite(best_sep)){
		return false;
	}

	double left=std::max(jd_near-span,best_t-step);
	double right=std::min(jd_near+span,best_t+step);
	if(!(right>left)){
		left=best_t-2.0*step;
		right=best_t+2.0*step;
	}

	auto fn=[&](double jd) -> double{
		GeoEval g;
		if(!eval_geo(eph,jd,g)){
			return std::numeric_limits<double>::infinity();
		}
		return g.sep;
	};

	const double gr=(std::sqrt(5.0)-1.0)*0.5;
	double c=right-gr*(right-left);
	double d=left+gr*(right-left);
	double fc=fn(c);
	double fd=fn(d);

	for(int i=0;i<80&&right-left>1e-10;++i){
		if(fc<=fd){
			right=d;
			d=c;
			fd=fc;
			c=right-gr*(right-left);
			fc=fn(c);
		}else{
			left=c;
			c=d;
			fc=fd;
			d=left+gr*(right-left);
			fd=fn(d);
		}
	}

	double t_min=0.5*(left+right);
	GeoEval g_min;
	if(!eval_geo(eph,t_min,g_min)){
		return false;
	}

	*jd_min=t_min;
	*out=g_min;
	return true;
}

Bracket mk_bracket(double t1,double f1,double t2,double f2){
	if(t1<=t2){
		return {t1,t2,f1,f2};
	}
	return {t2,t1,f2,f1};
}

bool bracket_side(const std::function<double(double)>&fn,double center,
				  double f_center,int direction,double start_step,
				  double max_span,Bracket&out){
	if((direction!=1&&direction!=-1)||!(start_step>0.0)||
	   !(max_span>=start_step)){
		return false;
	}
	double prev_t=center;
	double prev_f=f_center;
	double step=start_step;

	while(step<=max_span+1e-14){
		double t=center+static_cast<double>(direction)*step;
		double f=fn(t);
		if(!std::isfinite(f)){
			return false;
		}
		if(prev_f==0.0){
			out=mk_bracket(prev_t,prev_f,prev_t,prev_f);
			return true;
		}
		if(f==0.0||prev_f*f<0.0){
			out=mk_bracket(prev_t,prev_f,t,f);
			return true;
		}
		prev_t=t;
		prev_f=f;
		step*=1.6;
	}
	return false;
}

double solve_bracketed(const std::function<double(double)>&fn,
					   const std::function<double(double)>&dfn,
					   const Bracket&br,double x0,double x_tol){
	double left=br.left;
	double right=br.right;
	double f_left=br.f_left;
	double f_right=br.f_right;
	if(left>right){
		std::swap(left,right);
		std::swap(f_left,f_right);
	}
	if(!std::isfinite(left)||!std::isfinite(right)||!std::isfinite(f_left)||
	   !std::isfinite(f_right)){
		throw std::runtime_error("invalid bracket");
	}
	if(left==right){
		return left;
	}
	if(f_left==0.0){
		return left;
	}
	if(f_right==0.0){
		return right;
	}
	if(f_left*f_right>0.0){
		throw std::runtime_error("bracket does not straddle root");
	}

	double x=std::clamp(x0,left,right);
	if(!(x>left&&x<right)){
		x=0.5*(left+right);
	}

	constexpr double f_tol=1e-14;
	for(int i=0;i<96;++i){
		double fx=fn(x);
		if(!std::isfinite(fx)){
			throw std::runtime_error("function evaluation failed");
		}
		if(std::fabs(fx)<=f_tol){
			return x;
		}

		if(f_left*fx<=0.0){
			right=x;
			f_right=fx;
		}else{
			left=x;
			f_left=fx;
		}
		if((right-left)<=x_tol){
			return 0.5*(left+right);
		}

		double xn=0.5*(left+right);
		double f_span=f_right-f_left;
		if(std::isfinite(f_span)&&std::fabs(f_span)>0.0){
			double cand=left-f_left*(right-left)/f_span;
			if(cand>left&&cand<right){
				xn=cand;
			}
		}
		if(dfn){
			double dfx=dfn(x);
			if(std::isfinite(dfx)&&std::fabs(dfx)>1e-14){
				double cand=x-fx/dfx;
				if(cand>left&&cand<right){
					xn=cand;
				}
			}
		}
		x=xn;
	}
	return 0.5*(left+right);
}

double solve_bracketed(const std::function<double(double)>&fn,
					   const Bracket&br,double x0,double x_tol){
	return solve_bracketed(fn,std::function<double(double)>(),br,x0,x_tol);
}

bool solve_contact_pair(const std::function<double(double)>&fn,double center,
						double f_center,double&t1,double&t2){
	if(!(f_center<=0.0)){
		return false;
	}
	constexpr double step0=2.0/1440.0;
	constexpr double max_span=1.5;

	Bracket b_left;
	Bracket b_right;
	if(!bracket_side(fn,center,f_center,-1,step0,max_span,b_left)){
		return false;
	}
	if(!bracket_side(fn,center,f_center,1,step0,max_span,b_right)){
		return false;
	}

	double guess1=std::clamp(center-0.05,b_left.left,b_left.right);
	double guess2=std::clamp(center+0.05,b_right.left,b_right.right);
	try{
		t1=solve_bracketed(fn,b_left,guess1,1e-10);
		t2=solve_bracketed(fn,b_right,guess2,1e-10);
	}catch(const std::exception&){
		return false;
	}
	return std::isfinite(t1)&&std::isfinite(t2)&&t1<t2;
}

bool stage_norm(const std::string&in,std::string&out){
	std::string v=to_low(in);
	if(v=="any"||v=="all"||v=="partial"){
		out="any";
		return true;
	}
	if(v=="central"||v=="inner"||v=="umb"||v=="umbral"||v=="total"){
		out="central";
		return true;
	}
	return false;
}

std::string ecl_code(const std::string&type){
	if(type=="T"){
		return "total";
	}
	if(type=="A"){
		return "annular";
	}
	if(type=="H"){
		return "hybrid";
	}
	return "partial";
}

std::string ecl_name(const std::string&type){
	std::string fallback;
	if(type=="T"){
		fallback="日全食";
	}else if(type=="A"){
		fallback="日环食";
	}else if(type=="H"){
		fallback="全环食";
	}else{
		fallback="日偏食";
	}
	return lunar::i18n::tr_event_name("solar_eclipse",ecl_code(type),fallback);
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

	double N=a_m/std::sqrt(1.0-e2*s_lat*s_lat);
	double x=(N+h_m)*c_lat*c_lon;
	double y=(N+h_m)*c_lat*s_lon;
	double z=(N*(1.0-e2)+h_m)*s_lat;

	double au_m=AU_KM*1000.0;
	return Vec3(x/au_m,y/au_m,z/au_m);
}

Vec3 up_ecef(double lat_deg,double lon_deg){
	double lat=lat_deg*kRadPerDeg;
	double lon=lon_deg*kRadPerDeg;
	double c_lat=std::cos(lat);
	return Vec3(c_lat*std::cos(lon),c_lat*std::sin(lon),std::sin(lat));
}

Vec3 observer_beta_ecef(const Vec3&obs_ecef){
	Vec3 vel_au_per_day(-kEarthRotationRateRadPerDay*obs_ecef.y,
						kEarthRotationRateRadPerDay*obs_ecef.x,0.0);
	return vel_au_per_day/C_AUDAY;
}

Vec3 apply_diurnal_aberration(const Vec3&dir,const Vec3&obs_beta){
	double beta2=Vec3::dot(obs_beta,obs_beta);
	if(!(beta2>0.0)){
		return dir;
	}

	double gamma_inv=std::sqrt(std::max(0.0,1.0-beta2));
	double nb=Vec3::dot(dir,obs_beta);
	double denom=1.0+nb;
	if(!(denom>0.0)){
		return dir;
	}

	Vec3 dir_ab=(gamma_inv*dir)+obs_beta+((nb/(1.0+gamma_inv))*obs_beta);
	dir_ab=dir_ab/denom;
	double norm=dir_ab.norm();
	if(!(norm>0.0)){
		return dir;
	}
	return dir_ab/norm;
}

bool eval_body_ecef(EphRead&eph,double jd_tdb,double jd_utc,BodyEcefState&out){
	Vec3 sun_geo=raw_vec(AberCorr::geo_app(eph,eph.SUN,jd_tdb,3));
	Vec3 moon_geo=raw_vec(AberCorr::geo_app(eph,eph.MOON,jd_tdb,3));
	if(!finite_vec(sun_geo)||!finite_vec(moon_geo)){
		return false;
	}

	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 sun_eq=eq*sun_geo;
	Vec3 moon_eq=eq*moon_geo;

	double uta=std::floor(jd_utc);
	double utb=jd_utc-uta;
	double tta=std::floor(jd_tdb);
	double ttb=jd_tdb-tta;
	double gast=lunar::precnut::gst06a(uta,utb,tta,ttb);
	Mat3 R=CoordTf::R3(gast);
	out.sun_ecef=R*sun_eq;
	out.moon_ecef=R*moon_eq;
	return finite_vec(out.sun_ecef)&&finite_vec(out.moon_ecef);
}

bool eval_topo_from_body(const BodyEcefState&body,const PointObserver&obs,
						 TopoEval&out){
	Vec3 sun_topo=body.sun_ecef-obs.ecef;
	Vec3 moon_topo=body.moon_ecef-obs.ecef;
	double sun_n=sun_topo.norm();
	double moon_n=moon_topo.norm();
	if(!(sun_n>0.0)||!(moon_n>0.0)){
		return false;
	}

	Vec3 us=sun_topo/sun_n;
	Vec3 um=moon_topo/moon_n;
	us=apply_diurnal_aberration(us,obs.beta_ecef);
	um=apply_diurnal_aberration(um,obs.beta_ecef);

	double sep=std::acos(clamp_unit(Vec3::dot(us,um)));
	double sd_sun=std::asin(clamp_unit(kRsKm/(sun_n*AU_KM)));
	double sd_moon=std::asin(clamp_unit(kRmKm/(moon_n*AU_KM)));
	double outer=sep-(sd_sun+sd_moon);
	double inner=sep-std::fabs(sd_moon-sd_sun);
	double overlap=sd_sun+sd_moon-sep;

	double mag=std::numeric_limits<double>::quiet_NaN();
	if(sd_sun>0.0){
		mag=overlap/(2.0*sd_sun);
		if(mag<0.0){
			mag=0.0;
		}
	}
	double obsc=0.0;
	if(overlap>0.0){
		double area=circle_overlap_area(sd_sun,sd_moon,sep);
		if(std::isfinite(area)&&sd_sun>0.0){
			obsc=area/(PI*sd_sun*sd_sun);
			if(obsc<0.0){
				obsc=0.0;
			}
			if(obsc>1.0){
				obsc=1.0;
			}
		}
	}

	double sun_alt_deg=std::asin(clamp_unit(Vec3::dot(us,obs.up_ecef)))*kDegPerRad;
	out.sep=sep;
	out.sd_sun=sd_sun;
	out.sd_moon=sd_moon;
	out.outer=outer;
	out.inner=inner;
	out.mag=mag;
	out.obscuration=obsc;
	out.sun_alt_deg=sun_alt_deg;
	return std::isfinite(out.sep)&&std::isfinite(out.sd_sun)&&
		   std::isfinite(out.sd_moon)&&std::isfinite(out.outer)&&
		   std::isfinite(out.inner)&&std::isfinite(out.mag)&&
		   std::isfinite(out.obscuration)&&std::isfinite(out.sun_alt_deg);
}

bool eval_topo_at(EphRead&eph,double jd_tdb,const PointObserver&obs,TopoEval&out){
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	BodyEcefState body;
	if(!eval_body_ecef(eph,jd_tdb,jd_utc,body)){
		return false;
	}
	return eval_topo_from_body(body,obs,out);
}

bool find_topo_minimum(EphRead&eph,const PointObserver&obs,double t1,double t2,
					   double*jd_min,TopoEval*out){
	if(jd_min==nullptr||out==nullptr||!std::isfinite(t1)||!std::isfinite(t2)||
	   !(t2>=t1)){
		return false;
	}
	double span=t2-t1;
	double step=span/120.0;
	if(step<2.0/1440.0){
		step=2.0/1440.0;
	}
	if(step>20.0/1440.0){
		step=20.0/1440.0;
	}

	double best_t=std::numeric_limits<double>::quiet_NaN();
	double best_sep=std::numeric_limits<double>::infinity();
	for(double t=t1;t<=t2+1e-12;t+=step){
		TopoEval st;
		if(!eval_topo_at(eph,t,obs,st)){
			continue;
		}
		if(st.sep<best_sep){
			best_sep=st.sep;
			best_t=t;
		}
	}
	if(!std::isfinite(best_t)){
		return false;
	}

	double left=std::max(t1,best_t-step);
	double right=std::min(t2,best_t+step);
	if(!(right>left)){
		left=std::max(t1,best_t-2.0*step);
		right=std::min(t2,best_t+2.0*step);
		if(!(right>left)){
			left=t1;
			right=t2;
		}
	}

	auto fn=[&](double jd) -> double{
		TopoEval st;
		if(!eval_topo_at(eph,jd,obs,st)){
			return std::numeric_limits<double>::infinity();
		}
		return st.sep;
	};

	const double gr=(std::sqrt(5.0)-1.0)*0.5;
	double c=right-gr*(right-left);
	double d=left+gr*(right-left);
	double fc=fn(c);
	double fd=fn(d);
	for(int i=0;i<80&&right-left>1e-10;++i){
		if(fc<=fd){
			right=d;
			d=c;
			fd=fc;
			c=right-gr*(right-left);
			fc=fn(c);
		}else{
			left=c;
			c=d;
			fc=fd;
			d=left+gr*(right-left);
			fd=fn(d);
		}
	}

	double t_min=0.5*(left+right);
	TopoEval st_min;
	if(!eval_topo_at(eph,t_min,obs,st_min)){
		return false;
	}
	*jd_min=t_min;
	*out=st_min;
	return true;
}

std::vector<double> sample_grid(double t1,double t2,double sample_minutes){
	if(!(t2>=t1)){
		return {};
	}
	double step=sample_minutes/1440.0;
	if(!(step>0.0)){
		step=2.0/1440.0;
	}
	int n=static_cast<int>(std::ceil((t2-t1)/step))+1;
	if(n<2){
		n=2;
	}
	if(n>4000){
		n=4000;
	}
	std::vector<double> out;
	out.reserve(static_cast<std::size_t>(n));
	for(int i=0;i<n;++i){
		double u=(n==1)?0.0:static_cast<double>(i)/static_cast<double>(n-1);
		out.push_back(t1+(t2-t1)*u);
	}
	return out;
}

double stage_active_value(EphRead&eph,const PointObserver&obs,double jd_utc,
						  bool central_stage){
	double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	TopoEval st;
	if(!eval_topo_at(eph,jd_tdb,obs,st)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double contact=central_stage?st.inner:st.outer;
	double alt_lim=-st.sun_alt_deg*kRadPerDeg;
	return std::max(contact,alt_lim);
}

double refine_stage_edge(EphRead&eph,const PointObserver&obs,double left,double right,
						 bool central_stage){
	double f_left=stage_active_value(eph,obs,left,central_stage);
	double f_right=stage_active_value(eph,obs,right,central_stage);
	if(!std::isfinite(f_left)||!std::isfinite(f_right)){
		return 0.5*(left+right);
	}
	if(f_left==0.0){
		return left;
	}
	if(f_right==0.0){
		return right;
	}
	if(f_left*f_right>0.0){
		return 0.5*(left+right);
	}

	double a=left;
	double b=right;
	double fa=f_left;
	for(int i=0;i<36;++i){
		double m=0.5*(a+b);
		double fm=stage_active_value(eph,obs,m,central_stage);
		if(!std::isfinite(fm)){
			return m;
		}
		if(std::fabs(fm)<=1e-8){
			return m;
		}
		if(fa*fm<=0.0){
			b=m;
		}else{
			a=m;
			fa=fm;
		}
	}
	return 0.5*(a+b);
}

std::vector<double> build_lat_grid(double step_deg){
	if(!(step_deg>0.0)){
		step_deg=10.0;
	}
	std::vector<double> out;
	for(double lat=-90.0;lat<90.0-1e-12;lat+=step_deg){
		out.push_back(lat);
	}
	if(out.empty()||std::fabs(out.back()-90.0)>1e-9){
		out.push_back(90.0);
	}
	return out;
}

std::vector<double> build_lon_grid(double step_deg){
	if(!(step_deg>0.0)){
		step_deg=10.0;
	}
	std::vector<double> out;
	for(double lon=-180.0;lon<180.0-1e-12;lon+=step_deg){
		out.push_back(lon);
	}
	if(out.empty()){
		out.push_back(0.0);
	}
	return out;
}

}

bool calc_solar_eclipse(EphRead&eph,double jd_tdb_near_new_moon,SolarEclipse*out){
	if(out==nullptr){
		throw std::invalid_argument("out must not be null");
	}
	if(!std::isfinite(jd_tdb_near_new_moon)){
		throw std::invalid_argument("jd_tdb_near_new_moon must be finite");
	}
	*out=SolarEclipse{};

	double jd_max=0.0;
	double f_any_min=std::numeric_limits<double>::quiet_NaN();
	GeoEval g_max;
	if(!find_global_metric_minimum(eph,jd_tdb_near_new_moon,false,&jd_max,
								   &f_any_min,&g_max)){
		return false;
	}
	if(!(f_any_min<=0.0)){
		return false;
	}

	SolarEclipse ans;
	ans.has=true;
	ans.jd_tdb_max=jd_max;
	ans.mag=g_max.mag;
	ans.obscuration=g_max.obscuration;
	ans.gamma=g_max.gamma;
	ans.sep_max_deg=g_max.sep*kDegPerRad;
	ans.sun_sd_max_deg=g_max.sd_sun*kDegPerRad;
	ans.moon_sd_max_deg=g_max.sd_moon*kDegPerRad;
	ans.sun_dist_km=g_max.sun_dist_km;
	ans.moon_dist_km=g_max.moon_dist_km;
	ans.rp_re=g_max.rp/kReA;
	ans.ru_re=g_max.ru/kReA;
	ans.dt_max_sec=(ans.jd_tdb_max-TimeScale::tdb_to_utc(ans.jd_tdb_max))*SEC_DAY;

	double jd_cen_min=std::numeric_limits<double>::quiet_NaN();
	double f_cen_min=std::numeric_limits<double>::infinity();
	GeoEval g_cen;
	bool has_cen_min=
		find_global_metric_minimum(eph,jd_tdb_near_new_moon,true,&jd_cen_min,
								   &f_cen_min,&g_cen);
	bool central=has_cen_min&&(f_cen_min<=0.0);
	if(central){
		const GeoEval&g_ref=std::isfinite(jd_cen_min)?g_cen:g_max;
		if(g_ref.ru>=0.0){
			ans.type="T";
		}else{
			double ru_sub=
				g_ref.ru+kReEqA*(kRsA-kRmA)/g_ref.D;
			if(ru_sub>=0.0){
				ans.type="H";
			}else{
				ans.type="A";
			}
		}
	}else{
		ans.type="P";
	}

	auto outer_fn=[&](double jd) -> double{
		double f_any=std::numeric_limits<double>::quiet_NaN();
		double f_cen=std::numeric_limits<double>::quiet_NaN();
		if(!eval_global_metrics(eph,jd,f_any,f_cen,nullptr)){
			return std::numeric_limits<double>::quiet_NaN();
		}
		return f_any;
	};
	if(!solve_contact_pair(outer_fn,jd_max,f_any_min,ans.jd_tdb_c1,ans.jd_tdb_c4)){
		return false;
	}

	if(central){
		auto inner_fn=[&](double jd) -> double{
			double f_any=std::numeric_limits<double>::quiet_NaN();
			double f_cen=std::numeric_limits<double>::quiet_NaN();
			if(!eval_global_metrics(eph,jd,f_any,f_cen,nullptr)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			return f_cen;
		};
		double c2=std::numeric_limits<double>::quiet_NaN();
		double c3=std::numeric_limits<double>::quiet_NaN();
		double root_center=std::isfinite(jd_cen_min)?jd_cen_min:jd_max;
		double root_value=std::isfinite(f_cen_min)?f_cen_min:0.0;
		if(solve_contact_pair(inner_fn,root_center,root_value,c2,c3)){
			ans.jd_tdb_c2=c2;
			ans.jd_tdb_c3=c3;
		}else{
			return false;
		}
	}

	if(!(ans.jd_tdb_c1<ans.jd_tdb_max&&ans.jd_tdb_max<ans.jd_tdb_c4)){
		return false;
	}
	if(central&&std::isfinite(ans.jd_tdb_c2)&&std::isfinite(ans.jd_tdb_c3)){
		if(!(ans.jd_tdb_c1<ans.jd_tdb_c2&&ans.jd_tdb_c2<ans.jd_tdb_max&&
			 ans.jd_tdb_max<ans.jd_tdb_c3&&ans.jd_tdb_c3<ans.jd_tdb_c4)){
			return false;
		}
	}

	*out=ans;
	return true;
}

std::vector<EventRec> bld_solar_eclipse_events(EphRead&eph,const YearResult&yr,
												int tz_off){
	std::vector<EventRec> out;
	out.reserve(yr.lun_phase.size());
	for(const auto&item : yr.lun_phase){
		double jd_utc=item.new_moon.toUtcJD();
		double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
		SolarEclipse ecl;
		if(!calc_solar_eclipse(eph,jd_tdb,&ecl)||!ecl.has){
			continue;
		}
		EventRec ev;
		ev.kind="solar_eclipse";
		ev.code=ecl_code(ecl.type);
		ev.name=ecl_name(ecl.type);
		ev.year=yr.year;
		ev.jd_tdb=ecl.jd_tdb_max;
		ev.jd_utc=TimeScale::tdb_to_utc(ecl.jd_tdb_max);
		ev.utc_iso=fmt_iso(ev.jd_utc,0,true);
		ev.loc_iso=fmt_iso(ev.jd_utc,tz_off,true);
		out.push_back(std::move(ev));
	}
	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	out.erase(std::unique(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return std::fabs(a.jd_utc-b.jd_utc)<1e-9;
	}),out.end());
	return out;
}

bool solar_eclipse_window_tdb(const SolarEclipse&ecl,const std::string&stage_window,
							  double*jd_tdb_start,double*jd_tdb_end){
	if(jd_tdb_start==nullptr||jd_tdb_end==nullptr||!ecl.has){
		return false;
	}
	std::string stage;
	if(!stage_norm(stage_window,stage)){
		return false;
	}

	double t1=std::numeric_limits<double>::quiet_NaN();
	double t2=std::numeric_limits<double>::quiet_NaN();
	if(stage=="any"){
		t1=ecl.jd_tdb_c1;
		t2=ecl.jd_tdb_c4;
	}else{
		t1=ecl.jd_tdb_c2;
		t2=ecl.jd_tdb_c3;
	}
	if(!std::isfinite(t1)||!std::isfinite(t2)||!(t2>=t1)){
		return false;
	}
	constexpr double pad=10.0/1440.0;
	*jd_tdb_start=t1-pad;
	*jd_tdb_end=t2+pad;
	return true;
}

bool solar_eclipse_point_visibility(EphRead&eph,const SolarEclipse&ecl,
									const std::string&stage_window,
									double lat_deg,double lon_deg,double height_m,
									double sample_minutes,bool refine_edge,
									SolarEclipsePointVis*out){
	if(out==nullptr){
		throw std::invalid_argument("out must not be null");
	}
	if(!std::isfinite(lat_deg)||!std::isfinite(lon_deg)||!std::isfinite(height_m)){
		throw std::invalid_argument("point visibility inputs must be finite");
	}
	if(lat_deg<-90.0||lat_deg>90.0){
		throw std::invalid_argument("lat_deg must be in [-90,90]");
	}
	if(lon_deg<-180.0||lon_deg>180.0){
		throw std::invalid_argument("lon_deg must be in [-180,180]");
	}

	std::string stage;
	if(!stage_norm(stage_window,stage)){
		throw std::invalid_argument("stage_window must be any|central");
	}

	double t1_tdb=0.0;
	double t2_tdb=0.0;
	if(!solar_eclipse_window_tdb(ecl,stage,&t1_tdb,&t2_tdb)){
		return false;
	}

	PointObserver obs;
	obs.ecef=geodetic_to_ecef(lat_deg,lon_deg,height_m);
	obs.up_ecef=up_ecef(lat_deg,lon_deg);
	obs.beta_ecef=observer_beta_ecef(obs.ecef);

	*out=SolarEclipsePointVis{};
	out->stage_window=stage;
	out->lat_deg=lat_deg;
	out->lon_deg=lon_deg;
	out->height_m=height_m;

	double jd_topo_max=0.0;
	TopoEval topo_max;
	if(!find_topo_minimum(eph,obs,t1_tdb,t2_tdb,&jd_topo_max,&topo_max)){
		return false;
	}
	out->has_eclipse=topo_max.outer<=0.0;
	out->central=topo_max.inner<=0.0;
	out->max_jd_utc=TimeScale::tdb_to_utc(jd_topo_max);
	out->max_mag=topo_max.mag;
	out->max_obscuration=topo_max.obscuration;

	if(out->has_eclipse){
		auto outer_fn=[&](double jd) -> double{
			TopoEval st;
			if(!eval_topo_at(eph,jd,obs,st)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			return st.outer;
		};
		double c1=std::numeric_limits<double>::quiet_NaN();
		double c4=std::numeric_limits<double>::quiet_NaN();
		if(solve_contact_pair(outer_fn,jd_topo_max,topo_max.outer,c1,c4)){
			out->c1_jd_utc=TimeScale::tdb_to_utc(c1);
			out->c4_jd_utc=TimeScale::tdb_to_utc(c4);
		}

		if(out->central){
			auto inner_fn=[&](double jd) -> double{
				TopoEval st;
				if(!eval_topo_at(eph,jd,obs,st)){
					return std::numeric_limits<double>::quiet_NaN();
				}
				return st.inner;
			};
			double c2=std::numeric_limits<double>::quiet_NaN();
			double c3=std::numeric_limits<double>::quiet_NaN();
			if(solve_contact_pair(inner_fn,jd_topo_max,topo_max.inner,c2,c3)){
				out->c2_jd_utc=TimeScale::tdb_to_utc(c2);
				out->c3_jd_utc=TimeScale::tdb_to_utc(c3);
			}
		}
	}

	double t1_utc=TimeScale::tdb_to_utc(t1_tdb);
	double t2_utc=TimeScale::tdb_to_utc(t2_tdb);
	std::vector<double> times=sample_grid(t1_utc,t2_utc,sample_minutes);
	if(times.empty()){
		return false;
	}
	out->sample_count=static_cast<int>(times.size());

	int first_idx=-1;
	int last_idx=-1;
	double max_alt=-90.0;
	bool central_stage=(stage=="central");
	for(std::size_t i=0;i<times.size();++i){
		double jd_tdb=TimeScale::utc_to_tdb(times[i]);
		TopoEval st;
		if(!eval_topo_at(eph,jd_tdb,obs,st)){
			continue;
		}
		if(std::isfinite(st.sun_alt_deg)&&st.sun_alt_deg>max_alt){
			max_alt=st.sun_alt_deg;
		}
		double active_value=std::max(central_stage?st.inner:st.outer,
								 -st.sun_alt_deg*kRadPerDeg);
		if(active_value<=0.0){
			if(first_idx<0){
				first_idx=static_cast<int>(i);
			}
			last_idx=static_cast<int>(i);
		}
	}
	out->max_sun_alt_deg=max_alt;
	out->visible=(first_idx>=0);
	if(out->visible){
		out->first_jd_utc=times[static_cast<std::size_t>(first_idx)];
		out->last_jd_utc=times[static_cast<std::size_t>(last_idx)];
		if(refine_edge){
			if(first_idx>0){
				double a=times[static_cast<std::size_t>(first_idx-1)];
				double b=times[static_cast<std::size_t>(first_idx)];
				out->first_jd_utc=refine_stage_edge(eph,obs,a,b,central_stage);
			}
			if(last_idx+1<static_cast<int>(times.size())){
				double a=times[static_cast<std::size_t>(last_idx)];
				double b=times[static_cast<std::size_t>(last_idx+1)];
				out->last_jd_utc=refine_stage_edge(eph,obs,a,b,central_stage);
			}
		}
	}
	return true;
}

bool solar_eclipse_global_visibility(EphRead&eph,const SolarEclipse&ecl,
									 const std::string&stage_window,
									 double lat_step_deg,double lon_step_deg,
									 double sample_minutes,
									 SolarEclipseGlobalVis*out){
	if(out==nullptr){
		throw std::invalid_argument("out must not be null");
	}
	if(!std::isfinite(lat_step_deg)||!std::isfinite(lon_step_deg)||
	   !std::isfinite(sample_minutes)){
		throw std::invalid_argument("global visibility inputs must be finite");
	}
	if(!(lat_step_deg>0.0)||!(lon_step_deg>0.0)){
		throw std::invalid_argument("grid steps must be >0");
	}

	std::string stage;
	if(!stage_norm(stage_window,stage)){
		throw std::invalid_argument("stage_window must be any|central");
	}

	double t1_tdb=0.0;
	double t2_tdb=0.0;
	if(!solar_eclipse_window_tdb(ecl,stage,&t1_tdb,&t2_tdb)){
		return false;
	}
	double t1_utc=TimeScale::tdb_to_utc(t1_tdb);
	double t2_utc=TimeScale::tdb_to_utc(t2_tdb);
	std::vector<double> times=sample_grid(t1_utc,t2_utc,sample_minutes);
	if(times.empty()){
		return false;
	}

	std::vector<BodyEcefState> body_series;
	body_series.reserve(times.size());
	for(double jd_utc : times){
		double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
		BodyEcefState body;
		if(!eval_body_ecef(eph,jd_tdb,jd_utc,body)){
			return false;
		}
		body_series.push_back(body);
	}

	std::vector<double> lat_grid=build_lat_grid(lat_step_deg);
	std::vector<double> lon_grid=build_lon_grid(lon_step_deg);
	bool central_stage=(stage=="central");

	*out=SolarEclipseGlobalVis{};
	out->stage_window=stage;
	out->jd_start_utc=t1_utc;
	out->jd_end_utc=t2_utc;
	out->lat_step_deg=lat_step_deg;
	out->lon_step_deg=lon_step_deg;
	out->sample_count=static_cast<int>(times.size());

	for(double lat : lat_grid){
		for(double lon : lon_grid){
			PointObserver obs;
			obs.ecef=geodetic_to_ecef(lat,lon,0.0);
			obs.up_ecef=up_ecef(lat,lon);
			obs.beta_ecef=observer_beta_ecef(obs.ecef);

			bool vis=false;
			int first_idx=-1;
			int last_idx=-1;
			double max_mag=0.0;
			double max_alt=-90.0;

			for(std::size_t i=0;i<body_series.size();++i){
				TopoEval st;
				if(!eval_topo_from_body(body_series[i],obs,st)){
					continue;
				}
				double active_value=std::max(central_stage?st.inner:st.outer,
										 -st.sun_alt_deg*kRadPerDeg);
				if(active_value<=0.0){
					if(!vis){
						first_idx=static_cast<int>(i);
					}
					vis=true;
					last_idx=static_cast<int>(i);
					if(std::isfinite(st.mag)&&st.mag>max_mag){
						max_mag=st.mag;
					}
					if(std::isfinite(st.sun_alt_deg)&&st.sun_alt_deg>max_alt){
						max_alt=st.sun_alt_deg;
					}
				}
			}

			if(vis){
				SolarEclipseGlobalPoint pt;
				pt.lat_deg=lat;
				pt.lon_deg=lon;
				pt.max_mag=max_mag;
				pt.max_sun_alt_deg=max_alt;
				pt.first_jd_utc=times[static_cast<std::size_t>(first_idx)];
				pt.last_jd_utc=times[static_cast<std::size_t>(last_idx)];
				out->points.push_back(pt);
			}
		}
	}
	return true;
}
