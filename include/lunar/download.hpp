#pragma once

#include<string>
#include<vector>

struct BspOption{
	std::string id;
	std::string url;
	std::string size;
	std::string range;
};

std::vector<BspOption> bsp_opts();

bool cmd_exist(const std::string&cmd);

bool dl_file(const std::string&url,const std::string&out_path);
