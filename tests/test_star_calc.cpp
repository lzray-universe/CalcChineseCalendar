#include<gtest/gtest.h>

#include "lunar/app_long.hpp"
#include "lunar/frames.hpp"
#include "lunar/star.hpp"
#include "lunar/time_scale.hpp"

#include "test_common.hpp"

#include<cmath>
#include<string>
#include<vector>

namespace{

const lunar::AstroEvt*nearest_event(const std::vector<lunar::AstroEvt>&events,
						  const std::string&kind,double expected_jd){
	const lunar::AstroEvt*best=nullptr;
	double best_distance=1e100;
	for(const lunar::AstroEvt&event : events){
		if(event.kind!=kind){
			continue;
		}
		double distance=std::fabs(event.jd_utc-expected_jd);
		if(distance<best_distance){
			best=&event;
			best_distance=distance;
		}
	}
	return best;
}

}

TEST(StarBodyAspect, RefinesBracketedPhaseRoot){
	EphRead eph("@series");
	lunar::StarPick pick=lunar::make_star_pick(lunar::StarMode::Pick,"Vega");
	const std::vector<lunar::AstroEvt> events=lunar::calc_astro_evt(
		eph,greg2jd(2026,1,1),greg2jd(2026,1,10),pick);
	const lunar::AstroEvt*event=nullptr;
	for(const lunar::AstroEvt&candidate : events){
		if(candidate.code=="star_hr7001_sun_conjunction"){
			event=&candidate;
			break;
		}
	}
	ASSERT_NE(event,nullptr);

	const std::vector<lunar::StarApp> star=
		lunar::calc_star_app(eph,event->jd_utc,pick);
	ASSERT_EQ(star.size(),1u);
	const double ra=star[0].ra_deg*PI/180.0;
	const double dec=star[0].dec_deg*PI/180.0;
	Vec3 star_eq(std::cos(dec)*std::cos(ra),std::cos(dec)*std::sin(ra),
				 std::sin(dec));
	const double t=TimeScale::utc_to_tdb(event->jd_utc);
	const double eps=PrecNut::mean_obl(t)+PrecNut::nut_ang(t).second;
	Vec3 star_ecl=CoordTf::R1(eps)*star_eq;
	const double star_lam=std::atan2(star_ecl.y,star_ecl.x);
	AppLon app(eph);
	const double sun_lam=app.sun_calc(t).first;
	const double residual=std::atan2(std::sin(star_lam-sun_lam),
								   std::cos(star_lam-sun_lam));
	EXPECT_LT(std::fabs(residual),1e-10);
}

TEST(AstroEventDefinitions, MercuryElongationAndStationUseStandardCoordinates){
	if(!has_reference_bsp()){
		GTEST_SKIP()<<"requires LUNAR_TEST_BSP or a repo-local BSP file";
	}
	EphRead eph(reference_bsp());
	lunar::StarPick pick;
	pick.mode=lunar::StarMode::Less;
	const std::vector<lunar::AstroEvt> events=lunar::calc_astro_evt(
		eph,greg2jd(2026,2,19),greg2jd(2026,2,27),pick);

	double elongation_jd=greg2jd(2026,2,20,1,41,9.0)-UTC8DAY;
	const lunar::AstroEvt*elongation=
		nearest_event(events,"greatest_elongation",elongation_jd);
	ASSERT_NE(elongation,nullptr);
	EXPECT_NE(elongation->code.find("mercury"),std::string::npos);
	EXPECT_NEAR(elongation->jd_utc,elongation_jd,5.0*60.0/SEC_DAY);

	double station_jd=greg2jd(2026,2,26,0,46,15.0)-UTC8DAY;
	const lunar::AstroEvt*station=nearest_event(events,"stationary",station_jd);
	ASSERT_NE(station,nullptr);
	EXPECT_NE(station->code.find("mercury"),std::string::npos);
	EXPECT_NEAR(station->jd_utc,station_jd,5.0*60.0/SEC_DAY);
}
