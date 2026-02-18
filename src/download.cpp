#include "lunar/download.hpp"

#include<cctype>
#include<cstdlib>
#include<iostream>

std::vector<BspOption> bsp_opts(){
	return {
		{"de440",
		 "https://naif.jpl.nasa.gov/pub/naif/gen_kern/spk/planets/"
		 "de440.bsp",
		 "≈114MB","1550–2650年"},
		{"de440s",
		 "https://naif.jpl.nasa.gov/pub/naif/gen_kern/spk/planets/"
		 "de440s.bsp",
		 "≈31MB","1850–2150年"},
		{"de441p1",
		 "https://naif.jpl.nasa.gov/pub/naif/gen_kern/spk/planets/"
		 "de441_part-1.bsp",
		 "≈1.5GB","-13200–1969年"},
		{"de441p2",
		 "https://naif.jpl.nasa.gov/pub/naif/gen_kern/spk/planets/"
		 "de441_part-2.bsp",
		 "≈1.5GB","1969–17191年"},
		{"de442",
		 "https://naif.jpl.nasa.gov/pub/naif/gen_kern/spk/planets/"
		 "de442.bsp",
		 "≈114MB","1550–2650年"},
		{"de442s",
		 "https://naif.jpl.nasa.gov/pub/naif/gen_kern/spk/planets/"
		 "de442s.bsp",
		 "≈31MB","1850–2150年"},
	};
}

bool cmd_exist(const std::string&cmd){
#ifdef _WIN32
	std::string check="where "+cmd+" >NUL 2>NUL";
#else
	std::string check="command -v "+cmd+" >/dev/null 2>&1";
#endif
	int ret=std::system(check.c_str());
	return ret==0;
}

bool dl_file(const std::string&url,const std::string&out_path){
	std::string tool;
	if(cmd_exist("curl")){
		tool="curl -L --fail -o \""+out_path+"\" \""+url+"\"";
	}else if(cmd_exist("wget")){
		tool="wget -O \""+out_path+"\" \""+url+"\"";
	}else{
		std::cout<<"未找到 curl 或 wget，请手动下载。"<<std::endl;
		return false;
	}
	std::cout<<"正在下载: "<<url<<std::endl;
	int ret=std::system(tool.c_str());
	return ret==0;
}
