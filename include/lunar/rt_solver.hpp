#pragma once

#include<atomic>
#include<cstddef>
#include<string>
#include<vector>

struct SolLunCal;

struct RootTask{
	std::string kind;
	double target;
	double jd_initial;
	double eps_days;
	int max_iter;
};

struct RootCtx{
	SolLunCal*self;
	const std::vector<RootTask>*tasks;
	std::vector<double>*results;
	std::vector<std::string>*errors;
	std::atomic<std::size_t>*cursor;
};

void run_wkr(RootCtx*ctx);
