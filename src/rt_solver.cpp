#include "lunar/rt_solver.hpp"

#include "lunar/calendar.hpp"

#include<exception>

void run_wkr(RootCtx*ctx){
	std::size_t idx;
	while((idx=ctx->cursor->fetch_add(1))<ctx->tasks->size()){
		const auto&task=ctx->tasks->at(idx);
		try{
			double root=ctx->self->newton(task.kind,task.jd_initial,task.target,
										  task.eps_days,task.max_iter);
			ctx->results->at(idx)=root;
		}catch(const std::exception&ex){
			ctx->errors->at(idx)=ex.what();
		}catch(...){
			ctx->errors->at(idx)="unknown error";
		}
	}
}
