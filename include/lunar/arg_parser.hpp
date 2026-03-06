#pragma once

#include<cstddef>
#include<functional>
#include<stdexcept>
#include<string>
#include<unordered_map>
#include<vector>

namespace lunar{

class ArgParser{
public:
	using Action=std::function<void(const std::vector<std::string>&,
									std::size_t&,const std::string&)>;

	void add(const std::string&opt,Action action){
		actions_[opt]=std::move(action);
	}

	void add_flag(const std::string&opt,std::function<void()> action){
		add(opt,[fn=std::move(action)](const std::vector<std::string>&,
									  std::size_t&,const std::string&){ fn(); });
	}

	void add_value(const std::string&opt,
				   std::function<void(const std::string&)> action){
		add(opt,[fn=std::move(action)](const std::vector<std::string>&args,
									  std::size_t&idx,const std::string&name){
			fn(require_value(args,idx,name));
		});
	}

	bool parse_one(const std::vector<std::string>&args,std::size_t&idx,
				   const std::string&ctx) const{
		const std::string&opt=args[idx];
		auto it=actions_.find(opt);
		if(it==actions_.end()){
			return false;
		}
		it->second(args,idx,opt);
		(void)ctx;
		return true;
	}

	void parse_all(const std::vector<std::string>&args,std::size_t start,
				   const std::string&ctx) const{
		for(std::size_t i=start;i<args.size();++i){
			const std::string&opt=args[i];
			auto it=actions_.find(opt);
			if(it==actions_.end()){
				throw std::invalid_argument("unknown option for "+ctx+": "+opt);
			}
			it->second(args,i,opt);
		}
	}

	static bool is_opt(const std::string&s){
		return !s.empty()&&s[0]=='-';
	}

	static std::string require_value(const std::vector<std::string>&args,
									 std::size_t&idx,const std::string&opt){
		if(idx+1>=args.size()){
			throw std::invalid_argument("missing value for "+opt);
		}
		++idx;
		return args[idx];
	}

private:
	std::unordered_map<std::string,Action> actions_;
};

}
