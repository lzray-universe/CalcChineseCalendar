#pragma once

#include<string>
#include<vector>

struct GlobalContext{
	std::vector<std::string> bsp_candidates;
	std::string default_tz="+08:00";
	std::string default_format="txt";
	bool default_pretty=true;
};

GlobalContext load_global_ctx();

std::vector<std::string> prep_cmd_args(const std::string&command,
									   const std::vector<std::string>&args,
									   const GlobalContext&ctx);
