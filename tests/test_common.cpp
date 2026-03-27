#include "test_common.hpp"

#include<atomic>
#include<chrono>
#include<cstdlib>
#include<fstream>
#include<sstream>
#include<stdexcept>

#include "lunar/spc_ephem.hpp"

namespace{

std::string test_bsp_from_env(){
	const char*raw=std::getenv("LUNAR_TEST_BSP");
	if(raw==nullptr||*raw=='\0'){
		return "";
	}
	std::error_code ec;
	if(!std::filesystem::exists(raw,ec)||ec){
		return "";
	}
	return raw;
}

std::string repo_local_bsp(){
	for(const char*name : {"de442.bsp","de440s.bsp"}){
		const std::filesystem::path path=name;
		std::error_code ec;
		if(std::filesystem::exists(path,ec)&&!ec){
			return path.string();
		}
	}
	return "";
}

}

std::string test_ephem(){
	const std::string env_bsp=test_bsp_from_env();
	if(!env_bsp.empty()){
		return env_bsp;
	}
#if LUNAR_ENABLE_SERIES_FALLBACK
	return kSeriesEphemToken;
#else
	return "";
#endif
}

bool has_test_ephem(){
	return !test_ephem().empty();
}

std::string reference_bsp(){
	const std::string env_bsp=test_bsp_from_env();
	if(!env_bsp.empty()){
		return env_bsp;
	}
	return repo_local_bsp();
}

bool has_reference_bsp(){
	return !reference_bsp().empty();
}

std::filesystem::path make_temp_path(const char*stem,const char*ext){
	static std::atomic<unsigned long long> seq{0};
	const unsigned long long tick=
		static_cast<unsigned long long>(
			std::chrono::steady_clock::now().time_since_epoch().count());
	const unsigned long long id=seq.fetch_add(1,std::memory_order_relaxed);
	return std::filesystem::temp_directory_path()/
		   ("lunar_"+std::string(stem)+"_"+std::to_string(tick)+"_"+
			std::to_string(id)+ext);
}

std::string read_file_text(const std::filesystem::path&path){
	std::ifstream ifs(path,std::ios::binary);
	if(!ifs){
		throw std::runtime_error("failed to open file: "+path.string());
	}
	std::ostringstream oss;
	oss<<ifs.rdbuf();
	return oss.str();
}

std::string txt_value(const std::string&text,const std::string&key){
	std::istringstream iss(text);
	std::string line;
	const std::string prefix=key+"=";
	while(std::getline(iss,line)){
		if(line.rfind(prefix,0)==0){
			return line.substr(prefix.size());
		}
	}
	return "";
}
