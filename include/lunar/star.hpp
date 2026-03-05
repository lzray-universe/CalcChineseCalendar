#pragma once

#include<cstddef>
#include<string>
#include<vector>

#include "lunar/spc_ephem.hpp"

namespace lunar{

struct StarRecord{
	const char*id=nullptr;
	const char*en=nullptr;
	const char*zh=nullptr;
	const char*ja=nullptr;
	const char*ko=nullptr;
	const char*bayer=nullptr;
	const char*region=nullptr;
	double ra_rad=0.0;
	double dec_rad=0.0;
	double pmra_rad_yr=0.0;
	double pmdec_rad_yr=0.0;
	double pmra_mas_yr=0.0;
	double pmdec_mas_yr=0.0;
	double dist_pc=0.0;
	double x_pc=0.0;
	double y_pc=0.0;
	double z_pc=0.0;
	double vx_pc_yr=0.0;
	double vy_pc_yr=0.0;
	double vz_pc_yr=0.0;
	double rv_km_s=0.0;
	double mag_v=0.0;
	double abs_mag_v=0.0;
	const char*spect=nullptr;
	double ci_bv=0.0;
	bool is_juxing=false;
};

extern const StarRecord B_STARS[];
extern const std::size_t B_STARS_COUNT;

enum class StarMode{
	Less,
	All,
	Pick,
};

struct StarPick{
	StarMode mode=StarMode::Less;
	std::vector<std::string> picks;
};

struct StarApp{
	std::string id;
	std::string name;
	std::string region;
	double mag_v=0.0;
	bool is_juxing=false;
	double ra_deg=0.0;
	double dec_deg=0.0;
	double sep_moon_deg=0.0;
};

struct MoonXg{
	std::string region;
	std::string star_id;
	std::string star_name;
	double sep_deg=0.0;
};

struct AstroEvt{
	std::string kind;
	std::string code;
	std::string name;
	double jd_utc=0.0;
	std::string detail;
};

struct AstroObs{
	bool has_site=false;
	double lat_deg=0.0;
	double lon_deg=0.0;
	double h_m=0.0;
};

StarMode parse_star_mode(const std::string&text);

StarPick make_star_pick(StarMode mode,const std::string&pick_csv);

std::vector<StarApp> calc_star_app(EphRead&eph,double jd_utc,
								   const StarPick&pick);

MoonXg calc_moon_xg(EphRead&eph,double jd_utc);

std::vector<AstroEvt> calc_astro_evt(EphRead&eph,double jd_utc_start,
									 double jd_utc_end,const StarPick&pick,
									 const AstroObs&obs=AstroObs{});

}
