#pragma once

#include<map>
#include<set>
#include<string>
#include<utility>

#include "lunar/math.hpp"

void cfg_spice();
void chk_spice(const std::string&context);

struct EphRead{
	std::string filepath;
	int SSB;
	int SUN;
	int EMB;
	int EARTH;
	int MOON;
	std::map<int,std::string> id_name;
	static std::set<std::string> load_paths;
	static std::set<std::string> val_paths;

	explicit EphRead(const std::string&path);

	void load_kern();

	std::string to_name(int code) const;

	void val_kern();

	static double et_fromjd(double jd_tdb);

	std::pair<Vec3,Vec3> get_state(int target,int observer,double jd_tdb);

	Vec3 get_pos(int target,int observer,double jd_tdb);

	Vec3 get_vel(int target,int observer,double jd_tdb);
};
