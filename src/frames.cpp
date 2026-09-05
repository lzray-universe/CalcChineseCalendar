#include "lunar/frames.hpp"
#include "lunar/precnut_core.hpp"
#include "lunar/time_scale.hpp"

#include<algorithm>
#include<cmath>

const PrecModel PREC_MODEL=PrecModel::AUTO;
const double LONG_THR=2000.0;

namespace{

constexpr double kLongBlendStartYears=1000.0;

double long_term_weight(double jd_tdb){
	if(PREC_MODEL==PrecModel::IAU2006){
		return 0.0;
	}
	if(PREC_MODEL==PrecModel::VONDRAK){
		return 1.0;
	}
	double epj=2000.0+(jd_tdb-2451545.0)/365.25;
	double years=std::fabs(epj-2000.0);
	if(years<=kLongBlendStartYears){
		return 0.0;
	}
	if(years>=LONG_THR){
		return 1.0;
	}
	double u=(years-kLongBlendStartYears)/(LONG_THR-kLongBlendStartYears);
	return u*u*(3.0-2.0*u);
}

double long_term_obliquity(double jd_tdb){
	double epj=2000.0+(jd_tdb-2451545.0)/365.25;
	double equator[3];
	double ecliptic[3];
	lunar::precnut::ltpequ(epj,equator);
	lunar::precnut::ltpecl(epj,ecliptic);
	double c=equator[0]*ecliptic[0]+equator[1]*ecliptic[1]+
			 equator[2]*ecliptic[2];
	c=std::max(-1.0,std::min(1.0,c));
	return std::acos(c);
}

Mat3 transpose(const Mat3&m){
	Mat3 out;
	for(int i=0;i<3;++i){
		for(int j=0;j<3;++j){
			out.m[i][j]=m.m[j][i];
		}
	}
	return out;
}

Vec3 cross(const Vec3&a,const Vec3&b){
	return Vec3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);
}

Vec3 unit(const Vec3&v){
	double n=v.norm();
	return n>0.0?v/n:Vec3(1.0,0.0,0.0);
}

Mat3 orthonormalized(const Mat3&m){
	Vec3 x=unit(Vec3(m.m[0][0],m.m[0][1],m.m[0][2]));
	Vec3 y0(m.m[1][0],m.m[1][1],m.m[1][2]);
	Vec3 y=unit(y0-x*Vec3::dot(x,y0));
	Vec3 z=unit(cross(x,y));
	Vec3 z0(m.m[2][0],m.m[2][1],m.m[2][2]);
	if(Vec3::dot(z,z0)<0.0){
		y=y*(-1.0);
		z=z*(-1.0);
	}
	Mat3 out;
	out.m[0][0]=x.x;
	out.m[0][1]=x.y;
	out.m[0][2]=x.z;
	out.m[1][0]=y.x;
	out.m[1][1]=y.y;
	out.m[1][2]=y.z;
	out.m[2][0]=z.x;
	out.m[2][1]=z.y;
	out.m[2][2]=z.z;
	return out;
}

Mat3 blend_rotation(const Mat3&a,const Mat3&b,double weight){
	Mat3 mixed;
	for(int i=0;i<3;++i){
		for(int j=0;j<3;++j){
			mixed.m[i][j]=a.m[i][j]+weight*(b.m[i][j]-a.m[i][j]);
		}
	}
	return orthonormalized(mixed);
}

Mat3 corrected_celestial_pole(const Mat3&rnpb,const EarthOrientation&eop){
	constexpr double mas_to_rad=PI/(180.0*3600.0*1000.0);
	Vec3 z(rnpb.m[2][0]+eop.dx_mas*mas_to_rad,
		   rnpb.m[2][1]+eop.dy_mas*mas_to_rad,0.0);
	double xy2=z.x*z.x+z.y*z.y;
	z.z=std::sqrt(std::max(0.0,1.0-xy2));
	z=unit(z);

	Vec3 x0(rnpb.m[0][0],rnpb.m[0][1],rnpb.m[0][2]);
	Vec3 x=unit(x0-z*Vec3::dot(x0,z));
	Vec3 y=unit(cross(z,x));

	Mat3 out;
	out.m[0][0]=x.x;
	out.m[0][1]=x.y;
	out.m[0][2]=x.z;
	out.m[1][0]=y.x;
	out.m[1][1]=y.y;
	out.m[1][2]=y.z;
	out.m[2][0]=z.x;
	out.m[2][1]=z.y;
	out.m[2][2]=z.z;
	return out;
}

}

