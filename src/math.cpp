#include "lunar/math.hpp"

#include<cmath>
#include<iomanip>
#include<sstream>

double greg2jd(int year,int month,int day,int hour,int minute,double second){
	int Y=year;
	int M=month;
	if(M<=2){
		Y-=1;
		M+=12;
	}
	int A=Y/100;
	int B=2-A+(A/4);

	double day_frac=(hour+(minute+second/60.0)/60.0)/24.0;

	double JD=std::floor(365.25*(Y+4716))+std::floor(30.6001*(M+1))+day+B-
			  1524.5+day_frac;
	return JD;
}

void jd2greg(double jd,int&year,int&month,int&day,int&hour,int&minute,
			 double&second){
	double Z_d=std::floor(jd+0.5);
	double F=(jd+0.5)-Z_d;
	long Z=static_cast<long>(Z_d);
	long A=Z;
	long alpha=static_cast<long>((Z-1867216.25)/36524.25);
	A=Z+1+alpha-alpha/4;
	long B=A+1524;
	long C=static_cast<long>((B-122.1)/365.25);
	long D=static_cast<long>(365.25*C);
	long E=static_cast<long>((B-D)/30.6001);

	double day_d=B-D-std::floor(30.6001*E)+F;
	day=static_cast<int>(std::floor(day_d));
	double frac_day=day_d-day;

	if(E<14){
		month=static_cast<int>(E-1);
	}else{
		month=static_cast<int>(E-13);
	}

	if(month>2){
		year=static_cast<int>(C-4716);
	}else{
		year=static_cast<int>(C-4715);
	}

	double tot_secs=frac_day*86400.0;
	if(tot_secs<0){
		tot_secs=0;
	}
	hour=static_cast<int>(tot_secs/3600.0);
	tot_secs-=hour*3600.0;
	minute=static_cast<int>(tot_secs/60.0);
	second=tot_secs-minute*60.0;

	if(second>=59.9995){
		second=0.0;
		minute+=1;
		if(minute>=60){
			minute=0;
			hour+=1;
			if(hour>=24){
				hour=0;
				jd2greg(jd+1.0,year,month,day,hour,minute,second);
			}
		}
	}
}

LocalDT::LocalDT()
	: year(2000),month(1),day(1),hour(0),minute(0),second(0.0),
	  utc_jd(2451544.5){}

LocalDT LocalDT::from_loc(int y,int m,int d,int h,int min,double sec){
	LocalDT t;
	t.year=y;
	t.month=m;
	t.day=d;
	t.hour=h;
	t.minute=min;
	t.second=sec;
	double jd_local=greg2jd(y,m,d,h,min,sec);
	t.utc_jd=jd_local-UTC8DAY;
	return t;
}

LocalDT LocalDT::fromUtcJD(double jd_utc){
	LocalDT t;
	t.utc_jd=jd_utc;
	double jd_local=jd_utc+UTC8DAY;
	jd2greg(jd_local,t.year,t.month,t.day,t.hour,t.minute,t.second);
	return t;
}

double LocalDT::toUtcJD() const{ return utc_jd; }

LocalDT LocalDT::shiftDays(double days) const{
	return LocalDT::fromUtcJD(utc_jd+days);
}

bool LocalDT::operator<(const LocalDT&b) const{ return utc_jd<b.utc_jd; }

bool LocalDT::operator>(const LocalDT&b) const{ return utc_jd>b.utc_jd; }

bool LocalDT::operator<=(const LocalDT&b) const{ return utc_jd<=b.utc_jd; }

bool LocalDT::operator>=(const LocalDT&b) const{ return utc_jd>=b.utc_jd; }

std::string fmt_local(const LocalDT&t){
	std::ostringstream oss;
	oss<<std::setfill('0')<<std::setw(4)<<t.year<<"-"<<std::setw(2)<<t.month
	   <<"-"<<std::setw(2)<<t.day<<" "<<std::setw(2)<<t.hour<<":"<<std::setw(2)
	   <<t.minute<<":";
	oss<<std::fixed<<std::setprecision(3)<<std::setw(6)<<t.second;
	return oss.str();
}
