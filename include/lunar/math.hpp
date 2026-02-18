#pragma once

#include<cmath>
#include<string>

constexpr double PI=3.141592653589793238462643383279502884;
constexpr double TWO_PI=2.0*PI;

constexpr double AU_KM=149597870.7;
constexpr double SEC_DAY=86400.0;
constexpr double C_AUDAY=173.144632674;

constexpr double UTC8DAY=8.0/24.0;

constexpr double SYNODDAY=29.530588;
constexpr double JD_EPSILON=1e-8;

struct Vec3{
	double x,y,z;
	Vec3() : x(0.0),y(0.0),z(0.0){}
	Vec3(double xx,double yy,double zz) : x(xx),y(yy),z(zz){}

	Vec3 operator+(const Vec3&b) const{ return Vec3(x+b.x,y+b.y,z+b.z); }
	Vec3 operator-(const Vec3&b) const{ return Vec3(x-b.x,y-b.y,z-b.z); }
	Vec3 operator*(double s) const{ return Vec3(x*s,y*s,z*s); }
	Vec3 operator/(double s) const{ return Vec3(x/s,y/s,z/s); }

	Vec3&operator+=(const Vec3&b){
		x+=b.x;
		y+=b.y;
		z+=b.z;
		return *this;
	}
	Vec3&operator-=(const Vec3&b){
		x-=b.x;
		y-=b.y;
		z-=b.z;
		return *this;
	}

	double norm() const{ return std::sqrt(x*x+y*y+z*z); }

	static double dot(const Vec3&a,const Vec3&b){
		return a.x*b.x+a.y*b.y+a.z*b.z;
	}
};

inline Vec3 operator*(double s,const Vec3&v){ return v*s; }

struct Mat3{
	double m[3][3];

	Mat3(){
		for(int i=0;i<3;++i){
			for(int j=0;j<3;++j){
				m[i][j]=0.0;
			}
		}
	}

	static Mat3 identity(){
		Mat3 I;
		I.m[0][0]=I.m[1][1]=I.m[2][2]=1.0;
		return I;
	}

	Vec3 operator*(const Vec3&v) const{
		return Vec3(m[0][0]*v.x+m[0][1]*v.y+m[0][2]*v.z,
					m[1][0]*v.x+m[1][1]*v.y+m[1][2]*v.z,
					m[2][0]*v.x+m[2][1]*v.y+m[2][2]*v.z);
	}

	Mat3 operator*(const Mat3&b) const{
		Mat3 r;
		for(int i=0;i<3;++i){
			for(int j=0;j<3;++j){
				double s=0.0;
				for(int k=0;k<3;++k){
					s+=m[i][k]*b.m[k][j];
				}
				r.m[i][j]=s;
			}
		}
		return r;
	}
};

double greg2jd(int year,int month,int day,int hour=0,int minute=0,
			   double second=0.0);

void jd2greg(double jd,int&year,int&month,int&day,int&hour,int&minute,
			 double&second);

struct LocalDT{
	int year,month,day;
	int hour,minute;
	double second;
	double utc_jd;

	LocalDT();

	static LocalDT from_loc(int y,int m,int d,int h=0,int min=0,double sec=0.0);

	static LocalDT fromUtcJD(double jd_utc);

	double toUtcJD() const;

	LocalDT shiftDays(double days) const;

	bool operator<(const LocalDT&b) const;
	bool operator>(const LocalDT&b) const;
	bool operator<=(const LocalDT&b) const;
	bool operator>=(const LocalDT&b) const;
};

std::string fmt_local(const LocalDT&t);
