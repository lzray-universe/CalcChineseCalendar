#pragma once

#include<cmath>
#include<string>
#include<type_traits>

constexpr double PI=3.141592653589793238462643383279502884;
constexpr double TWO_PI=2.0*PI;

constexpr double AU_KM=149597870.7;
constexpr double SEC_DAY=86400.0;
constexpr double C_AUDAY=173.144632674;

constexpr double UTC8DAY=8.0/24.0;

constexpr double SYNODDAY=29.530588;
constexpr double JD_EPSILON=1e-8;

struct UnitlessVecTag{};
struct LengthAuVecTag{};
struct SpeedAuDayVecTag{};
struct LengthKmVecTag{};
struct SpeedKmSecVecTag{};

template<class Tag>
struct BasicVec3{
	double x,y,z;
	BasicVec3() : x(0.0),y(0.0),z(0.0){}
	BasicVec3(double xx,double yy,double zz) : x(xx),y(yy),z(zz){}

	BasicVec3 operator+(const BasicVec3&b) const{
		return BasicVec3(x+b.x,y+b.y,z+b.z);
	}
	BasicVec3 operator-(const BasicVec3&b) const{
		return BasicVec3(x-b.x,y-b.y,z-b.z);
	}
	BasicVec3 operator*(double s) const{ return BasicVec3(x*s,y*s,z*s); }
	BasicVec3 operator/(double s) const{ return BasicVec3(x/s,y/s,z/s); }

	BasicVec3&operator+=(const BasicVec3&b){
		x+=b.x;
		y+=b.y;
		z+=b.z;
		return *this;
	}
	BasicVec3&operator-=(const BasicVec3&b){
		x-=b.x;
		y-=b.y;
		z-=b.z;
		return *this;
	}

	double norm() const{ return std::sqrt(x*x+y*y+z*z); }

	static double dot(const BasicVec3&a,const BasicVec3&b){
		return a.x*b.x+a.y*b.y+a.z*b.z;
	}
};

using Vec3=BasicVec3<UnitlessVecTag>;

#ifndef LUNAR_ENABLE_DIMENSION_TYPES
#define LUNAR_ENABLE_DIMENSION_TYPES 1
#endif

#if LUNAR_ENABLE_DIMENSION_TYPES
using Pos3=BasicVec3<LengthAuVecTag>;
using Vel3=BasicVec3<SpeedAuDayVecTag>;
using PosKm3=BasicVec3<LengthKmVecTag>;
using VelKmSec3=BasicVec3<SpeedKmSecVecTag>;
#else
using Pos3=Vec3;
using Vel3=Vec3;
using PosKm3=Vec3;
using VelKmSec3=Vec3;
#endif

template<class Tag>
inline BasicVec3<Tag> operator*(double s,const BasicVec3<Tag>&v){ return v*s; }

template<class ToTag,class FromTag>
inline BasicVec3<ToTag> vec_cast(const BasicVec3<FromTag>&v){
	return BasicVec3<ToTag>(v.x,v.y,v.z);
}

inline Pos3 pos3(const Vec3&v){ return Pos3(v.x,v.y,v.z); }

inline Vel3 vel3(const Vec3&v){ return Vel3(v.x,v.y,v.z); }

inline PosKm3 pos_km3(const Vec3&v){ return PosKm3(v.x,v.y,v.z); }

inline VelKmSec3 vel_kms3(const Vec3&v){ return VelKmSec3(v.x,v.y,v.z); }

template<class Tag>
inline Vec3 raw_vec(const BasicVec3<Tag>&v){
	return Vec3(v.x,v.y,v.z);
}

static_assert(sizeof(Pos3)==sizeof(Vec3),"typed vectors must stay zero-cost");
static_assert(sizeof(Vel3)==sizeof(Vec3),"typed vectors must stay zero-cost");
static_assert(std::is_trivially_copyable<Pos3>::value,
			  "typed vectors must stay trivially copyable");
static_assert(std::is_trivially_copyable<Vel3>::value,
			  "typed vectors must stay trivially copyable");

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

	template<class Tag>
	BasicVec3<Tag> operator*(const BasicVec3<Tag>&v) const{
		return BasicVec3<Tag>(m[0][0]*v.x+m[0][1]*v.y+m[0][2]*v.z,
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
