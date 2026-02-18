#pragma once

#include<iosfwd>
#include<map>
#include<set>
#include<string>
#include<tuple>
#include<utility>
#include<vector>

#include "lunar/app_long.hpp"
#include "lunar/rt_solver.hpp"
#include "lunar/time_scale.hpp"

struct SolarTerm{
	std::string name;
	LocalDT datetime;
};

struct MoonPhMon{
	LocalDT new_moon;
	LocalDT fst_qtr;
	LocalDT full_moon;
	LocalDT lst_qtr;
};

struct YearResult{
	int year;
	std::map<std::string,SolarTerm> sol_terms;
	std::vector<MoonPhMon> lun_phase;
};

struct SolLunCal{
	struct STermDef{
		std::string name;
		double lambda;
	};
	struct LPhaseDef{
		std::string name;
		double angle;
	};
	struct TaskMeta{
		bool is_solar;
		std::string solar_code;
		int solar_year;
		std::string lp_key;
		int lp_idx;
	};

	EphRead&eph;
	AppLon app;

	explicit SolLunCal(EphRead&reader);

	static double norm_angle(double angle);

	static LocalDT mk_local(int year,int month,int day,int hour=0,int minute=0,
							double second=0.0);

	static double loc2utc(const LocalDT&t);

	static LocalDT utc2loc(double jd_utc);

	static std::string fmt_time(const LocalDT&t);

	static const std::map<std::string,STermDef>&st_defs();

	static const std::map<std::string,int>&st_init();

	static const std::map<std::string,LPhaseDef>&lp_defs();

	static const std::map<std::string,double>&lp_offs();

	static double st_guess(int year,const std::string&code);

	double f_sterm(double jd_tdb,double tgt_lam,double*lam_ptr=nullptr);

	double f_lphase(double jd_tdb,double ph_angle,double*lam_s_ptr=nullptr,
					double*lam_m_ptr=nullptr);

	std::pair<double,double> val_der(const std::string&kind,double jd_tdb,
									 double target);

	double newton(const std::string&kind,double jd_initial,double target,
				  double eps_days=1e-8,int max_iter=20);

	std::pair<std::vector<double>,std::vector<std::string>>
	run_roots(const std::vector<RootTask>&tasks);

	LocalDT find_st(const std::string&code,int year);

	LocalDT find_lp(const std::string&phase_key,double jd_near);

	YearResult compute_year(int year);
	YearResult compute_year(int year,std::ostream*log);
};

struct LunarMonth{
	LocalDT start_dt;
	LocalDT end_dt;
	int month_no;
	bool is_leap;
	std::string label;
};

extern const std::map<int,std::string> CN_MONTH;

struct LunCal6{
	EphRead&eph;
	SolLunCal engine;
	std::map<std::pair<std::string,int>,LocalDT> st_cache;

	std::vector<std::string> Z_CODES;

	explicit LunCal6(EphRead&reader);

	LocalDT get_st(const std::string&code,int year);

	static double to_utcjd(const LocalDT&t);

	static LocalDT to_local(double jd);

	LocalDT near_nmon(const LocalDT&t_guess);

	LocalDT prev_nmon(const LocalDT&t);

	LocalDT next_nmon(const LocalDT&t_nm);

	static std::tuple<int,int,int> loc_tuple(const LocalDT&t);

	std::vector<std::pair<LocalDT,std::string>> p_terms(int year);

	static std::pair<bool,std::set<std::string>>
	has_zint(const std::vector<std::pair<LocalDT,std::string>>&z_list,
			 const LocalDT&start,const LocalDT&end);

	static bool has_princ(bool has_z,const std::set<std::string>&end_codes);
};

LocalDT nmon_orbf(LunCal6&calc,const LocalDT&t);

std::vector<LunarMonth> comp_sym(LunCal6&calc,int year);

std::vector<LunarMonth> enum_lyr(LunCal6&calc,int year);

std::vector<LunarMonth> enum_gyr(LunCal6&calc,int year);

int run_rootw(const std::string&ephem,const std::string&input_path,
			  const std::string&out_path);
