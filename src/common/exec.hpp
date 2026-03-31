#pragma once

#include<algorithm>
#include<cstddef>
#include<exception>
#include<utility>

#ifndef LUNAR_ENABLE_THREADS
#define LUNAR_ENABLE_THREADS 0
#endif

#if LUNAR_ENABLE_THREADS
#include<atomic>
#include<thread>
#include<vector>
#endif

namespace lunar::exec{

inline int&depth_slot(){
	static thread_local int depth=0;
	return depth;
}

struct Scope{
	Scope(){ ++depth_slot(); }
	~Scope(){ --depth_slot(); }
};

inline bool enabled(){
#if LUNAR_ENABLE_THREADS
	return true;
#else
	return false;
#endif
}

inline std::size_t hw_jobs(){
#if LUNAR_ENABLE_THREADS
	const unsigned jobs=std::thread::hardware_concurrency();
	return jobs==0?1u:static_cast<std::size_t>(jobs);
#else
	return 1u;
#endif
}

inline std::size_t pick_jobs(std::size_t work_items,std::size_t req_jobs=0){
	if(!enabled()||work_items<4||depth_slot()>0){
		return 1;
	}
	std::size_t jobs=(req_jobs>0)?req_jobs:hw_jobs();
	if(jobs<1){
		jobs=1;
	}
	return std::min(jobs,work_items);
}

template<class MakeCtx,class Run>
void for_each_index(std::size_t work_items,std::size_t req_jobs,MakeCtx&&make_ctx,
					Run&&run){
	const std::size_t jobs=pick_jobs(work_items,req_jobs);
#if LUNAR_ENABLE_THREADS
	if(jobs>1){
		const std::size_t chunk=
			std::max<std::size_t>(1,work_items/(jobs*4));
		std::atomic<std::size_t> next{0};
		std::vector<std::thread> workers;
		std::vector<std::exception_ptr> errors(jobs);
		workers.reserve(jobs);
		for(std::size_t w=0;w<jobs;++w){
			workers.emplace_back([&,w,ctx=make_ctx()]() mutable {
				try{
					Scope scope;
					for(;;){
						std::size_t begin=
							next.fetch_add(chunk,std::memory_order_relaxed);
						if(begin>=work_items){
							break;
						}
						const std::size_t end=
							std::min(work_items,begin+chunk);
						for(std::size_t idx=begin;idx<end;++idx){
							run(ctx,idx);
						}
					}
				}catch(...){
					errors[w]=std::current_exception();
				}
			});
		}
		for(auto&worker : workers){
			worker.join();
		}
		for(const auto&err : errors){
			if(err){
				std::rethrow_exception(err);
			}
		}
		return;
	}
#endif
	auto ctx=make_ctx();
	Scope scope;
	for(std::size_t idx=0;idx<work_items;++idx){
		run(ctx,idx);
	}
}

template<class Run>
void for_each_index(std::size_t work_items,std::size_t req_jobs,Run&&run){
	struct Empty{};
	for_each_index(
		work_items,req_jobs,
		[](){ return Empty{}; },
		[&](Empty&,std::size_t idx){ run(idx); });
}

}
