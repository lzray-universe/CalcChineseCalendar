#pragma once

namespace lunar::precnut{

void ltp(double epj,double rp[3][3]);

void ltpequ(double epj,double equator_pole[3]);

void ltpecl(double epj,double ecliptic_pole[3]);

void pmat06(double date1,double date2,double rbp[3][3]);

double obl06(double date1,double date2);

void nut00a(double date1,double date2,double*dpsi,double*deps);

void nut06a(double date1,double date2,double*dpsi,double*deps);

void num06a(double date1,double date2,double rmatn[3][3]);

double gst06(double uta,double utb,double tta,double ttb,double rnpb[3][3]);

double gst06a(double uta,double utb,double tta,double ttb);

void pom00(double xp,double yp,double sp,double rpom[3][3]);

}