Mat3 CoordTf::R1(double angle){
	double c=std::cos(angle);
	double s=std::sin(angle);
	Mat3 R;
	R.m[0][0]=1.0;
	R.m[0][1]=0.0;
	R.m[0][2]=0.0;
	R.m[1][0]=0.0;
	R.m[1][1]=c;
	R.m[1][2]=s;
	R.m[2][0]=0.0;
	R.m[2][1]=-s;
	R.m[2][2]=c;
	return R;
}

Mat3 CoordTf::R3(double angle){
	double c=std::cos(angle);
	double s=std::sin(angle);
	Mat3 R;
	R.m[0][0]=c;
	R.m[0][1]=s;
	R.m[0][2]=0.0;
	R.m[1][0]=-s;
	R.m[1][1]=c;
	R.m[1][2]=0.0;
	R.m[2][0]=0.0;
	R.m[2][1]=0.0;
	R.m[2][2]=1.0;
	return R;
}

Mat3 CoordTf::bias_mat(){
	Mat3 B;
	B.m[0][0]=0.9999999999999942;
	B.m[0][1]=-7.078279744199198e-8;
	B.m[0][2]=8.056148940257979e-8;

	B.m[1][0]=7.078279477857338e-8;
	B.m[1][1]=0.9999999999999969;
	B.m[1][2]=3.306041454222136e-8;

	B.m[2][0]=-8.056149173973727e-8;
	B.m[2][1]=-3.306040883980552e-8;
	B.m[2][2]=0.9999999999999962;
	return B;
}

Mat3 PrecNut::prec_mat(double jd_tdb){
	double weight=long_term_weight(jd_tdb);
	double epj=2000.0+(jd_tdb-2451545.0)/365.25;
	Mat3 long_bp;
	if(weight>0.0){
		lunar::precnut::ltp(epj,long_bp.m);
		long_bp=long_bp*CoordTf::bias_mat();
		if(weight>=1.0){
			return long_bp;
		}
	}

	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	Mat3 modern_bp;
	lunar::precnut::pmat06(d1,d2,modern_bp.m);
	return weight>0.0?blend_rotation(modern_bp,long_bp,weight):modern_bp;
}

double PrecNut::mean_obl(double jd_tdb){
	double weight=long_term_weight(jd_tdb);
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	double modern=lunar::precnut::obl06(d1,d2);
	return weight>0.0?modern+weight*(long_term_obliquity(jd_tdb)-modern):modern;
}

std::pair<double,double> PrecNut::nut_ang(double jd_tdb){
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	double dpsi=0.0;
	double deps=0.0;
	lunar::precnut::nut06a(d1,d2,&dpsi,&deps);
	return {dpsi,deps};
}

Mat3 PrecNut::nut_mat(double jd_tdb){
	auto nd=nut_ang(jd_tdb);
	double eps=mean_obl(jd_tdb);
	return CoordTf::R1(-(eps+nd.second))*CoordTf::R3(-nd.first)*
		   CoordTf::R1(eps);
}

Mat3 PrecNut::earth_rot(double jd_tdb){
	double jd_ut1=TimeScale::tdb_to_ut1(jd_tdb);
	double uta=std::floor(jd_ut1);
	double utb=jd_ut1-uta;
	double jd_tt=TimeScale::tdb_to_tt(jd_tdb);
	double tta=std::floor(jd_tt);
	double ttb=jd_tt-tta;
	double gast=lunar::precnut::gst06a(uta,utb,tta,ttb);
	Mat3 earth_rotation=CoordTf::R3(gast);

	EarthOrientation eop=
		TimeScale::earth_orientation(TimeScale::tdb_to_utc(jd_tdb));
	if(!eop.available){
		return earth_rotation;
	}
	Mat3 rnpb=nut_mat(jd_tdb)*prec_mat(jd_tdb);
	Mat3 corrected=corrected_celestial_pole(rnpb,eop);
	gast=lunar::precnut::gst06(uta,utb,tta,ttb,corrected.m);
	earth_rotation=CoordTf::R3(gast)*corrected*transpose(rnpb);

	constexpr double arcsec_to_rad=PI/(180.0*3600.0);
	double tt_centuries=(jd_tt-2451545.0)/36525.0;
	double sp=-47e-6*arcsec_to_rad*tt_centuries;
	Mat3 polar_motion;
	lunar::precnut::pom00(eop.xp_arcsec*arcsec_to_rad,
						  eop.yp_arcsec*arcsec_to_rad,sp,polar_motion.m);
	return polar_motion*earth_rotation;
}
