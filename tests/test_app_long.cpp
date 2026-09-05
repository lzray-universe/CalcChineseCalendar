#include<gtest/gtest.h>

#include "lunar/app_long.hpp"
#include "lunar/time_scale.hpp"

#include<cmath>

namespace{

double wrap_pm_pi(double angle){
	double value=std::fmod(angle+PI,TWO_PI);
	if(value<0.0){
		value+=TWO_PI;
	}
	return value-PI;
}

void expect_vec_near(const Vec3&a,const Vec3&b,double tolerance){
	EXPECT_NEAR(a.x,b.x,tolerance);
	EXPECT_NEAR(a.y,b.y,tolerance);
	EXPECT_NEAR(a.z,b.z,tolerance);
}

}

TEST(AberrationStates, LightTimeKeepsObserverAtReceptionEpoch){
	EphRead eph("series");
	double t=TimeScale::utc_to_tdb(greg2jd(2026,2,20));
	AberState geometric=AberCorr::geo_geom_state(eph,eph.SUN,t);
	auto target_now=eph.get_state(eph.SUN,eph.SSB,t);
	auto observer=eph.get_state(eph.EARTH,eph.SSB,t);
	expect_vec_near(raw_vec(geometric.X),
					raw_vec(target_now.first-observer.first),1e-15);
	expect_vec_near(raw_vec(geometric.V),
					raw_vec(target_now.second-observer.second),1e-15);
	EXPECT_EQ(geometric.tr,t);
	EXPECT_EQ(geometric.tr_rate,1.0);

	AberState state=AberCorr::geo_lt_state(eph,eph.SUN,t,6);
	auto target=eph.get_state(eph.SUN,eph.SSB,state.tr);
	expect_vec_near(raw_vec(state.X),raw_vec(target.first-observer.first),1e-15);
	EXPECT_NEAR(state.tr,t-state.X.norm()/C_AUDAY,1e-12);

	constexpr double h=1e-3;
	AberState before=AberCorr::geo_lt_state(eph,eph.SUN,t-h,6);
	AberState after=AberCorr::geo_lt_state(eph,eph.SUN,t+h,6);
	Vec3 finite_difference=raw_vec(after.X-before.X)/(2.0*h);
	expect_vec_near(raw_vec(state.V),finite_difference,3e-9);
}

TEST(AberrationStates, ApparentStateDerivativeMatchesPositionChain){
	EphRead eph("series");
	constexpr double h=1e-3;
	for(double t : {TimeScale::utc_to_tdb(greg2jd(2026,4,4)),
					 TimeScale::utc_to_tdb(greg2jd(2026,6,16))}){
		for(int target : {199,eph.SUN,eph.MOON}){
			AberState state=AberCorr::geo_app_state(eph,target,t,6);
			Pos3 before=AberCorr::geo_app(eph,target,t-h,6);
			Pos3 after=AberCorr::geo_app(eph,target,t+h,6);
			Vec3 finite_difference=raw_vec(after-before)/(2.0*h);
			SCOPED_TRACE(target);
			SCOPED_TRACE(t);
			expect_vec_near(raw_vec(state.V),finite_difference,3e-9);
			EXPECT_NEAR(state.X.norm(),
						AberCorr::geo_lt_state(eph,target,t,6).X.norm(),1e-14);
		}
	}
}

TEST(ApparentLongitude, DerivativeUsesCompletePositionChain){
	EphRead eph("series");
	AppLon app(eph);
	double t=TimeScale::utc_to_tdb(greg2jd(2026,4,4));
	constexpr double h=1e-3;
	for(int target : {eph.SUN,eph.MOON}){
		auto state=(target==eph.SUN)?app.sun_calc(t):app.moon_calc(t);
		double before=app.body_lam_app(target,t-h);
		double after=app.body_lam_app(target,t+h);
		double finite_difference=wrap_pm_pi(after-before)/(2.0*h);
		EXPECT_NEAR(state.second,finite_difference,1e-6);
	}
}
