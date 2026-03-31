#pragma once

#include<string>

struct RootTask{
	std::string kind;
	double target;
	double jd_initial;
	double eps_days;
	int max_iter;
};
