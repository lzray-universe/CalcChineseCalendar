#include "lunar/download.hpp"

#include<cctype>
#include<cstdlib>
#include<iostream>

#include "lunar/i18n.hpp"

std::vector<BspOption> bsp_opts(){
	return {
		{"de440",
		 "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/"
		 "de440.bsp",
		 "≈114MB",lunar::i18n::pick("1550–2650年","1550-2650","1550-2650年","1550-2650년")},
		{"de440s",
		 "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/"
		 "de440s.bsp",
		 "≈31MB",lunar::i18n::pick("1850–2150年","1850-2150","1850-2150年","1850-2150년")},
		{"de441p1",
		 "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/"
		 "de441_part-1.bsp",
		 "≈1.5GB",lunar::i18n::pick("-13200–1969年","-13200-1969","-13200-1969年","-13200-1969년")},
		{"de441p2",
		 "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/"
		 "de441_part-2.bsp",
		 "≈1.5GB",lunar::i18n::pick("1969–17191年","1969-17191","1969-17191年","1969-17191년")},
		{"de442",
		 "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/"
		 "de442.bsp",
		 "≈114MB",lunar::i18n::pick("1550–2650年","1550-2650","1550-2650年","1550-2650년")},
		{"de442s",
		 "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/"
		 "de442s.bsp",
		 "≈31MB",lunar::i18n::pick("1850–2150年","1850-2150","1850-2150年","1850-2150년")},
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
		std::cout<<lunar::i18n::pick("未找到 curl 或 wget，请手动下载。",
									  "curl or wget not found. Please download manually.",
									  "curl または wget が見つかりません。手動でダウンロードしてください。",
									  "curl 또는 wget 을 찾지 못했습니다. 수동으로 다운로드하세요.")
				 <<std::endl;
		return false;
	}
	std::cout<<lunar::i18n::pick("正在下载: ","Downloading: ","ダウンロード中: ",
								 "다운로드 중: ")
			 <<url<<std::endl;
	int ret=std::system(tool.c_str());
	return ret==0;
}
