#include "lunar/precnut_core.hpp"

extern "C"{

void eraLtp(double epj,double rp[3][3]);

void eraPmat06(double date1,double date2,double rbp[3][3]);

double eraObl06(double date1,double date2);

void eraNut00a(double date1,double date2,double*dpsi,double*deps);

void eraNum06a(double date1,double date2,double rmatn[3][3]);

}

namespace lunar::precnut{

void ltp(double epj,double rp[3][3]){
	eraLtp(epj,rp);
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

void num06a(double date1,double date2,double rmatn[3][3]){
	eraNum06a(date1,date2,rmatn);
}

}
