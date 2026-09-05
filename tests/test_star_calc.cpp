#include<gtest/gtest.h>

#include "lunar/star.hpp"

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
