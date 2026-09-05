#include "lunar/lunar_eclipse.hpp"
#include "lunar/app_long.hpp"
#include "lunar/format.hpp"
#include "lunar/frames.hpp"
#include "lunar/i18n.hpp"
#include "lunar/precnut_core.hpp"
#include "lunar/time_scale.hpp"

#include<algorithm>
#include<array>
#include<cctype>
#include<cmath>
#include<functional>
#include<limits>
#include<stdexcept>
#include<vector>

namespace{

constexpr double kReKm=6378.1366;
constexpr double kRsKm=695700.0;
constexpr double kRmKm=1737.4;
constexpr double kDanjonKm=75.0;
constexpr double kWgs84InvF=298.257223563;
constexpr double kWgs84F=1.0/kWgs84InvF;
constexpr double kRpKm=kReKm*(1.0-kWgs84F);
constexpr double kEarthRotationRateRadPerDay=1.00273781191135448*TWO_PI;
constexpr double kDegPerRad=180.0/PI;
constexpr double kRadPerDeg=PI/180.0;

constexpr double kRe0A=kReKm/AU_KM;
constexpr double kRp0A=kRpKm/AU_KM;
constexpr double kDanjonA=kDanjonKm/AU_KM;
constexpr double kReLegacyA=(kReKm+kDanjonKm)/AU_KM;
constexpr double kRsA=kRsKm/AU_KM;
constexpr double kRmA=kRmKm/AU_KM;

struct EqSph{
	double ra=std::numeric_limits<double>::quiet_NaN();
	double dec=std::numeric_limits<double>::quiet_NaN();
};

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

double norm_deg360(double angle_deg){
	double v=std::fmod(angle_deg,360.0);
	if(v<0.0){
		v+=360.0;
	}
	return v;
}

std::string to_low(std::string s){
	for(char&c : s){
		c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return s;
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

EqSph vec_to_eqsph(const Vec3&v){
	EqSph out;
	double r=v.norm();
	if(!(r>0.0)){
		return out;
	}
	out.ra=norm2pi(std::atan2(v.y,v.x));
	out.dec=std::asin(clamp_unit(v.z/r));
	return out;
}

Mat3 eq_true_mat(double jd_tdb){
	Mat3 P=PrecNut::prec_mat(jd_tdb);
	Mat3 N=PrecNut::nut_mat(jd_tdb);
	return N*P;
}

bool fill_geo_coord(const Vec3&geo_eq,double radius_km,EclipseGeoCoord*out){
	if(out==nullptr){
		return false;
	}
	double dist_km=geo_eq.norm()*AU_KM;
	EqSph sph=vec_to_eqsph(geo_eq);
	if(!(dist_km>0.0)||!std::isfinite(sph.ra)||!std::isfinite(sph.dec)){
		return false;
	}

	double sd=std::asin(clamp_unit(radius_km/dist_km));
	double ehp=std::asin(clamp_unit(kReKm/dist_km));
	out->ra_deg=sph.ra*kDegPerRad;
	out->dec_deg=sph.dec*kDegPerRad;
	out->sd_deg=sd*kDegPerRad;
	out->ehp_deg=ehp*kDegPerRad;
	return std::isfinite(out->ra_deg)&&std::isfinite(out->dec_deg)&&
		   std::isfinite(out->sd_deg)&&std::isfinite(out->ehp_deg);
}

struct MoonOrient{
	double ra_rad=std::numeric_limits<double>::quiet_NaN();
	double dec_rad=std::numeric_limits<double>::quiet_NaN();
	double w_rad=std::numeric_limits<double>::quiet_NaN();
};

Vec3 unit_from_ra_dec(double ra_rad,double dec_rad){
	double c=std::cos(dec_rad);
	return Vec3(c*std::cos(ra_rad),c*std::sin(ra_rad),std::sin(dec_rad));
}

MoonOrient moon_orient_iau(double jd_tdb){
	double d=jd_tdb-2451545.0;
	double T=d/36525.0;
	auto d2r=[](double v) -> double{ return v*kRadPerDeg; };
	auto arg=[&](double c0,double c1) -> double{ return d2r(c0+c1*d); };

	double E1=arg(125.045,-0.0529921);
	double E2=arg(250.089,-0.1059842);
	double E3=arg(260.008,13.0120009);
	double E4=arg(176.625,13.3407154);
	double E5=arg(357.529,0.9856003);
	double E6=arg(311.589,26.4057084);
	double E7=arg(134.963,13.0649930);
	double E8=arg(276.617,0.3287146);
	double E9=arg(34.226,1.7484877);
	double E10=arg(15.134,-0.1589763);
	double E11=arg(119.743,0.0036096);
	double E12=arg(239.961,0.1643573);
	double E13=arg(25.053,12.9590088);

	double ra_deg=269.9949+0.0031*T-3.8787*std::sin(E1)-0.1204*std::sin(E2)+
				  0.0700*std::sin(E3)-0.0172*std::sin(E4)+0.0072*std::sin(E6)-
				  0.0052*std::sin(E10)+0.0043*std::sin(E13);

	double dec_deg=66.5392+0.0130*T+1.5419*std::cos(E1)+0.0239*std::cos(E2)-
				   0.0278*std::cos(E3)+0.0068*std::cos(E4)-0.0029*std::cos(E6)+
				   0.0009*std::cos(E7)+0.0008*std::cos(E10)-0.0009*std::cos(E13);

	double w_deg=38.3213+13.17635815*d-1.4e-12*d*d+3.5610*std::sin(E1)+
				 0.1208*std::sin(E2)-0.0642*std::sin(E3)+0.0158*std::sin(E4)+
				 0.0252*std::sin(E5)-0.0066*std::sin(E6)-0.0047*std::sin(E7)-
				 0.0046*std::sin(E8)+0.0028*std::sin(E9)+0.0052*std::sin(E10)+
				 0.0040*std::sin(E11)+0.0019*std::sin(E12)-0.0044*std::sin(E13);

	MoonOrient out;
	out.ra_rad=d2r(norm_deg360(ra_deg));
	out.dec_rad=d2r(dec_deg);
	out.w_rad=d2r(norm_deg360(w_deg));
	return out;
}

Mat3 m_inertial_to_moon_fixed(double ra_rad,double dec_rad,double w_rad){
	double sa=std::sin(ra_rad);
	double ca=std::cos(ra_rad);
	double sd=std::sin(dec_rad);
	double cd=std::cos(dec_rad);
	double sw=std::sin(w_rad);
	double cw=std::cos(w_rad);

	Mat3 m;
	m.m[0][0]=-sa*cw-ca*sd*sw;
	m.m[0][1]=ca*cw-sa*sd*sw;
	m.m[0][2]=cd*sw;

	m.m[1][0]=sa*sw-ca*sd*cw;
	m.m[1][1]=-ca*sw-sa*sd*cw;
	m.m[1][2]=cd*cw;

	m.m[2][0]=ca*cd;
	m.m[2][1]=sa*cd;
	m.m[2][2]=sd;
	return m;
}

bool fill_libration(const Vec3&moon_geo,const Vec3&moon_eq,double jd_tdb,
					EclipseLibration*out){
	if(out==nullptr){
		return false;
	}
	double geo_n=moon_geo.norm();
	double eq_n=moon_eq.norm();
	if(!(geo_n>0.0)||!(eq_n>0.0)){
		return false;
	}

	MoonOrient o=moon_orient_iau(jd_tdb);
	if(!std::isfinite(o.ra_rad)||!std::isfinite(o.dec_rad)||!std::isfinite(o.w_rad)){
		return false;
	}
	Mat3 to_fix=m_inertial_to_moon_fixed(o.ra_rad,o.dec_rad,o.w_rad);
	Vec3 to_earth=(-1.0/geo_n)*moon_geo;
	Vec3 eb=to_fix*to_earth;

	double l=std::atan2(eb.y,eb.x)*kDegPerRad;
	double b=std::asin(clamp_unit(eb.z))*kDegPerRad;

	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 pole_eq=eq*unit_from_ra_dec(o.ra_rad,o.dec_rad);
	EqSph moon_sph=vec_to_eqsph(moon_eq);
	EqSph pole_sph=vec_to_eqsph(pole_eq);
	if(!std::isfinite(moon_sph.ra)||!std::isfinite(moon_sph.dec)||
	   !std::isfinite(pole_sph.ra)||!std::isfinite(pole_sph.dec)){
		return false;
	}

	double da=norm_pm_pi(pole_sph.ra-moon_sph.ra);
	double y=std::cos(pole_sph.dec)*std::sin(da);
	double x=std::sin(pole_sph.dec)*std::cos(moon_sph.dec)-
			 std::cos(pole_sph.dec)*std::sin(moon_sph.dec)*std::cos(da);
	double c=std::atan2(y,x)*kDegPerRad;

	out->l_deg=l;
	out->b_deg=b;
	out->c_deg=c;
	return std::isfinite(l)&&std::isfinite(b)&&std::isfinite(c);
}

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
	double re1=0.0;
	double re2=0.0;
	double rp1=0.0;
	double rp2=0.0;
	double ru1=0.0;
	double ru2=0.0;
	double rp=0.0;
	double ru=0.0;
	double rp_dot=0.0;
	double ru_dot=0.0;
	Vec3 e1_eq;
	Vec3 e2_eq;
	double p1=0.0;
	double p2=0.0;
};

struct ShadowBodyState{
	Vec3 s;
	Vec3 s_dot;
	Vec3 m;
	Vec3 m_dot;
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

bool finite_vec(const Vec3&v){
	return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}

int&eclipse_method_storage(){
	static int method=static_cast<int>(LunarEclipseCalcMethod::Modern);
	return method;
}

bool use_legacy_calc_method(){
	return eclipse_method_storage()==
		   static_cast<int>(LunarEclipseCalcMethod::Legacy);
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

bool earth_projected_axes(const Vec3&axis_eq,double&re1,double&re2){
	double an=axis_eq.norm();
	if(!(an>0.0)){
		return false;
	}
	Vec3 u=axis_eq/an;
	double mu=std::fabs(clamp_unit(u.z));
	double re1_core=std::sqrt(kRe0A*kRe0A*mu*mu+kRp0A*kRp0A*(1.0-mu*mu));
	re1=re1_core+kDanjonA;
	re2=kRe0A+kDanjonA;
	return std::isfinite(re1)&&std::isfinite(re2);
}

bool eval_shadow_state(EphRead&eph,double jd_tdb,ShadowBodyState&st){
	constexpr int max_iter=3;
	// The following true-equator/GCRS geometry expects a geocentric apparent
	// state, including the reception-epoch frame transformation.
	AberState sun=AberCorr::geo_app_state(eph,eph.SUN,jd_tdb,max_iter);
	auto moon=eph.get_state(eph.MOON,eph.EARTH,jd_tdb);
	st.s=raw_vec(sun.X);
	st.s_dot=raw_vec(sun.V);
	st.m=raw_vec(moon.first);
	st.m_dot=raw_vec(moon.second);
	return finite_vec(st.s)&&finite_vec(st.s_dot)&&finite_vec(st.m)&&
		   finite_vec(st.m_dot);
}

bool cone_slope(double c,double D,double D_dot,double&slope,double&slope_dot){
	if(!(D>0.0)){
		return false;
	}
	double z=c/D;
	double one_minus=1.0-z*z;
	if(!(one_minus>0.0)){
		return false;
	}
	double root=std::sqrt(one_minus);
	double z_dot=-(c*D_dot)/(D*D);
	slope=z/root;
	slope_dot=z_dot/(one_minus*root);
	return std::isfinite(slope)&&std::isfinite(slope_dot);
}

double ellipse_radial_radius(double x,double y,double a,double b){
	if(!(a>0.0)||!(b>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	double r=std::hypot(x,y);
	if(!(r>0.0)){
		return std::min(a,b);
	}
	double ux=x/r;
	double uy=y/r;
	double den=(ux*ux)/(a*a)+(uy*uy)/(b*b);
	if(!(den>0.0)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	return 1.0/std::sqrt(den);
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

	double lambda=0.5*(lo+hi);
	for(int i=0;i<72;++i){
		double mid=0.5*(lo+hi);
		double f_mid=ellipse_constraint(x,y,a2,b2,mid);
		if(!std::isfinite(f_mid)){
			return std::numeric_limits<double>::quiet_NaN();
		}
		lambda=mid;
		if(std::fabs(f_mid)<=1e-16){
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

bool eval_shadow(EphRead&eph,double jd_tdb,ShadowGeom&g){
	ShadowBodyState st;
	if(!eval_shadow_state(eph,jd_tdb,st)){
		return false;
	}
	g.s=st.s;
	g.s_dot=st.s_dot;
	g.m=st.m;
	g.m_dot=st.m_dot;
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

	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 axis_eq=eq*g.axis;
	Vec3 dvec_eq=eq*g.dvec;
	if(use_legacy_calc_method()){
		double tan_pen=0.0;
		double tan_pen_dot=0.0;
		double tan_umb=0.0;
		double tan_umb_dot=0.0;
		if(!cone_slope(kRsA+kReLegacyA,g.D,g.D_dot,tan_pen,tan_pen_dot)){
			return false;
		}
		if(!cone_slope(kRsA-kReLegacyA,g.D,g.D_dot,tan_umb,tan_umb_dot)){
			return false;
		}
		g.re1=kReLegacyA;
		g.re2=kReLegacyA;
		g.rp=kReLegacyA+g.x*tan_pen;
		g.ru=kReLegacyA-g.x*tan_umb;
		g.rp_dot=g.x_dot*tan_pen+g.x*tan_pen_dot;
		g.ru_dot=-(g.x_dot*tan_umb+g.x*tan_umb_dot);
		g.rp1=g.rp;
		g.rp2=g.rp;
		g.ru1=g.ru;
		g.ru2=g.ru;
		g.p1=g.d;
		g.p2=0.0;
		return std::isfinite(g.x)&&std::isfinite(g.x_dot)&&std::isfinite(g.d)&&
			   std::isfinite(g.d_dot)&&std::isfinite(g.rp)&&std::isfinite(g.ru)&&
			   std::isfinite(g.rp_dot)&&std::isfinite(g.ru_dot);
	}

	if(!build_shadow_plane_basis(axis_eq,g.e1_eq,g.e2_eq)){
		return false;
	}
	g.p1=Vec3::dot(dvec_eq,g.e1_eq);
	g.p2=Vec3::dot(dvec_eq,g.e2_eq);
	if(!earth_projected_axes(axis_eq,g.re1,g.re2)){
		return false;
	}

	double tan_pen1=0.0;
	double tan_pen1_dot=0.0;
	double tan_pen2=0.0;
	double tan_pen2_dot=0.0;
	double tan_umb1=0.0;
	double tan_umb1_dot=0.0;
	double tan_umb2=0.0;
	double tan_umb2_dot=0.0;
	if(!cone_slope(kRsA+g.re1,g.D,g.D_dot,tan_pen1,tan_pen1_dot)){
		return false;
	}
	if(!cone_slope(kRsA+g.re2,g.D,g.D_dot,tan_pen2,tan_pen2_dot)){
		return false;
	}
	if(!cone_slope(kRsA-g.re1,g.D,g.D_dot,tan_umb1,tan_umb1_dot)){
		return false;
	}
	if(!cone_slope(kRsA-g.re2,g.D,g.D_dot,tan_umb2,tan_umb2_dot)){
		return false;
	}
	g.rp1=g.re1+g.x*tan_pen1;
	g.rp2=g.re2+g.x*tan_pen2;
	g.ru1=g.re1-g.x*tan_umb1;
	g.ru2=g.re2-g.x*tan_umb2;
	g.rp=ellipse_radial_radius(g.p1,g.p2,g.rp1,g.rp2);
	g.ru=ellipse_radial_radius(g.p1,g.p2,g.ru1,g.ru2);

	return std::isfinite(g.x)&&std::isfinite(g.x_dot)&&std::isfinite(g.d)&&
		   std::isfinite(g.d_dot)&&std::isfinite(g.re1)&&std::isfinite(g.re2)&&
		   std::isfinite(g.rp1)&&std::isfinite(g.rp2)&&std::isfinite(g.ru1)&&
		   std::isfinite(g.ru2)&&std::isfinite(g.rp)&&std::isfinite(g.ru)&&
		   std::isfinite(g.p1)&&std::isfinite(g.p2);
}

bool fill_point_meta(EphRead&eph,double jd_tdb,bool inner_touch,
					 EclipsePointMeta*out){
	if(out==nullptr||!std::isfinite(jd_tdb)){
		return false;
	}
	ShadowGeom g;
	if(!eval_shadow(eph,jd_tdb,g)){
		return false;
	}

	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 moon_eq=eq*g.m;
	Vec3 axis_eq=eq*g.axis;
	double rn=moon_eq.norm();
	double an=axis_eq.norm();
	if(!(rn>0.0)||!(an>0.0)){
		return false;
	}
	Vec3 moon_u=moon_eq/rn;
	Vec3 axis_u=axis_eq/an;
	EqSph m=vec_to_eqsph(moon_u);
	if(!std::isfinite(m.ra)||!std::isfinite(m.dec)){
		return false;
	}

	double jd_td=TimeScale::tdb_to_tt(jd_tdb);
	double jd_ut1=TimeScale::tdb_to_ut1(jd_tdb);
	double uta=std::floor(jd_ut1);
	double utb=jd_ut1-uta;
	double tta=std::floor(jd_td);
	double ttb=jd_td-tta;
	double gast=lunar::precnut::gst06a(uta,utb,tta,ttb);
	double zen_lon=norm_pm_pi(m.ra-gast)*kDegPerRad;
	double zen_lat=m.dec*kDegPerRad;

	double cs=clamp_unit(Vec3::dot(moon_u,axis_u));
	double axis_deg=std::acos(cs)*kDegPerRad;

	Vec3 east(-std::sin(m.ra),std::cos(m.ra),0.0);
	Vec3 north(-std::cos(m.ra)*std::sin(m.dec),-std::sin(m.ra)*std::sin(m.dec),
			   std::cos(m.dec));

	Vec3 t=axis_u-Vec3::dot(axis_u,moon_u)*moon_u;
	double tn=t.norm();
	double pa=std::numeric_limits<double>::quiet_NaN();
	if(tn>0.0){
		t=t/tn;
		if(inner_touch){
			t=(-1.0)*t;
		}
		double e=Vec3::dot(t,east);
		double n=Vec3::dot(t,north);
		pa=norm_deg360(std::atan2(e,n)*kDegPerRad);
	}

	out->zen_lat_deg=zen_lat;
	out->zen_lon_deg=zen_lon;
	out->pa_deg=pa;
	out->axis_deg=axis_deg;
	return std::isfinite(zen_lat)&&std::isfinite(zen_lon)&&
		   std::isfinite(axis_deg);
}

double contact_radius_legacy(const ShadowGeom&g,ContactMode mode){
	if(mode==ContactMode::PenOuter){
		return g.rp+kRmA;
	}
	if(mode==ContactMode::UmbOuter){
		return g.ru+kRmA;
	}
	return g.ru-kRmA;
}

double contact_radius_eff(const ShadowGeom&g,ContactMode mode){
	if(mode==ContactMode::PenOuter){
		return std::max(g.rp1,g.rp2)+kRmA;
	}
	if(mode==ContactMode::UmbOuter){
		return std::max(g.ru1,g.ru2)+kRmA;
	}
	return std::min(g.ru1,g.ru2)-kRmA;
}

double contact_value(const ShadowGeom&g,ContactMode mode){
	if(use_legacy_calc_method()){
		return g.d-contact_radius_legacy(g,mode);
	}

	double a=0.0;
	double b=0.0;
	if(mode==ContactMode::PenOuter){
		a=g.rp1;
		b=g.rp2;
	}else{
		a=g.ru1;
		b=g.ru2;
	}
	double sd=ellipse_signed_distance(g.p1,g.p2,a,b);
	if(!std::isfinite(sd)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	if(mode==ContactMode::UmbInner){
		return sd+kRmA;
	}
	return sd-kRmA;
}

double contact_derivative_legacy(const ShadowGeom&g,ContactMode mode){
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
	bool legacy_mode=use_legacy_calc_method();
	double radius=legacy_mode ? contact_radius_legacy(g_max,mode)
							  : contact_radius_eff(g_max,mode);
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
	std::function<double(double)> dfn;
	if(legacy_mode){
		dfn=[&](double jd) -> double{
			ShadowGeom g;
			if(!eval_shadow(eph,jd,g)){
				return std::numeric_limits<double>::quiet_NaN();
			}
			return contact_derivative_legacy(g,mode);
		};
	}

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
	try{
		if(dfn){
			t1=solve_bracketed(fn,dfn,b_left,guess1,1e-10);
			t2=solve_bracketed(fn,dfn,b_right,guess2,1e-10);
		}else{
			t1=solve_bracketed(fn,b_left,guess1,1e-10);
			t2=solve_bracketed(fn,b_right,guess2,1e-10);
		}
	}catch(const std::exception&){
		return false;
	}
	return std::isfinite(t1)&&std::isfinite(t2)&&t1<t2;
}

double opp_value(EphRead&eph,double jd_tdb){
	ShadowBodyState st;
	if(!eval_shadow_state(eph,jd_tdb,st)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 sun_eq=eq*st.s;
	Vec3 moon_eq=eq*st.m;
	EqSph s=vec_to_eqsph(sun_eq);
	EqSph m=vec_to_eqsph(moon_eq);
	if(!std::isfinite(s.ra)||!std::isfinite(m.ra)){
		return std::numeric_limits<double>::quiet_NaN();
	}
	return norm_pm_pi(m.ra-s.ra-PI);
}

bool solve_opp(EphRead&eph,double jd_near,double*jd_opp){
	if(jd_opp==nullptr||!std::isfinite(jd_near)){
		return false;
	}
	auto fn=[&](double jd) -> double{ return opp_value(eph,jd); };

	Bracket br;
	if(!bracket_near(fn,jd_near,2.0,1.0/24.0,br)){
		if(!bracket_near(fn,jd_near,3.0,1.0/48.0,br)){
			return false;
		}
	}

	double guess=std::clamp(jd_near,br.left,br.right);
	double root=0.0;
	try{
		root=solve_bracketed(fn,br,guess,1e-10);
	}catch(const std::exception&){
		return false;
	}
	if(!std::isfinite(root)){
		return false;
	}
	*jd_opp=root;
	return true;
}

double signed_gamma_re(const ShadowGeom&g,double jd_tdb){
	double gamma_abs=g.d/kRe0A;
	if(!(gamma_abs>0.0)){
		return 0.0;
	}

	Mat3 eq=eq_true_mat(jd_tdb);
	Vec3 axis_eq=eq*g.axis;
	Vec3 dvec_eq=eq*g.dvec;
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

}

void set_lunar_eclipse_calc_method(LunarEclipseCalcMethod method){
	switch(method){
		case LunarEclipseCalcMethod::Modern:
		case LunarEclipseCalcMethod::Legacy:
			eclipse_method_storage()=static_cast<int>(method);
			return;
	}
	throw std::invalid_argument("invalid lunar eclipse calculation method");
}

LunarEclipseCalcMethod get_lunar_eclipse_calc_method(){
	if(eclipse_method_storage()==
	   static_cast<int>(LunarEclipseCalcMethod::Legacy)){
		return LunarEclipseCalcMethod::Legacy;
	}
	return LunarEclipseCalcMethod::Modern;
}

bool parse_lunar_eclipse_calc_method(const std::string&value,
									 LunarEclipseCalcMethod*out){
	if(out==nullptr){
		return false;
	}
	std::string v=to_low(value);
	if(v=="modern"||v=="current"||v=="new"){
		*out=LunarEclipseCalcMethod::Modern;
		return true;
	}
	if(v=="legacy"||v=="old"){
		*out=LunarEclipseCalcMethod::Legacy;
		return true;
	}
	return false;
}

const char*lunar_eclipse_calc_method_name(LunarEclipseCalcMethod method){
	if(method==LunarEclipseCalcMethod::Legacy){
		return "legacy";
	}
	return "modern";
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

	Bracket g_br;
	if(!bracket_near(fn_g,jd_tdb_near_full_moon,2.0,1.0/24.0,g_br)){
		if(!bracket_near(fn_g,jd_tdb_near_full_moon,3.0,1.0/48.0,g_br)){
			return false;
		}
	}

	double guess=std::clamp(jd_tdb_near_full_moon,g_br.left,g_br.right);
	double jd_max=0.0;
	try{
		jd_max=solve_bracketed(fn_g,g_br,guess,1e-10);
	}catch(const std::exception&){
		return false;
	}
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

	ans.rp_re=g_max.rp/kRe0A;
	ans.ru_re=g_max.ru/kRe0A;
	ans.gamma=signed_gamma_re(g_max,ans.jd_tdb_max);
	ans.eps_deg=std::atan2(g_max.d,g_max.x)*kDegPerRad;
	ans.moon_dist_km=g_max.m.norm()*AU_KM;

	if(std::isfinite(ans.jd_tdb_p1)&&std::isfinite(ans.jd_tdb_p4)){
		ans.dur_pen_sec=(ans.jd_tdb_p4-ans.jd_tdb_p1)*SEC_DAY;
	}
	if(std::isfinite(ans.jd_tdb_u1)&&std::isfinite(ans.jd_tdb_u4)){
		ans.dur_umb_sec=(ans.jd_tdb_u4-ans.jd_tdb_u1)*SEC_DAY;
	}
	if(std::isfinite(ans.jd_tdb_u2)&&std::isfinite(ans.jd_tdb_u3)){
		ans.dur_tot_sec=(ans.jd_tdb_u3-ans.jd_tdb_u2)*SEC_DAY;
	}

	ans.dt_max_sec=TimeScale::delta_t_seconds(TimeScale::tdb_to_tt(ans.jd_tdb_max));

	double jd_opp=jd_tdb_near_full_moon;
	double jd_opp_solved=std::numeric_limits<double>::quiet_NaN();
	if(solve_opp(eph,jd_tdb_near_full_moon,&jd_opp_solved)){
		jd_opp=jd_opp_solved;
	}
	ans.jd_tdb_opp=jd_opp;
	ShadowGeom g_opp;
	if(eval_shadow(eph,jd_opp,g_opp)){
		ans.opp_rp_re=g_opp.rp/kRe0A;
		ans.opp_ru_re=g_opp.ru/kRe0A;
	}

	Mat3 eq=eq_true_mat(ans.jd_tdb_max);
	Vec3 sun_eq=eq*g_max.s;
	Vec3 moon_eq=eq*g_max.m;
	fill_geo_coord(sun_eq,kRsKm,&ans.sun_geo);
	fill_geo_coord(moon_eq,kRmKm,&ans.moon_geo);
	fill_libration(g_max.m,moon_eq,ans.jd_tdb_max,&ans.lib);

	fill_point_meta(eph,ans.jd_tdb_p1,false,&ans.p1_meta);
	fill_point_meta(eph,ans.jd_tdb_u1,false,&ans.u1_meta);
	fill_point_meta(eph,ans.jd_tdb_u2,true,&ans.u2_meta);
	fill_point_meta(eph,ans.jd_tdb_max,false,&ans.max_meta);
	fill_point_meta(eph,ans.jd_tdb_u3,true,&ans.u3_meta);
	fill_point_meta(eph,ans.jd_tdb_u4,false,&ans.u4_meta);
	fill_point_meta(eph,ans.jd_tdb_p4,false,&ans.p4_meta);
	fill_point_meta(eph,ans.jd_tdb_opp,false,&ans.opp_meta);

	*out=ans;
	return true;
}

namespace{

bool stage_norm(const std::string&in,std::string&out){
	std::string v=to_low(in);
	if(v=="any"||v=="all"||v=="pen"||v=="penumbral"){
		out="any";
		return true;
	}
	if(v=="umb"||v=="umbral"){
		out="umb";
		return true;
	}
	if(v=="total"||v=="tot"){
		out="total";
		return true;
	}
	return false;
}

std::string ecl_code(const std::string&type){
	if(type=="T"){
		return "total";
	}
	if(type=="U"){
		return "partial";
	}
	return "penumbral";
}

std::string ecl_name(const std::string&type){
	std::string fallback;
	if(type=="T"){
		fallback="月全食";
	}else if(type=="U"){
		fallback="月偏食";
	}else{
		fallback="半影月食";
	}
	return lunar::i18n::tr_event_name("lunar_eclipse",ecl_code(type),fallback);
}

Vec3 geodetic_to_ecef(double lat_deg,double lon_deg,double h_m){
	constexpr double a_m=6378137.0;
	constexpr double inv_f=298.257223563;
	const double f=1.0/inv_f;
	const double e2=f*(2.0-f);

	double lat=lat_deg*PI/180.0;
	double lon=lon_deg*PI/180.0;
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
	double lat=lat_deg*PI/180.0;
	double lon=lon_deg*PI/180.0;
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

Vec3 moon_ecef(EphRead&eph,double jd_utc){
	double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	Vec3 moon_geo=raw_vec(AberCorr::geo_app(eph,eph.MOON,jd_tdb,3));

	Mat3 P=PrecNut::prec_mat(jd_tdb);
	Mat3 N=PrecNut::nut_mat(jd_tdb);
	Mat3 eq_true=N*P;
	Vec3 moon_eq=eq_true*moon_geo;

	return PrecNut::earth_rot(jd_tdb)*moon_eq;
}

double topocentric_alt_deg(const Vec3&moon_ecef,const Vec3&obs_ecef,
						   const Vec3&up_dir,const Vec3&obs_beta){
	Vec3 topo=moon_ecef-obs_ecef;
	double rn=topo.norm();
	if(!(rn>0.0)){
		return -90.0;
	}
	Vec3 sight=topo/rn;
	sight=apply_diurnal_aberration(sight,obs_beta);
	double s=Vec3::dot(sight,up_dir);
	return std::asin(clamp_unit(s))*180.0/PI;
}

double point_alt_deg(EphRead&eph,double jd_utc,const Vec3&obs_ecef,
					 const Vec3&up_dir,const Vec3&obs_beta){
	Vec3 moon=moon_ecef(eph,jd_utc);
	return topocentric_alt_deg(moon,obs_ecef,up_dir,obs_beta);
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

double refine_cross(EphRead&eph,double left,double right,const Vec3&obs_ecef,
					const Vec3&up_dir,const Vec3&obs_beta){
	double f_left=point_alt_deg(eph,left,obs_ecef,up_dir,obs_beta);
	double f_right=point_alt_deg(eph,right,obs_ecef,up_dir,obs_beta);
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
	for(int i=0;i<30;++i){
		double m=0.5*(a+b);
		double fm=point_alt_deg(eph,m,obs_ecef,up_dir,obs_beta);
		if(!std::isfinite(fm)){
			return m;
		}
		if(std::fabs(fm)<=1e-6){
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

std::vector<EventRec> bld_lunar_eclipse_events(EphRead&eph,const YearResult&yr,
												int tz_off){
	std::vector<EventRec> out;
	out.reserve(yr.lun_phase.size());
	for(const auto&item : yr.lun_phase){
		double jd_utc=item.full_moon.toUtcJD();
		double jd_tdb=TimeScale::utc_to_tdb(jd_utc);
		LunarEclipse ecl;
		if(!calc_lunar_eclipse(eph,jd_tdb,&ecl)||!ecl.has){
			continue;
		}
		EventRec ev;
		ev.kind="lunar_eclipse";
		ev.code=ecl_code(ecl.type);
		ev.name=ecl_name(ecl.type);
		ev.jd_tdb=ecl.jd_tdb_max;
		ev.jd_utc=TimeScale::tdb_to_utc(ecl.jd_tdb_max);
		ev.utc_iso=fmt_iso(ev.jd_utc,0,true);
		ev.loc_iso=fmt_iso(ev.jd_utc,tz_off,true);
		int event_year=0;
		int month=0;
		int day=0;
		int hour=0;
		int minute=0;
		double second=0.0;
		jd2greg(ev.jd_utc+static_cast<double>(tz_off)/1440.0,event_year,month,day,
				 hour,minute,second);
		ev.year=event_year;
		out.push_back(std::move(ev));
	}
	std::sort(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
		return a.jd_utc<b.jd_utc;
	});
	out.erase(
		std::unique(out.begin(),out.end(),[](const EventRec&a,const EventRec&b){
			return std::fabs(a.jd_utc-b.jd_utc)<1e-9;
		}),
		out.end());
	return out;
}

bool lunar_eclipse_window_tdb(const LunarEclipse&ecl,const std::string&stage_window,
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
		t1=ecl.jd_tdb_p1;
		t2=ecl.jd_tdb_p4;
	}else if(stage=="umb"){
		t1=ecl.jd_tdb_u1;
		t2=ecl.jd_tdb_u4;
	}else{
		t1=ecl.jd_tdb_u2;
		t2=ecl.jd_tdb_u3;
	}
	if(!std::isfinite(t1)||!std::isfinite(t2)||!(t2>=t1)){
		return false;
	}
	*jd_tdb_start=t1;
	*jd_tdb_end=t2;
	return true;
}

bool lunar_eclipse_point_visibility(EphRead&eph,const LunarEclipse&ecl,
									const std::string&stage_window,
									double lat_deg,double lon_deg,double height_m,
									double sample_minutes,bool refine_edge,
									LunarEclipsePointVis*out){
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
		throw std::invalid_argument("stage_window must be any|umb|total");
	}

	double t1_tdb=0.0;
	double t2_tdb=0.0;
	if(!lunar_eclipse_window_tdb(ecl,stage,&t1_tdb,&t2_tdb)){
		return false;
	}
	double t1_utc=TimeScale::tdb_to_utc(t1_tdb);
	double t2_utc=TimeScale::tdb_to_utc(t2_tdb);
	std::vector<double> times=sample_grid(t1_utc,t2_utc,sample_minutes);
	if(times.empty()){
		return false;
	}

	Vec3 obs=geodetic_to_ecef(lat_deg,lon_deg,height_m);
	Vec3 up=up_ecef(lat_deg,lon_deg);
	Vec3 obs_beta=observer_beta_ecef(obs);

	double max_alt=-90.0;
	int first_idx=-1;
	int last_idx=-1;
	for(std::size_t i=0;i<times.size();++i){
		double alt=point_alt_deg(eph,times[i],obs,up,obs_beta);
		if(std::isfinite(alt)&&alt>max_alt){
			max_alt=alt;
		}
		if(alt>0.0){
			if(first_idx<0){
				first_idx=static_cast<int>(i);
			}
			last_idx=static_cast<int>(i);
		}
	}

	*out=LunarEclipsePointVis{};
	out->stage_window=stage;
	out->lat_deg=lat_deg;
	out->lon_deg=lon_deg;
	out->height_m=height_m;
	out->sample_count=static_cast<int>(times.size());
	out->max_alt_deg=max_alt;
	out->visible=(first_idx>=0);
	if(out->visible){
		out->first_jd_utc=times[static_cast<std::size_t>(first_idx)];
		out->last_jd_utc=times[static_cast<std::size_t>(last_idx)];
		if(refine_edge){
			if(first_idx>0){
				double a=times[static_cast<std::size_t>(first_idx-1)];
				double b=times[static_cast<std::size_t>(first_idx)];
				out->first_jd_utc=refine_cross(eph,a,b,obs,up,obs_beta);
			}
			if(last_idx+1<static_cast<int>(times.size())){
				double a=times[static_cast<std::size_t>(last_idx)];
				double b=times[static_cast<std::size_t>(last_idx+1)];
				out->last_jd_utc=refine_cross(eph,a,b,obs,up,obs_beta);
			}
		}
	}
	return true;
}

bool lunar_eclipse_global_visibility(EphRead&eph,const LunarEclipse&ecl,
									 const std::string&stage_window,
									 double lat_step_deg,double lon_step_deg,
									 double sample_minutes,
									 LunarEclipseGlobalVis*out){
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
		throw std::invalid_argument("stage_window must be any|umb|total");
	}

	double t1_tdb=0.0;
	double t2_tdb=0.0;
	if(!lunar_eclipse_window_tdb(ecl,stage,&t1_tdb,&t2_tdb)){
		return false;
	}
	double t1_utc=TimeScale::tdb_to_utc(t1_tdb);
	double t2_utc=TimeScale::tdb_to_utc(t2_tdb);
	std::vector<double> times=sample_grid(t1_utc,t2_utc,sample_minutes);
	if(times.empty()){
		return false;
	}
	std::vector<Vec3> moon_series;
	moon_series.reserve(times.size());
	for(double jd_utc : times){
		moon_series.push_back(moon_ecef(eph,jd_utc));
	}

	std::vector<double> lat_grid=build_lat_grid(lat_step_deg);
	std::vector<double> lon_grid=build_lon_grid(lon_step_deg);

	*out=LunarEclipseGlobalVis{};
	out->stage_window=stage;
	out->jd_start_utc=t1_utc;
	out->jd_end_utc=t2_utc;
	out->lat_step_deg=lat_step_deg;
	out->lon_step_deg=lon_step_deg;
	out->sample_count=static_cast<int>(times.size());

	for(double lat : lat_grid){
		for(double lon : lon_grid){
			Vec3 obs=geodetic_to_ecef(lat,lon,0.0);
			Vec3 up=up_ecef(lat,lon);
			Vec3 obs_beta=observer_beta_ecef(obs);
			bool vis=false;
			int first_idx=-1;
			int last_idx=-1;
			double max_alt=-90.0;
			for(std::size_t i=0;i<moon_series.size();++i){
				double alt=topocentric_alt_deg(moon_series[i],obs,up,obs_beta);
				if(std::isfinite(alt)&&alt>max_alt){
					max_alt=alt;
				}
				if(alt>0.0){
					if(!vis){
						first_idx=static_cast<int>(i);
					}
					vis=true;
					last_idx=static_cast<int>(i);
				}
			}
			if(vis){
				LunarEclipseGlobalPoint pt;
				pt.lat_deg=lat;
				pt.lon_deg=lon;
				pt.max_alt_deg=max_alt;
				pt.first_jd_utc=times[static_cast<std::size_t>(first_idx)];
				pt.last_jd_utc=times[static_cast<std::size_t>(last_idx)];
				out->points.push_back(pt);
			}
		}
	}
	return true;
}
