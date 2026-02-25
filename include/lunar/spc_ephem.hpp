#pragma once

#include<map>
#include<memory>
#include<mutex>
#include<string>
#include<utility>
#include<vector>

#include "lunar/math.hpp"

struct SpkFile;

struct EphRead{
	std::string filepath;
	int SSB;
	int SUN;
	int EMB;
	int EARTH;
	int MOON;
	std::map<int,std::string> id_name;
	std::shared_ptr<SpkFile> kern;
	std::once_flag kern_flag;

	explicit EphRead(const std::string&path);

	void load_kern();

	std::string to_name(int code) const;

	void val_kern();

	static double et_fromjd(double jd_tdb);

	std::pair<Vec3,Vec3> get_state_et_km(int target,int observer,double et_tdb);

	Vec3 get_pos_et_km(int target,int observer,double et_tdb);

	Vec3 get_vel_et_kms(int target,int observer,double et_tdb);

	std::pair<Vec3,Vec3> get_state(int target,int observer,double jd_tdb);

	Vec3 get_pos(int target,int observer,double jd_tdb);

	Vec3 get_vel(int target,int observer,double jd_tdb);

	std::vector<int> spk_objects();

	std::vector<std::pair<double,double>> spk_coverage(int obj);
};
