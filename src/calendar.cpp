#include "lunar/calendar.hpp"

#include<algorithm>
#include<chrono>
#include<cmath>
#include<cstdlib>
#include<cstring>
#include<filesystem>
#include<fstream>
#include<iomanip>
#include<iostream>
#include<limits>
#include<sstream>
#include<stdexcept>
#include<thread>
#include<vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<windows.h>
#else
#include<unistd.h>
#endif

namespace fs=std::filesystem;

namespace{

constexpr std::size_t kMaxWork=8;

std::string clean_txt(std::string text){
	for(char&c : text){
		if(c=='\t'||c=='\r'||c=='\n'){
			c=' ';
		}
	}
	return text;
}

bool parse_tsv(const std::string&line,std::vector<std::string>&fields,
			   std::size_t min_fields){
	fields.clear();
	std::string current;
	for(char c : line){
		if(c=='\t'){
			fields.push_back(current);
			current.clear();
		}else{
			current.push_back(c);
		}
	}
	fields.push_back(current);
	return fields.size()>=min_fields;
}

std::string quote_arg(const std::string&arg){
	std::string q="\"";
	for(char c : arg){
		if(c=='"'){
			q+="\\\"";
		}else{
			q.push_back(c);
		}
	}
	q.push_back('"');
	return q;
}

int run_wproc(const std::string&exe_path,const std::string&ephem_path,
			  const std::string&input_path,const std::string&out_path){
#ifdef _WIN32
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	std::memset(&si,0,sizeof(si));
	std::memset(&pi,0,sizeof(pi));
	si.cb=sizeof(si);

	std::string cmd=quote_arg(exe_path)+" __root_batch "+quote_arg(ephem_path)+
					" "+quote_arg(input_path)+" "+quote_arg(out_path);
	std::vector<char> cmd_buf(cmd.begin(),cmd.end());
	cmd_buf.push_back('\0');

	BOOL ok=CreateProcessA(exe_path.c_str(),cmd_buf.data(),nullptr,nullptr,
						   FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi);
	if(!ok){
		return -1;
	}

	WaitForSingleObject(pi.hProcess,INFINITE);
	DWORD exit_code=1;
	if(!GetExitCodeProcess(pi.hProcess,&exit_code)){
		exit_code=1;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return static_cast<int>(exit_code);
#else
	std::string cmd=quote_arg(exe_path)+" __root_batch "+quote_arg(ephem_path)+
					" "+quote_arg(input_path)+" "+quote_arg(out_path);
	return std::system(cmd.c_str());
#endif
}

std::string exe_path(){
#ifdef _WIN32
	char buf[4096];
	DWORD len=GetModuleFileNameA(nullptr,buf,static_cast<DWORD>(sizeof(buf)));
	if(len==0||len>=sizeof(buf)){
		throw std::runtime_error("failed to query executable path");
	}
	return std::string(buf,len);
#else
	char buf[4096];
	ssize_t len=readlink("/proc/self/exe",buf,sizeof(buf)-1);
	if(len<=0){
		throw std::runtime_error("failed to query executable path");
	}
	buf[len]='\0';
	return std::string(buf);
#endif
}

}

const std::map<int,std::string> CN_MONTH={
	{1,"正月"},{2,"二月"},{3,"三月"},{4,"四月"}, {5,"五月"},   {6,"六月"},
	{7,"七月"},{8,"八月"},{9,"九月"},{10,"十月"},{11,"十一月"},{12,"腊月"},
};

SolLunCal::SolLunCal(EphRead&reader) : eph(reader),app(reader){}

double SolLunCal::norm_angle(double angle){
	return angle-TWO_PI*std::floor((angle+PI)/TWO_PI);
}

LocalDT SolLunCal::mk_local(int year,int month,int day,int hour,int minute,
							double second){
	return LocalDT::from_loc(year,month,day,hour,minute,second);
}

double SolLunCal::loc2utc(const LocalDT&t){ return t.toUtcJD(); }

LocalDT SolLunCal::utc2loc(double jd_utc){ return LocalDT::fromUtcJD(jd_utc); }

std::string SolLunCal::fmt_time(const LocalDT&t){ return fmt_local(t); }

const std::map<std::string,SolLunCal::STermDef>&SolLunCal::st_defs(){
	static std::map<std::string,STermDef> m={
		{"Z2",{"春分",0.0}},	 {"J3",{"清明",PI/12}},
		{"Z3",{"谷雨",PI/6}},	 {"J4",{"立夏",PI/4}},
		{"Z4",{"小满",PI/3}},	 {"J5",{"芒种",5*PI/12}},
		{"Z5",{"夏至",PI/2}},	 {"J6",{"小暑",7*PI/12}},
		{"Z6",{"大暑",2*PI/3}},	 {"J7",{"立秋",3*PI/4}},
		{"Z7",{"处暑",5*PI/6}},	 {"J8",{"白露",11*PI/12}},
		{"Z8",{"秋分",PI}},		 {"J9",{"寒露",-11*PI/12}},
		{"Z9",{"霜降",-5*PI/6}}, {"J10",{"立冬",-3*PI/4}},
		{"Z10",{"小雪",-2*PI/3}},{"J11",{"大雪",-7*PI/12}},
		{"Z11",{"冬至",-PI/2}},	 {"J12",{"小寒",-5*PI/12}},
		{"Z12",{"大寒",-PI/3}},	 {"J1",{"立春",-PI/4}},
		{"Z1",{"雨水",-PI/6}},	 {"J2",{"惊蛰",-PI/12}},
	};
	return m;
}

const std::map<std::string,int>&SolLunCal::st_init(){
	static std::map<std::string,int> m={
		{"Z11",12},{"J12",1},{"Z12",1},{"J1",2},  {"Z1",2},	 {"J2",3},
		{"Z2",3},  {"J3",4}, {"Z3",4}, {"J4",5},  {"Z4",5},	 {"J5",6},
		{"Z5",6},  {"J6",7}, {"Z6",7}, {"J7",8},  {"Z7",8},	 {"J8",9},
		{"Z8",9},  {"J9",10},{"Z9",10},{"J10",11},{"Z10",11},{"J11",12},
	};
	return m;
}

const std::map<std::string,SolLunCal::LPhaseDef>&SolLunCal::lp_defs(){
	static std::map<std::string,LPhaseDef> m={
		{"new_moon",{"朔",0.0}},
		{"fst_qtr",{"上弦",PI/2}},
		{"full_moon",{"望",PI}},
		{"lst_qtr",{"下弦",-PI/2}},
	};
	return m;
}

const std::map<std::string,double>&SolLunCal::lp_offs(){
	static std::map<std::string,double> m={
		{"new_moon",0.0},
		{"fst_qtr",7.0},
		{"full_moon",15.0},
		{"lst_qtr",22.0},
	};
	return m;
}

double SolLunCal::st_guess(int year,const std::string&code){
	const auto&m=st_init();
	int month=1;
	auto it=m.find(code);
	if(it!=m.end()){
		month=it->second;
	}
	int day=(code=="Z11")?22:15;
	double jd0=greg2jd(year,month,day,0,0,0.0);
	return TimeScale::utc_to_tdb(jd0);
}

double SolLunCal::f_sterm(double jd_tdb,double tgt_lam,double*lam_ptr){
	double lam;
	if(lam_ptr){
		lam=*lam_ptr;
	}else{
		auto s=app.sun_calc(jd_tdb);
		lam=s.first;
	}
	return norm_angle(lam-tgt_lam);
}

double SolLunCal::f_lphase(double jd_tdb,double ph_angle,double*lam_s_ptr,
						   double*lam_m_ptr){
	double lam_s;
	double lam_m;
	if(lam_s_ptr&&lam_m_ptr){
		lam_s=*lam_s_ptr;
		lam_m=*lam_m_ptr;
	}else{
		auto s=app.sun_calc(jd_tdb);
		auto m=app.moon_calc(jd_tdb);
		lam_s=s.first;
		lam_m=m.first;
	}
	return norm_angle(lam_m-lam_s-ph_angle);
}

std::pair<double,double> SolLunCal::val_der(const std::string&kind,
											double jd_tdb,double target){
	if(kind=="solar"){
		auto s=app.sun_calc(jd_tdb);
		double lam=s.first;
		double lam_dot=s.second;
		double f=f_sterm(jd_tdb,target,&lam);
		double fdot=lam_dot;
		return {f,fdot};
	}

	auto s=app.sun_calc(jd_tdb);
	auto m=app.moon_calc(jd_tdb);
	double lam_s=s.first;
	double lam_dot_s=s.second;
	double lam_m=m.first;
	double lam_dot_m=m.second;
	double f=f_lphase(jd_tdb,target,&lam_s,&lam_m);
	double fdot=lam_dot_m-lam_dot_s;
	return {f,fdot};
}

double SolLunCal::newton(const std::string&kind,double jd_initial,double target,
						 double eps_days,int max_iter){
	double jd=jd_initial;
	auto vf=val_der(kind,jd,target);
	double f=vf.first;
	double fdot=vf.second;
	if(std::fabs(f)<1e-12){
		return jd;
	}

	for(int iter=0;iter<max_iter;++iter){
		if(std::fabs(fdot)<1e-12){
			break;
		}
		double delta=f/fdot;
		if(delta>3.0){
			delta=3.0;
		}
		if(delta<-3.0){
			delta=-3.0;
		}
		double jd_new=jd-delta;
		auto vf_new=val_der(kind,jd_new,target);
		double f_new=vf_new.first;
		double fdot_new=vf_new.second;

		int backtracks=0;
		while(std::fabs(f_new)>std::fabs(f)&&std::fabs(delta)>eps_days&&
			  backtracks<20){
			delta*=0.5;
			jd_new=jd-delta;
			vf_new=val_der(kind,jd_new,target);
			f_new=vf_new.first;
			fdot_new=vf_new.second;
			++backtracks;
		}

		if(std::fabs(f_new)>std::fabs(f)&&std::fabs(delta)>eps_days){
			break;
		}

		if(std::fabs(delta)<eps_days||std::fabs(f_new)<1e-12){
			return jd_new;
		}

		jd=jd_new;
		f=f_new;
		fdot=fdot_new;
	}

	auto f_only=[&](double jd_val) -> double{
		return val_der(kind,jd_val,target).first;
	};

	double scan_step=0.5;
	double scan_limit=3.0;
	double f_center=f;
	double jd_center=jd;
	if(std::fabs(f_center)<1e-12){
		return jd_center;
	}

	struct Interval{
		double left;
		double right;
		double f_left;
		double f_right;
	};
	std::vector<Interval> intervals;

	for(int dirSign=-1;dirSign<=1;dirSign+=2){
		double prev_jd=jd_center;
		double prev_f=f_center;
		int steps=static_cast<int>(scan_limit/scan_step);
		for(int i=1;i<=steps;++i){
			double cand_jd=jd_center+dirSign*i*scan_step;
			double cand_f=f_only(cand_jd);
			if(std::fabs(cand_f)<1e-12){
				return cand_jd;
			}
			if(prev_f*cand_f<=0.0){
				double left=std::min(prev_jd,cand_jd);
				double right=std::max(prev_jd,cand_jd);
				double f_left=(left==prev_jd)?prev_f:cand_f;
				double f_right=(right==cand_jd)?cand_f:prev_f;
				intervals.push_back({left,right,f_left,f_right});
				break;
			}
			prev_jd=cand_jd;
			prev_f=cand_f;
		}
	}

	for(auto&iv : intervals){
		double left=iv.left;
		double right=iv.right;
		double f_left=iv.f_left;
		double f_right=iv.f_right;
		if(f_left==0.0){
			return left;
		}
		if(f_right==0.0){
			return right;
		}
		for(int it=0;it<40;++it){
			double mid=0.5*(left+right);
			double f_mid=f_only(mid);
			if(std::fabs(f_mid)<1e-12||(right-left)*0.5<eps_days){
				return mid;
			}
			if(f_left*f_mid<=0.0){
				right=mid;
				f_right=f_mid;
			}else{
				left=mid;
				f_left=f_mid;
			}
		}
	}

	throw std::runtime_error("Newton-Raphson did not converge");
}

std::pair<std::vector<double>,std::vector<std::string>>
SolLunCal::run_roots(const std::vector<RootTask>&tasks){
	std::vector<double> results(tasks.size(),
								std::numeric_limits<double>::quiet_NaN());
	std::vector<std::string> errors(tasks.size());

	if(tasks.empty()){
		return {results,errors};
	}

	auto run_serial=[&](){
		for(std::size_t idx=0;idx<tasks.size();++idx){
			const auto&task=tasks[idx];
			try{
				results[idx]=newton(task.kind,task.jd_initial,task.target,
									task.eps_days,task.max_iter);
			}catch(const std::exception&ex){
				errors[idx]=ex.what();
			}catch(...){
				errors[idx]="unknown error";
			}
		}
	};

	if(tasks.size()==1){
		run_serial();
		return {results,errors};
	}

	try{
		const std::string exe_file=exe_path();
		const std::string ephem_path=fs::absolute(eph.filepath).string();

		unsigned int hc=std::thread::hardware_concurrency();
		std::size_t wk_count=hc==0?4:static_cast<std::size_t>(hc);
		wk_count=std::min<std::size_t>(wk_count,tasks.size());
		wk_count=std::min<std::size_t>(wk_count,kMaxWork);
		if(wk_count==0){
			wk_count=1;
		}
		if(wk_count<=1){
			run_serial();
			return {results,errors};
		}

		std::error_code ec;
		fs::path tmp_root=fs::temp_directory_path(ec);
		if(ec){
			throw std::runtime_error("failed to get temporary directory");
		}
		fs::path tmp_dir=tmp_root/"lunar_rb";
		fs::create_directories(tmp_dir,ec);
		if(ec){
			throw std::runtime_error("failed to create temporary directory");
		}

		struct BatchJob{
			std::vector<std::size_t> task_idx;
			fs::path input_path;
			fs::path out_path;
			int exit_code=-1;
		};

		std::vector<BatchJob> jobs(wk_count);
		for(std::size_t idx=0;idx<tasks.size();++idx){
			jobs[idx%wk_count].task_idx.push_back(idx);
		}

#ifdef _WIN32
		unsigned long pid=static_cast<unsigned long>(GetCurrentProcessId());
#else
		unsigned long pid=static_cast<unsigned long>(getpid());
#endif
		auto now_ticks=static_cast<unsigned long long>(
			std::chrono::steady_clock::now().time_since_epoch().count());

		for(std::size_t i=0;i<jobs.size();++i){
			if(jobs[i].task_idx.empty()){
				continue;
			}
			jobs[i].input_path=tmp_dir/("root_batch_in_"+std::to_string(pid)+
										"_"+std::to_string(now_ticks)+"_"+
										std::to_string(i)+".tsv");
			jobs[i].out_path=tmp_dir/("root_batch_out_"+std::to_string(pid)+"_"+
									  std::to_string(now_ticks)+"_"+
									  std::to_string(i)+".tsv");

			std::ofstream ofs(jobs[i].input_path);
			if(!ofs){
				throw std::runtime_error("failed to write batch task file");
			}
			ofs<<std::setprecision(17);
			for(std::size_t idx : jobs[i].task_idx){
				const auto&task=tasks[idx];
				ofs<<idx<<'\t'<<task.kind<<'\t'<<task.target<<'\t'
				   <<task.jd_initial<<'\t'<<task.eps_days<<'\t'<<task.max_iter
				   <<'\n';
			}
		}

		std::vector<std::thread> launchers;
		for(std::size_t i=0;i<jobs.size();++i){
			if(jobs[i].task_idx.empty()){
				continue;
			}
			launchers.emplace_back([&,i](){
				jobs[i].exit_code=
					run_wproc(exe_file,ephem_path,jobs[i].input_path.string(),
							  jobs[i].out_path.string());
			});
		}
		for(auto&th : launchers){
			th.join();
		}

		std::vector<bool> got_result(tasks.size(),false);
		std::vector<std::string> fields;

		for(std::size_t i=0;i<jobs.size();++i){
			if(jobs[i].task_idx.empty()){
				continue;
			}
			if(jobs[i].exit_code!=0){
				for(std::size_t idx : jobs[i].task_idx){
					errors[idx]="root worker process failed with code "+
								std::to_string(jobs[i].exit_code);
				}
				continue;
			}

			std::ifstream ifs(jobs[i].out_path);
			if(!ifs){
				for(std::size_t idx : jobs[i].task_idx){
					errors[idx]="root worker output missing";
				}
				continue;
			}

			std::string line;
			while(std::getline(ifs,line)){
				if(line.empty()){
					continue;
				}
				if(!parse_tsv(line,fields,3)){
					continue;
				}

				std::size_t idx=0;
				try{
					idx=static_cast<std::size_t>(std::stoull(fields[0]));
				}catch(...){
					continue;
				}
				if(idx>=tasks.size()){
					continue;
				}

				if(fields[1]=="OK"){
					try{
						results[idx]=std::stod(fields[2]);
						got_result[idx]=true;
					}catch(...){
						errors[idx]="failed to parse worker root value";
					}
				}else if(fields[1]=="ERR"){
					errors[idx]=fields[2];
					got_result[idx]=true;
				}
			}

			for(std::size_t idx : jobs[i].task_idx){
				if(!got_result[idx]&&errors[idx].empty()){
					errors[idx]="root worker returned no result";
				}
			}
		}

		for(const auto&job : jobs){
			if(!job.input_path.empty()){
				fs::remove(job.input_path,ec);
			}
			if(!job.out_path.empty()){
				fs::remove(job.out_path,ec);
			}
		}

	}catch(...){
		run_serial();
	}

	return {results,errors};
}

LocalDT SolLunCal::find_st(const std::string&code,int year){
	const auto&defs=st_defs();
	auto it=defs.find(code);
	if(it==defs.end()){
		throw std::runtime_error("Unknown solar term code: "+code);
	}
	double lam_target=it->second.lambda;
	double jd_tdb0=st_guess(year,code);
	double jd_tdb=newton("solar",jd_tdb0,lam_target);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	return utc2loc(jd_utc);
}

LocalDT SolLunCal::find_lp(const std::string&phase_key,double jd_near){
	const auto&defs=lp_defs();
	auto it=defs.find(phase_key);
	if(it==defs.end()){
		throw std::runtime_error("Unknown lunar phase key: "+phase_key);
	}
	double ang=it->second.angle;
	double jd_tdb=newton("lunar",jd_near,ang);
	double jd_utc=TimeScale::tdb_to_utc(jd_tdb);
	return utc2loc(jd_utc);
}

YearResult SolLunCal::compute_year(int year){
	return compute_year(year,&std::cout);
}

YearResult SolLunCal::compute_year(int year,std::ostream*log){
	if(log){
		(*log)<<"正在计算 "<<year<<" 年 ..."<<std::endl;
	}
	YearResult out;
	out.year=year;

	const auto&defs=st_defs();

	std::vector<RootTask> tasks;
	std::vector<TaskMeta> metas;

	auto add_stask=[&](int tgt_year,const std::string&code){
		auto it=defs.find(code);
		if(it==defs.end()){
			throw std::runtime_error("Unknown solar term code: "+code);
		}
		double jd0=st_guess(tgt_year,code);
		tasks.push_back({"solar",it->second.lambda,jd0,1e-8,20});
		metas.push_back({true,code,tgt_year,"",-1});
	};

	add_stask(year-1,"Z11");
	for(const auto&p : defs){
		add_stask(year,p.first);
	}

	double ws_prevt=st_guess(year-1,"Z11");
	double ws_prevu=TimeScale::tdb_to_utc(ws_prevt);
	double jd_anch=TimeScale::utc_to_tdb(ws_prevu-45.0);

	const auto&phase_defs=lp_defs();
	const auto&offsets=lp_offs();

	for(int idx=0;idx<18;++idx){
		double base_jd=jd_anch+idx*SYNODDAY;
		for(const auto&ph : phase_defs){
			const std::string&key=ph.first;
			double ang=ph.second.angle;
			double offset=offsets.at(key);
			double guess=base_jd+offset;
			tasks.push_back({"lunar",ang,guess,1e-8,20});
			metas.push_back({false,"",0,key,idx});
		}
	}

	auto task_out=run_roots(tasks);
	const auto&roots=task_out.first;
	const auto&errors=task_out.second;

	double ws_prevjd=std::numeric_limits<double>::quiet_NaN();
	std::map<std::pair<std::string,int>,double> lun_res;

	for(std::size_t i=0;i<tasks.size();++i){
		if(!errors[i].empty()){
			if(log){
				(*log)<<"  Root task "<<i<<" failed: "<<errors[i]<<std::endl;
			}
			continue;
		}
		const auto&meta=metas[i];
		double root=roots[i];
		if(meta.is_solar){
			if(meta.solar_code=="Z11"&&meta.solar_year==year-1){
				ws_prevjd=root;
			}
			if(meta.solar_year==year){
				auto it=defs.find(meta.solar_code);
				const STermDef&def=it->second;
				double jd_utc=TimeScale::tdb_to_utc(root);
				LocalDT dt=utc2loc(jd_utc);
				out.sol_terms[meta.solar_code]={def.name,dt};
				if(log){
					(*log)<<"  "<<def.name<<": "<<fmt_time(dt)<<std::endl;
				}
			}
		}else{
			lun_res[{meta.lp_key,meta.lp_idx}]=root;
		}
	}

	if(std::isnan(ws_prevjd)){
		throw std::runtime_error("未能求解上一年冬至");
	}

	LocalDT st_local=mk_local(year,1,1);
	LocalDT end_local=mk_local(year+1,1,1);

	int added=0;
	std::set<int> all_idx;
	for(const auto&kv : lun_res){
		all_idx.insert(kv.first.second);
	}

	for(int idx : all_idx){
		auto key_nm=std::make_pair(std::string("new_moon"),idx);
		auto it_nm=lun_res.find(key_nm);
		if(it_nm==lun_res.end()){
			continue;
		}
		double jd_new_tdb=it_nm->second;
		double jd_new_utc=TimeScale::tdb_to_utc(jd_new_tdb);
		LocalDT dt_new=utc2loc(jd_new_utc);
		if(dt_new<st_local){
			continue;
		}
		if(dt_new>=end_local&&added>0){
			break;
		}

		auto it_fq=lun_res.find({"fst_qtr",idx});
		auto it_full=lun_res.find({"full_moon",idx});
		auto it_lq=lun_res.find({"lst_qtr",idx});
		if(it_fq==lun_res.end()||it_full==lun_res.end()||it_lq==lun_res.end()){
			if(log){
				(*log)<<"  朔望月索引 "<<idx<<" 结果不完整"<<std::endl;
			}
			continue;
		}

		LocalDT dt_fq=utc2loc(TimeScale::tdb_to_utc(it_fq->second));
		LocalDT dt_full=utc2loc(TimeScale::tdb_to_utc(it_full->second));
		LocalDT dt_lq=utc2loc(TimeScale::tdb_to_utc(it_lq->second));

		MoonPhMon phset{dt_new,dt_fq,dt_full,dt_lq};
		out.lun_phase.push_back(phset);
		++added;
	}

	if(log){
		(*log)<<"\n二十四节气（UTC+8）\n"
			  <<"--------------------------------------------------------------"
				"-\n";
		const char*order[]={
			"J12","Z12","J1","Z1","J2", "Z2", "J3", "Z3",
			"J4", "Z4", "J5","Z5","J6", "Z6", "J7", "Z7",
			"J8", "Z8", "J9","Z9","J10","Z10","J11","Z11",
		};
		for(const char*code : order){
			auto it=out.sol_terms.find(code);
			if(it!=out.sol_terms.end()){
				(*log)<<std::setw(6)<<std::left<<it->second.name<<" "
					  <<fmt_time(it->second.datetime)<<"\n";
			}
		}

		(*log)<<"\n朔望月相（UTC+8）\n"
			  <<"--------------------------------------------------------------"
				"-\n";
		(*log)<<std::right;
		for(std::size_t i=0;i<out.lun_phase.size();++i){
			const auto&ph=out.lun_phase[i];
			(*log)<<"第"<<std::setw(2)<<std::setfill('0')<<(i+1)
				  <<"月  朔:"<<fmt_time(ph.new_moon)
				  <<"  上弦:"<<fmt_time(ph.fst_qtr)
				  <<"  望:"<<fmt_time(ph.full_moon)
				  <<"  下弦:"<<fmt_time(ph.lst_qtr)<<std::setfill(' ')<<"\n";
		}
		(*log)<<std::left;
	}

	return out;
}

LunCal6::LunCal6(EphRead&reader) : eph(reader),engine(reader){
	Z_CODES={"Z1","Z2","Z3","Z4", "Z5", "Z6",
			 "Z7","Z8","Z9","Z10","Z11","Z12"};
}

LocalDT LunCal6::get_st(const std::string&code,int year){
	auto key=std::make_pair(code,year);
	auto it=st_cache.find(key);
	if(it!=st_cache.end()){
		return it->second;
	}
	LocalDT t=engine.find_st(code,year);
	st_cache[key]=t;
	return t;
}

double LunCal6::to_utcjd(const LocalDT&t){ return SolLunCal::loc2utc(t); }

LocalDT LunCal6::to_local(double jd){ return SolLunCal::utc2loc(jd); }

LocalDT LunCal6::near_nmon(const LocalDT&t_guess){
	double jd_guess=to_utcjd(t_guess);
	double jd_tdb=TimeScale::utc_to_tdb(jd_guess);
	return engine.find_lp("new_moon",jd_tdb);
}

LocalDT LunCal6::prev_nmon(const LocalDT&t){
	LocalDT guess=t.shiftDays(-SYNODDAY);
	LocalDT nm=near_nmon(guess);
	while(nm>=t){
		guess=guess.shiftDays(-SYNODDAY);
		nm=near_nmon(guess);
	}
	while(true){
		LocalDT nxt=next_nmon(nm);
		if(nxt<=t){
			nm=nxt;
		}else{
			break;
		}
	}
	return nm;
}

LocalDT LunCal6::next_nmon(const LocalDT&t_nm){
	double jd_nm=to_utcjd(t_nm);
	double jd_guess=TimeScale::utc_to_tdb(jd_nm+SYNODDAY);
	return engine.find_lp("new_moon",jd_guess);
}

std::tuple<int,int,int> LunCal6::loc_tuple(const LocalDT&t){
	return std::make_tuple(t.year,t.month,t.day);
}

std::vector<std::pair<LocalDT,std::string>> LunCal6::p_terms(int year){
	std::vector<std::pair<LocalDT,std::string>> z_times;
	for(int y=year-1;y<=year+1;++y){
		for(const auto&code : Z_CODES){
			try{
				LocalDT dt=get_st(code,y);
				z_times.push_back({dt,code});
			}catch(...){
			}
		}
	}
	std::sort(z_times.begin(),z_times.end(),[](const auto&a,const auto&b){
		return a.first.toUtcJD()<b.first.toUtcJD();
	});
	return z_times;
}

std::pair<bool,std::set<std::string>>
LunCal6::has_zint(const std::vector<std::pair<LocalDT,std::string>>&z_list,
				  const LocalDT&start,const LocalDT&end){
	std::vector<double> times_jd;
	times_jd.reserve(z_list.size());
	for(const auto&p : z_list){
		times_jd.push_back(SolLunCal::loc2utc(p.first));
	}

	auto start_date=loc_tuple(start);
	int sy=std::get<0>(start_date);
	int sm=std::get<1>(start_date);
	int sd=std::get<2>(start_date);
	LocalDT start_day=SolLunCal::mk_local(sy,sm,sd);
	double start_jd=SolLunCal::loc2utc(start_day);

	auto it=std::lower_bound(times_jd.begin(),times_jd.end(),start_jd);
	std::size_t idx=static_cast<std::size_t>(it-times_jd.begin());

	auto end_date=loc_tuple(end);
	double end_jd=SolLunCal::loc2utc(end);
	std::set<std::string> end_codes;

	while(idx<z_list.size()&&times_jd[idx]<end_jd){
		const LocalDT&dt=z_list[idx].first;
		const std::string&code=z_list[idx].second;
		if(loc_tuple(dt)==end_date){
			end_codes.insert(code);
		}else{
			return {true,end_codes};
		}
		++idx;
	}
	return {false,end_codes};
}

bool LunCal6::has_princ(bool has_z,const std::set<std::string>&){
	return has_z;
}

LocalDT nmon_orbf(LunCal6&calc,const LocalDT&t){
	LocalDT nm=calc.near_nmon(t);
	if(nm>t&&LunCal6::loc_tuple(nm)!=LunCal6::loc_tuple(t)){
		nm=calc.prev_nmon(t);
	}
	return nm;
}

std::vector<LunarMonth> comp_sym(LunCal6&calc,int year){
	LocalDT wy_prev=calc.get_st("Z11",year-1);
	LocalDT wy_curr=calc.get_st("Z11",year);

	LocalDT m_minus_1=nmon_orbf(calc,wy_prev);
	LocalDT m11_bnd=nmon_orbf(calc,wy_curr);
	double bnd_jd=LunCal6::to_utcjd(m11_bnd);

	std::vector<LocalDT> new_moons;
	new_moons.push_back(m_minus_1);

	while(true){
		LocalDT nxt=calc.next_nmon(new_moons.back());
		double nxt_jd=LunCal6::to_utcjd(nxt);
		if(nxt_jd<=bnd_jd+JD_EPSILON){
			if(std::fabs(nxt_jd-bnd_jd)<=JD_EPSILON){
				new_moons.push_back(m11_bnd);
			}else{
				new_moons.push_back(nxt);
			}
		}else{
			break;
		}
	}

	if(new_moons.size()<2){
		throw std::runtime_error("朔月列表构建失败：请检查历表是否完整");
	}

	LocalDT m11=new_moons.back();
	int ly_count=static_cast<int>(new_moons.size())-1;

	double dt_days=m11.toUtcJD()-m_minus_1.toUtcJD();
	int approx_ly=static_cast<int>(std::lround(dt_days/29.53));
	if(std::abs(ly_count-approx_ly)>1){
		throw std::runtime_error("朔数与平均朔望月估计不符，计算可能异常");
	}

	auto z_list=calc.p_terms(year);
	std::vector<bool> mon_prin;
	std::vector<std::pair<LocalDT,LocalDT>> mon_itv;

	for(std::size_t idx=0;idx+1<new_moons.size();++idx){
		LocalDT start=new_moons[idx];
		LocalDT end=new_moons[idx+1];
		auto res=LunCal6::has_zint(z_list,start,end);
		bool has_z=res.first;
		std::set<std::string> end_codes=res.second;
		mon_prin.push_back(LunCal6::has_princ(has_z,end_codes));
		mon_itv.push_back({start,end});
	}

	int leap_index=-1;
	if(ly_count==13){
		for(std::size_t idx=0;idx<mon_prin.size();++idx){
			if(!mon_prin[idx]){
				leap_index=static_cast<int>(idx);
				break;
			}
		}
		if(leap_index<0){
			throw std::runtime_error("闰年未找到闰月：请检查中气计算");
		}
	}else if(ly_count!=12){
		throw std::runtime_error("朔月数量异常："+std::to_string(ly_count));
	}

	std::vector<LunarMonth> months;
	std::vector<int> mon_nums;
	int next_mno=11;

	for(std::size_t idx=0;idx<mon_itv.size();++idx){
		if(leap_index>=0&&static_cast<int>(idx)==leap_index){
			if(idx==0){
				mon_nums.push_back(next_mno);
			}else{
				mon_nums.push_back(mon_nums.back());
			}
		}else{
			mon_nums.push_back(next_mno);
			next_mno=(next_mno==12)?1:(next_mno+1);
		}
	}

	for(std::size_t idx=0;idx<mon_itv.size();++idx){
		const auto&interval=mon_itv[idx];
		int month_no=mon_nums[idx];
		bool is_leap=(leap_index>=0&&static_cast<int>(idx)==leap_index);
		std::string label=(is_leap?"闰":"")+CN_MONTH.at(month_no);
		months.push_back(
			{interval.first,interval.second,month_no,is_leap,label});
	}

	return months;
}

std::vector<LunarMonth> enum_lyr(LunCal6&calc,int year){
	return comp_sym(calc,year);
}

std::vector<LunarMonth> enum_gyr(LunCal6&calc,int year){
	std::vector<LunarMonth> mon_span=comp_sym(calc,year);
	std::vector<LunarMonth> next=comp_sym(calc,year+1);
	mon_span.insert(mon_span.end(),next.begin(),next.end());

	std::map<double,LunarMonth> unique;
	for(const auto&m : mon_span){
		double key=SolLunCal::loc2utc(m.start_dt);
		if(unique.find(key)==unique.end()){
			unique[key]=m;
		}
	}

	std::vector<LunarMonth> selected;
	for(const auto&kv : unique){
		const LunarMonth&m=kv.second;
		if(m.start_dt.year==year){
			selected.push_back(m);
		}
	}
	std::sort(selected.begin(),selected.end(),
			  [](const LunarMonth&a,const LunarMonth&b){
				  return a.start_dt.toUtcJD()<b.start_dt.toUtcJD();
			  });
	return selected;
}

int run_rootw(const std::string&ephem,const std::string&input_path,
			  const std::string&out_path){
	try{
		EphRead eph(ephem);
		SolLunCal solver(eph);

		std::ifstream ifs(input_path);
		if(!ifs){
			throw std::runtime_error("cannot read root batch input file");
		}
		std::ofstream ofs(out_path);
		if(!ofs){
			throw std::runtime_error("cannot write root batch output file");
		}
		ofs<<std::setprecision(17);

		std::string line;
		std::vector<std::string> fields;
		while(std::getline(ifs,line)){
			if(line.empty()){
				continue;
			}
			if(!parse_tsv(line,fields,6)){
				continue;
			}

			std::size_t idx=static_cast<std::size_t>(std::stoull(fields[0]));
			const std::string&kind=fields[1];
			double target=std::stod(fields[2]);
			double jd_initial=std::stod(fields[3]);
			double eps_days=std::stod(fields[4]);
			int max_iter=std::stoi(fields[5]);

			try{
				double root=
					solver.newton(kind,jd_initial,target,eps_days,max_iter);
				ofs<<idx<<'\t'<<"OK"<<'\t'<<root<<'\n';
			}catch(const std::exception&ex){
				ofs<<idx<<'\t'<<"ERR"<<'\t'<<clean_txt(ex.what())<<'\n';
			}catch(...){
				ofs<<idx<<'\t'<<"ERR"<<'\t'<<"unknown error"<<'\n';
			}
		}
		return 0;
	}catch(const std::exception&ex){
		std::cerr<<"root batch worker error: "<<ex.what()<<std::endl;
		return 1;
	}catch(...){
		std::cerr<<"root batch worker unknown error"<<std::endl;
		return 1;
	}
}
