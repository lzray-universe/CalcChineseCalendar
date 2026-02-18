#include "lunar/frames.hpp"

#include<cmath>

extern "C"{
#ifdef USE_ERFA
#include "erfa.h"
#endif
}

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
#ifdef USE_ERFA
	double epj=2000.0+(jd_tdb-2451545.0)/365.25;
	if(PREC_MODEL==PrecModel::VONDRAK||
	   (PREC_MODEL==PrecModel::AUTO&&std::fabs(epj-2000.0)>=LONG_THR)){
		Mat3 R;
		eraLtp(epj,R.m);
		return R;
	}

	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	Mat3 R;
	eraPmat06(d1,d2,R.m);
	return R;
#else
	double T=(jd_tdb-2451545.0)/36525.0;
	double psi_A=(5038.481507*T-1.0790069*T*T-0.00114045*T*T*T+
				  0.000132851*std::pow(T,4)-0.0000000951*std::pow(T,5));
	double omega_A=(84381.406-0.025754*T+0.0512623*T*T-0.00772503*std::pow(T,3)-
					0.000000467*std::pow(T,4)+0.0000000337*std::pow(T,5));
	double chi_A=(10.556403*T-2.3814292*T*T-0.00121197*T*T*T+
				  0.000170663*std::pow(T,4)-0.0000000560*std::pow(T,5));
	double eps0=84381.406;

	double as2rad=PI/648000.0;
	psi_A*=as2rad;
	omega_A*=as2rad;
	chi_A*=as2rad;
	eps0*=as2rad;

	return CoordTf::R3(chi_A)*CoordTf::R1(-omega_A)*CoordTf::R3(-psi_A)*
		   CoordTf::R1(eps0);
#endif
}

double PrecNut::mean_obl(double jd_tdb){
#ifdef USE_ERFA
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	return eraObl06(d1,d2);
#else
	double T=(jd_tdb-2451545.0)/36525.0;
	double as2rad=PI/648000.0;
	double eps=(84381.406-46.836769*T-0.0001831*T*T+0.00200340*std::pow(T,3)-
				0.000000576*std::pow(T,4)-0.0000000434*std::pow(T,5))*
			   as2rad;
	return eps;
#endif
}

std::pair<double,double> PrecNut::nut_ang(double jd_tdb){
#ifdef USE_ERFA
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	double dpsi=0.0;
	double deps=0.0;
	eraNut00a(d1,d2,&dpsi,&deps);
	return {dpsi,deps};
#else
	double T=(jd_tdb-2451545.0)/36525.0;
	double l=(134.96340251+1717915923.2178*T+31.8792*T*T+0.051635*std::pow(T,3)-
			  0.00024470*std::pow(T,4));
	double lp=(357.52910918+129596581.0481*T-0.5532*T*T+0.000136*std::pow(T,3)-
			   0.00001149*std::pow(T,4));
	double F=(93.27209062+1739527262.8478*T-12.7512*T*T-0.001037*std::pow(T,3)+
			  0.00000417*std::pow(T,4));
	double D=(297.85019547+1602961601.2090*T-6.3706*T*T+0.006593*std::pow(T,3)-
			  0.00003169*std::pow(T,4));
	double Om=(125.04455501-6962890.5431*T+7.4722*T*T+0.007702*std::pow(T,3)-
			   0.00005939*std::pow(T,4));

	double d2r=PI/180.0;
	l*=d2r;
	lp*=d2r;
	F*=d2r;
	D*=d2r;
	Om*=d2r;
	double as2rad=PI/648000.0;

	double dpsi=
		(-17.20642418*std::sin(Om)+0.003386*std::cos(Om)-
		 1.31709122*std::sin(2*F-2*D+2*Om)-0.0013696*std::cos(2*F-2*D+2*Om))*
		as2rad;
	double deps=
		(0.0015377*std::sin(Om)+9.2052331*std::cos(Om)-
		 0.0004587*std::sin(2*F-2*D+2*Om)+0.5730336*std::cos(2*F-2*D+2*Om))*
		as2rad;
	return {dpsi,deps};
#endif
}

Mat3 PrecNut::nut_mat(double jd_tdb){
#ifdef USE_ERFA
	double d1=std::floor(jd_tdb);
	double d2=jd_tdb-d1;
	Mat3 N;
	eraNum06a(d1,d2,N.m);
	return N;
#else
	auto nd=nut_ang(jd_tdb);
	double dpsi=nd.first;
	double deps=nd.second;
	double epsA=mean_obl(jd_tdb);
	double eps=epsA+deps;
	Mat3 N=CoordTf::R1(-eps)*CoordTf::R3(-dpsi)*CoordTf::R1(epsA);
	return N;
#endif
}
