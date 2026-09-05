#include<gtest/gtest.h>

#include "lunar/frames.hpp"
#include "lunar/math.hpp"

#include<algorithm>
#include<array>
#include<cmath>

TEST(GregorianJulianDate, BceCenturyDatesRoundTrip){
	for(int year : {-1000,-500,-300,-200,-100}){
		double jd=greg2jd(year,3,1,12,34,56.0);
		int y=0;
		int m=0;
		int d=0;
		int hh=0;
		int mm=0;
		double ss=0.0;
		jd2greg(jd,y,m,d,hh,mm,ss);
		EXPECT_EQ(y,year);
		EXPECT_EQ(m,3);
		EXPECT_EQ(d,1);
		EXPECT_EQ(hh,12);
		EXPECT_EQ(mm,34);
		EXPECT_NEAR(ss,56.0,1e-4);
	}
}

TEST(GregorianJulianDate, BceAdjacentDaysStayAdjacent){
	for(int year : {-1000,-500,-300,-200,-100}){
		EXPECT_DOUBLE_EQ(greg2jd(year,3,1)-greg2jd(year,2,28),1.0);
	}
}

TEST(ReferenceFrames, ModernPrecessionAlreadyIncludesFrameBias){
	Mat3 bp=PrecNut::prec_mat(2451545.0);
	Mat3 bias=CoordTf::bias_mat();
	for(int i=0;i<3;++i){
		for(int j=0;j<3;++j){
			EXPECT_NEAR(bp.m[i][j],bias.m[i][j],2e-12);
		}
	}
}

TEST(ReferenceFrames, LongTermSwitchHasNoExtraBiasStep){
	double boundary=2451545.0+LONG_THR*365.25;
	Mat3 before=PrecNut::prec_mat(boundary-1e-6);
	Mat3 after=PrecNut::prec_mat(boundary+1e-6);
	double max_jump=0.0;
	for(int i=0;i<3;++i){
		for(int j=0;j<3;++j){
			max_jump=std::max(max_jump,std::fabs(after.m[i][j]-before.m[i][j]));
		}
	}
	EXPECT_LT(max_jump,1e-9);
}

TEST(ReferenceFrames, EarthFixedRotationWithEopIsOrthonormal){
	double jd_tdb=greg2jd(2026,9,4)+69.184/SEC_DAY;
	Mat3 rotation=PrecNut::earth_rot(jd_tdb);
	for(int i=0;i<3;++i){
		for(int j=0;j<3;++j){
			double dot=0.0;
			for(int k=0;k<3;++k){
				dot+=rotation.m[i][k]*rotation.m[j][k];
			}
			EXPECT_NEAR(dot,i==j?1.0:0.0,2e-14);
		}
	}
}
