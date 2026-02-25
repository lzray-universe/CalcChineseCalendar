#include "lunar/frames.hpp"
#include "lunar/precnut_core.hpp"

#include<cmath>

const PrecModel PREC_MODEL=PrecModel::AUTO;
const double LONG_THR=10.0;

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
	double epj=2000.0+(jd_tdb-2451545.0)/365.25;
	if(PREC_MODEL==PrecModel::VONDRAK||
	   (PREC_MODEL==PrecModel::AUTO&&std::fabs(epj-2000.0)>=LONG_THR)){
		Mat3 R;
		lunar::precnut::ltp(epj,R.m);
		return R;
	}

	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	Mat3 R;
	lunar::precnut::pmat06(d1,d2,R.m);
	return R;
}

double PrecNut::mean_obl(double jd_tdb){
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	return lunar::precnut::obl06(d1,d2);
}

std::pair<double,double> PrecNut::nut_ang(double jd_tdb){
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	double dpsi=0.0;
	double deps=0.0;
	lunar::precnut::nut00a(d1,d2,&dpsi,&deps);
	return {dpsi,deps};
}

Mat3 PrecNut::nut_mat(double jd_tdb){
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	Mat3 N;
	lunar::precnut::num06a(d1,d2,N.m);
	return N;
}
