#include "lunar/precnut_core.hpp"

extern "C"{

void eraLtp(double epj,double rp[3][3]);

void eraLtpequ(double epj,double equator_pole[3]);

void eraLtpecl(double epj,double ecliptic_pole[3]);

void eraPmat06(double date1,double date2,double rbp[3][3]);

double eraObl06(double date1,double date2);

void eraNut00a(double date1,double date2,double*dpsi,double*deps);

void eraNut06a(double date1,double date2,double*dpsi,double*deps);

void eraNum06a(double date1,double date2,double rmatn[3][3]);

double eraGst06(double uta,double utb,double tta,double ttb,double rnpb[3][3]);

double eraGst06a(double uta,double utb,double tta,double ttb);

void eraPom00(double xp,double yp,double sp,double rpom[3][3]);

}

namespace lunar::precnut{

void ltp(double epj,double rp[3][3]){
	eraLtp(epj,rp);
}

void ltpequ(double epj,double equator_pole[3]){
	eraLtpequ(epj,equator_pole);
}

void ltpecl(double epj,double ecliptic_pole[3]){
	eraLtpecl(epj,ecliptic_pole);
}

void pmat06(double date1,double date2,double rbp[3][3]){
	eraPmat06(date1,date2,rbp);
}

double obl06(double date1,double date2){
	return eraObl06(date1,date2);
}

void nut00a(double date1,double date2,double*dpsi,double*deps){
	eraNut00a(date1,date2,dpsi,deps);
}

void nut06a(double date1,double date2,double*dpsi,double*deps){
	eraNut06a(date1,date2,dpsi,deps);
}

void num06a(double date1,double date2,double rmatn[3][3]){
	eraNum06a(date1,date2,rmatn);
}

double gst06(double uta,double utb,double tta,double ttb,double rnpb[3][3]){
	return eraGst06(uta,utb,tta,ttb,rnpb);
}

double gst06a(double uta,double utb,double tta,double ttb){
	return eraGst06a(uta,utb,tta,ttb);
}

void pom00(double xp,double yp,double sp,double rpom[3][3]){
	eraPom00(xp,yp,sp,rpom);
}

}
