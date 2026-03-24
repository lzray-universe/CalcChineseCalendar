#pragma once

#include<cstddef>
#include<limits>
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

enum class SkyMode{
	All,
	Pick,
};

struct SkyPick{
	SkyMode mode=SkyMode::All;
	std::vector<std::string> picks;
};

struct SkyPos{
	std::string kind;
	std::string code;
	std::string name;
	std::string region;
	bool is_solar_system=false;
	bool is_juxing=false;
	double mag_v=std::numeric_limits<double>::quiet_NaN();
	double ra_deg=std::numeric_limits<double>::quiet_NaN();
	double dec_deg=std::numeric_limits<double>::quiet_NaN();
	double az_deg=std::numeric_limits<double>::quiet_NaN();
	double alt_deg=std::numeric_limits<double>::quiet_NaN();
};

StarMode parse_star_mode(const std::string&text);

StarPick make_star_pick(StarMode mode,const std::string&pick_csv);

SkyMode parse_sky_mode(const std::string&text);

SkyPick make_sky_pick(SkyMode mode,const std::string&pick_csv);

std::vector<StarApp> calc_star_app(EphRead&eph,double jd_utc,
								   const StarPick&pick);

MoonXg calc_moon_xg(EphRead&eph,double jd_utc);

std::vector<SkyPos> calc_sky_pos(EphRead&eph,double jd_utc,
								 const AstroObs&obs,const SkyPick&pick);

std::vector<AstroEvt> calc_astro_evt(EphRead&eph,double jd_utc_start,
									 double jd_utc_end,const StarPick&pick,
									 const AstroObs&obs=AstroObs{});

}
