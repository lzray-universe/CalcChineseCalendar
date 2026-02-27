#include "lunar/lunar_eclipse.hpp"
#include "lunar/app_long.hpp"

#include<algorithm>
#include<cmath>
#include<functional>
#include<limits>
#include<stdexcept>

namespace{

constexpr double kReKm=6378.1366;
constexpr double kRsKm=695700.0;
constexpr double kRmKm=1737.4;
constexpr double kDanjonKm=75.0;

constexpr double kReA=(kReKm+kDanjonKm)/AU_KM;
constexpr double kRsA=kRsKm/AU_KM;
constexpr double kRmA=kRmKm/AU_KM;

enum class ContactMode{
	PenOuter,
	UmbOuter,
	UmbInner,
};

struct ShadowGeom{
	Vec3 s;
	Vec3 s_dot;
	Vec3 m;
	Vec3 m_dot;
	Vec3 axis;
	Vec3 axis_dot;
	Vec3 dvec;
	Vec3 dvec_dot;
	double D=0.0;
	double D_dot=0.0;
	double x=0.0;
	double x_dot=0.0;
	double d2=0.0;
	double d=0.0;
	double d_dot=0.0;
	double rp=0.0;
	double rp_dot=0.0;
	double ru=0.0;
	double ru_dot=0.0;
};

struct Bracket{
	double left=0.0;
	double right=0.0;
	double f_left=0.0;
	double f_right=0.0;
};

Bracket mk_bracket(double t1,double f1,double t2,double f2){
	if(t1<=t2){
		return {t1,t2,f1,f2};
	}
	return {t2,t1,f2,f1};
}

bool eval_shadow(EphRead&eph,double jd_tdb,ShadowGeom&g){
	constexpr double h=2e-6;
	constexpr int max_iter=3;
	g.s=AberCorr::geo_app(eph,eph.SUN,jd_tdb,max_iter);
	g.m=AberCorr::geo_app(eph,eph.MOON,jd_tdb,max_iter);
	Vec3 s_plus=AberCorr::geo_app(eph,eph.SUN,jd_tdb+h,max_iter);
	Vec3 s_minus=AberCorr::geo_app(eph,eph.SUN,jd_tdb-h,max_iter);
	Vec3 m_plus=AberCorr::geo_app(eph,eph.MOON,jd_tdb+h,max_iter);
	Vec3 m_minus=AberCorr::geo_app(eph,eph.MOON,jd_tdb-h,max_iter);
	g.s_dot=(s_plus-s_minus)/(2.0*h);
	g.m_dot=(m_plus-m_minus)/(2.0*h);
	g.D=g.s.norm();
	if(!(g.D>0.0)){
		return false;
	}

	g.axis=(-1.0/g.D)*g.s;
	double ad=Vec3::dot(g.axis,g.s_dot);
	g.axis_dot=(-1.0/g.D)*g.s_dot+(ad/g.D)*g.axis;

	g.D_dot=Vec3::dot(g.s,g.s_dot)/g.D;

	g.x=Vec3::dot(g.m,g.axis);
	g.x_dot=Vec3::dot(g.m_dot,g.axis)+Vec3::dot(g.m,g.axis_dot);

	g.dvec=g.m-g.x*g.axis;
	g.dvec_dot=g.m_dot-g.x_dot*g.axis-g.x*g.axis_dot;

	g.d2=Vec3::dot(g.dvec,g.dvec);
	if(g.d2<0.0&&g.d2>-1e-20){
		g.d2=0.0;
	}
	if(g.d2<0.0){
		return false;
	}
	g.d=std::sqrt(g.d2);
	if(g.d>0.0){
		g.d_dot=Vec3::dot(g.dvec,g.dvec_dot)/g.d;
	}else{
		g.d_dot=0.0;
	}

	double x_div_D=g.x/g.D;
	double x_div_D_dot=(g.x_dot*g.D-g.x*g.D_dot)/(g.D*g.D);
	g.rp=kReA+(kRsA+kReA)*x_div_D;
	g.ru=kReA-(kRsA-kReA)*x_div_D;
	g.rp_dot=(kRsA+kReA)*x_div_D_dot;
	g.ru_dot=-(kRsA-kReA)*x_div_D_dot;

	return std::isfinite(g.x)&&std::isfinite(g.x_dot)&&std::isfinite(g.d)&&
		   std::isfinite(g.d_dot)&&std::isfinite(g.rp)&&std::isfinite(g.ru)&&
		   std::isfinite(g.rp_dot)&&std::isfinite(g.ru_dot);
}

double contact_radius(const ShadowGeom&g,ContactMode mode){
	if(mode==ContactMode::PenOuter){
		return g.rp+kRmA;
	}
	if(mode==ContactMode::UmbOuter){
		return g.ru+kRmA;
	}
	return g.ru-kRmA;
}

double contact_value(const ShadowGeom&g,ContactMode mode){
	return g.d-contact_radius(g,mode);
}

double contact_derivative(const ShadowGeom&g,ContactMode mode){
	if(mode==ContactMode::PenOuter){
		return g.d_dot-g.rp_dot;
	}
	return g.d_dot-g.ru_dot;
}

double g_value(EphRead&eph,double jd_tdb){
	ShadowGeom g;
	if(!eval_shadow(eph,jd_tdb,g)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	return 2.0*Vec3::dot(g.dvec,g.dvec_dot);
}

double g_derivative(EphRead&eph,double jd_tdb){
	constexpr double h=1e-5;
	double f1=g_value(eph,jd_tdb+h);
	double f2=g_value(eph,jd_tdb-h);
	if(!std::isfinite(f1)||!std::isfinite(f2)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	return (f1-f2)/(2.0*h);
}

bool bracket_near(const std::function<double(double)>&fn,double center,
				  double span,double step,Bracket&out){
	if(!(span>0.0)||!(step>0.0)){
		return false;
	}
	int n=static_cast<int>(std::ceil((2.0*span)/step));
	double start=center-span;
	double prev_t=start;
	double prev_f=fn(prev_t);
	if(!std::isfinite(prev_f)){
		return false;
	}

	bool found=false;
	double best_dist=std::numeric_limits<double>::infinity();
	Bracket best{};

	for(int i=1;i<=n;++i){
		double t=start+step*static_cast<double>(i);
		double f=fn(t);
		if(!std::isfinite(f)){
			return false;
		}

		if(prev_f==0.0){
			double dist=std::fabs(prev_t-center);
			if(dist<best_dist){
				best=mk_bracket(prev_t,prev_f,prev_t,prev_f);
				best_dist=dist;
				found=true;
			}
		}
		if(f==0.0){
			double dist=std::fabs(t-center);
			if(dist<best_dist){
				best=mk_bracket(t,f,t,f);
				best_dist=dist;
				found=true;
			}
		}
		if(prev_f*f<0.0){
			double mid=0.5*(prev_t+t);
			double dist=std::fabs(mid-center);
			if(dist<best_dist){
				best=mk_bracket(prev_t,prev_f,t,f);
				best_dist=dist;
				found=true;
			}
		}

		prev_t=t;
		prev_f=f;
	}

	if(!found){
		return false;
	}
	out=best;
	return true;
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
		double dfx=dfn(x);
		if(std::isfinite(dfx)&&std::fabs(dfx)>1e-14){
			double cand=x-fx/dfx;
			if(cand>left&&cand<right){
				xn=cand;
			}
		}
		x=xn;
	}
	return 0.5*(left+right);
}

bool solve_contact_pair(EphRead&eph,double jd_max,const ShadowGeom&g_max,
						ContactMode mode,double&t1,double&t2){
	double f_center=contact_value(g_max,mode);
	if(!(f_center<=0.0)){
		return false;
	}

	double v=g_max.dvec_dot.norm();
	if(!(v>0.0)){
		v=1e-6;
	}
	double radius=contact_radius(g_max,mode);
	if(!(radius>0.0)){
		return false;
	}

	double span_est=radius/v;
	if(span_est<2e-4){
		span_est=2e-4;
	}
	if(span_est>0.3){
		span_est=0.3;
	}
	double step0=span_est*0.6;
	if(step0<1e-4){
		step0=1e-4;
	}
	double max_span=std::max(0.25,span_est*12.0);
	if(max_span>2.0){
		max_span=2.0;
	}

	auto fn=[&](double jd) -> double{
		ShadowGeom g;
		if(!eval_shadow(eph,jd,g)){
			return std::numeric_limits<double>::quiet_NaN();
		}
		return contact_value(g,mode);
	};
	auto dfn=[&](double jd) -> double{
		ShadowGeom g;
		if(!eval_shadow(eph,jd,g)){
			return std::numeric_limits<double>::quiet_NaN();
		}
		return contact_derivative(g,mode);
	};

	Bracket b_left;
	Bracket b_right;
	if(!bracket_side(fn,jd_max,f_center,-1,step0,max_span,b_left)){
		return false;
	}
	if(!bracket_side(fn,jd_max,f_center,1,step0,max_span,b_right)){
		return false;
	}

	double guess1=std::clamp(jd_max-span_est,b_left.left,b_left.right);
	double guess2=std::clamp(jd_max+span_est,b_right.left,b_right.right);
	t1=solve_bracketed(fn,dfn,b_left,guess1,1e-10);
	t2=solve_bracketed(fn,dfn,b_right,guess2,1e-10);
	return std::isfinite(t1)&&std::isfinite(t2)&&t1<t2;
}

}

bool calc_lunar_eclipse(EphRead&eph,double jd_tdb_near_full_moon,
						LunarEclipse*out){
	if(out==nullptr){
		throw std::invalid_argument("out must not be null");
	}
	if(!std::isfinite(jd_tdb_near_full_moon)){
		throw std::invalid_argument("jd_tdb_near_full_moon must be finite");
	}

	*out=LunarEclipse{};

	auto fn_g=[&](double jd) -> double{ return g_value(eph,jd); };
	auto dfn_g=[&](double jd) -> double{ return g_derivative(eph,jd); };

	Bracket g_br;
	if(!bracket_near(fn_g,jd_tdb_near_full_moon,2.0,1.0/24.0,g_br)){
		if(!bracket_near(fn_g,jd_tdb_near_full_moon,3.0,1.0/48.0,g_br)){
			return false;
		}
	}

	double guess=std::clamp(jd_tdb_near_full_moon,g_br.left,g_br.right);
	double jd_max=solve_bracketed(fn_g,dfn_g,g_br,guess,1e-10);
	ShadowGeom g_max;
	if(!eval_shadow(eph,jd_max,g_max)){
		return false;
	}
	if(!(g_max.x>0.0)){
		return false;
	}

	double pen_mag=(g_max.rp+kRmA-g_max.d)/(2.0*kRmA);
	double umb_mag=(g_max.ru+kRmA-g_max.d)/(2.0*kRmA);
	if(!std::isfinite(pen_mag)||!std::isfinite(umb_mag)){
		return false;
	}
	if(!(pen_mag>0.0)){
		return false;
	}

	LunarEclipse ans;
	ans.has=true;
	ans.jd_tdb_max=jd_max;
	ans.pen_mag=pen_mag;
	ans.umb_mag=umb_mag;

	if(umb_mag>=1.0){
		ans.type="T";
	}else if(umb_mag>0.0){
		ans.type="U";
	}else{
		ans.type="P";
	}

	if(!solve_contact_pair(eph,jd_max,g_max,ContactMode::PenOuter,ans.jd_tdb_p1,
						   ans.jd_tdb_p4)){
		return false;
	}

	if(umb_mag>0.0){
		if(!solve_contact_pair(eph,jd_max,g_max,ContactMode::UmbOuter,
							   ans.jd_tdb_u1,ans.jd_tdb_u4)){
			return false;
		}
	}

	if(umb_mag>=1.0){
		double u2=std::numeric_limits<double>::quiet_NaN();
		double u3=std::numeric_limits<double>::quiet_NaN();
		if(solve_contact_pair(eph,jd_max,g_max,ContactMode::UmbInner,u2,u3)){
			ans.jd_tdb_u2=u2;
			ans.jd_tdb_u3=u3;
		}
	}

	if(!(ans.jd_tdb_p1<ans.jd_tdb_max&&ans.jd_tdb_max<ans.jd_tdb_p4)){
		return false;
	}
	if(umb_mag>0.0){
		if(!(ans.jd_tdb_p1<ans.jd_tdb_u1&&ans.jd_tdb_u1<ans.jd_tdb_max&&
			 ans.jd_tdb_max<ans.jd_tdb_u4&&ans.jd_tdb_u4<ans.jd_tdb_p4)){
			return false;
		}
	}
	if(umb_mag>=1.0&&std::isfinite(ans.jd_tdb_u2)&&
	   std::isfinite(ans.jd_tdb_u3)){
		if(!(ans.jd_tdb_u1<ans.jd_tdb_u2&&ans.jd_tdb_u2<ans.jd_tdb_max&&
			 ans.jd_tdb_max<ans.jd_tdb_u3&&ans.jd_tdb_u3<ans.jd_tdb_u4)){
			return false;
		}
	}

	*out=ans;
	return true;
}
