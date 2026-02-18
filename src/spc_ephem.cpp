#include "lunar/spc_ephem.hpp"

#include<filesystem>
#include<mutex>
#include<sstream>
#include<stdexcept>
#include<vector>

extern "C"{
#include "SpiceUsr.h"
}

namespace fs=std::filesystem;

std::set<std::string> EphRead::load_paths;
std::set<std::string> EphRead::val_paths;

EphRead::EphRead(const std::string&path){
	filepath=path;
	if(filepath.empty()){
		throw std::runtime_error("ephemeris path is empty");
	}
	SSB=0;
	SUN=10;
	EMB=3;
	EARTH=399;
	MOON=301;

	id_name[SSB]="SOLAR SYSTEM BARYCENTER";
	id_name[SUN]="SUN";
	id_name[EMB]="EARTH BARYCENTER";
	id_name[EARTH]="EARTH";
	id_name[MOON]="MOON";

	cfg_spice();
	load_kern();
}

void EphRead::load_kern(){
	cfg_spice();

	if(!fs::exists(filepath)){
		throw std::runtime_error("ephemeris file not found: "+filepath);
	}

	std::error_code ec;
	auto fsize=fs::file_size(filepath,ec);
	if(ec||fsize==0){
		throw std::runtime_error("ephemeris file is not readable or empty: "+
								 filepath);
	}

	bool need_load=false;
	SpiceInt count=0;
	try{
		ktotal_c("SPK",&count);
		if(count==0){
			need_load=true;
		}
	}catch(...){
		need_load=true;
	}
	if(load_paths.find(filepath)==load_paths.end()){
		need_load=true;
	}
	if(need_load){
		furnsh_c(filepath.c_str());
		chk_spice("Failed to load ephemeris kernel");
		load_paths.insert(filepath);
	}

	val_kern();
}

std::string EphRead::to_name(int code) const{
	auto it=id_name.find(code);
	if(it==id_name.end()){
		throw std::runtime_error("Unknown target/observer code");
	}
	return it->second;
}

void EphRead::val_kern(){
	cfg_spice();

	SpiceInt count=0;
	ktotal_c("SPK",&count);
	chk_spice("Failed to query loaded SPK kernels");
	if(count==0){
		throw std::runtime_error(
			"No SPK kernels are loaded; expected ephemeris "+filepath);
	}
	val_paths.insert(filepath);
}

double EphRead::et_fromjd(double jd_tdb){ return (jd_tdb-2451545.0)*SEC_DAY; }

std::pair<Vec3,Vec3> EphRead::get_state(int target,int observer,double jd_tdb){
	load_kern();
	double et=et_fromjd(jd_tdb);
	std::string tname=to_name(target);
	std::string oname=to_name(observer);
	SpiceDouble state[6];
	SpiceDouble lt;
	spkezr_c(tname.c_str(),et,"J2000","NONE",oname.c_str(),state,&lt);
	chk_spice("spkezr_c failed for target "+tname+" observer "+oname);
	Vec3 pos(state[0]/AU_KM,state[1]/AU_KM,state[2]/AU_KM);
	Vec3 vel(state[3]*(SEC_DAY/AU_KM),state[4]*(SEC_DAY/AU_KM),
			 state[5]*(SEC_DAY/AU_KM));
	return {pos,vel};
}

Vec3 EphRead::get_pos(int target,int observer,double jd_tdb){
	return get_state(target,observer,jd_tdb).first;
}

Vec3 EphRead::get_vel(int target,int observer,double jd_tdb){
	return get_state(target,observer,jd_tdb).second;
}

void chk_spice(const std::string&context){
	if(!failed_c()){
		return;
	}

	SpiceChar msg[1841];
	getmsg_c("LONG",sizeof(msg),msg);
	reset_c();
	throw std::runtime_error(context+": "+std::string(msg));
}

void cfg_spice(){
	static std::once_flag flag;
	std::call_once(flag,[](){
		SpiceChar action[]="RETURN";
		SpiceChar detail[]="SHORT,EXPLAIN";
		erract_c("SET",0,action);
		errprt_c("SET",0,detail);
	});
}
