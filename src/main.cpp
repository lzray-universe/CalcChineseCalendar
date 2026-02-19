#include "lunar/entry.hpp"

#include<exception>
#include<iostream>
#include<string>
#include<vector>

#ifdef _WIN32
#include<windows.h>
#endif

namespace{

std::vector<std::string> col_args(int argc,char**argv){
	std::vector<std::string> out;
	out.reserve(argc>1?static_cast<std::size_t>(argc-1):0);
	for(int i=1;i<argc;++i){
		out.emplace_back(argv[i]);
	}
	return out;
}

}

int main(int argc,char**argv){
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	try{
		return run_cli_args(col_args(argc,argv));

	}catch(const std::invalid_argument&ex){
		std::cerr<<"argument error: "<<ex.what()<<std::endl;
		return 2;
	}catch(const std::exception&ex){
		std::cerr<<"error: "<<ex.what()<<std::endl;
		return 1;
	}
}
