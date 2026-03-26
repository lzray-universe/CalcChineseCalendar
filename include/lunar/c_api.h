#pragma once

#if defined(_WIN32)
#if defined(LUNAR_BUILD_DLL)
#define LUNAR_API __declspec(dllexport)
#elif defined(LUNAR_USE_DLL)
#define LUNAR_API __declspec(dllimport)
#else
#define LUNAR_API
#endif
#define LUNAR_CALL __cdecl
#else
#define LUNAR_API
#define LUNAR_CALL
#endif

#ifdef __cplusplus
extern "C"{
#endif

LUNAR_API const char*LUNAR_CALL lunar_tool_ver(void);
LUNAR_API const char*LUNAR_CALL lunar_last_error(void);
LUNAR_API void LUNAR_CALL lunar_clear_error(void);

LUNAR_API int LUNAR_CALL lunar_run(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_root_batch(const char*ephem,
										  const char*input_path,
										  const char*out_path);

typedef struct lunar_eot_result{
	double jd_utc;
	double jd_tdb;
	double lon_deg;
	double lon_rad;
	double apparent_solar_time_rad;
	double mean_solar_time_rad;
	double eot_rad;
	double eot_minutes;
	double eot_seconds;
} lunar_eot_result;

LUNAR_API int LUNAR_CALL lunar_calc_eot(const char*ephem,double jd_utc,
										double lon_deg,lunar_eot_result*out);

typedef struct lunar_day_summary{
	int lunar_year;
	int lun_mno;
	int lunar_day;
	int is_leap;
	double ill_pct;
	char phase_name[32];
	char lun_label[96];
} lunar_day_summary;

LUNAR_API int LUNAR_CALL lunar_core_day(const char*ephem,const char*date,
										const char*tz,lunar_day_summary*out);

typedef struct lunar_hli_rules{
	int profile_code;
	int year_boundary;
	int month_boundary;
	int leap_month_mode;
	int day_boundary;
} lunar_hli_rules;

typedef struct lunar_ganzhi_node{
	int index;
	int stem;
	int branch;
	char text[32];
} lunar_ganzhi_node;

typedef struct lunar_ganzhi_summary{
	lunar_ganzhi_node year;
	lunar_ganzhi_node month;
	lunar_ganzhi_node day;
	int rule_profile_code;
	int year_boundary_code;
	int month_boundary_code;
	int leap_month_mode_code;
	int day_boundary_code;
} lunar_ganzhi_summary;

typedef struct lunar_ganzhi_month_summary{
	int year;
	int month;
	int day_count;
	lunar_ganzhi_node years[31];
	lunar_ganzhi_node months[31];
	lunar_ganzhi_node days[31];
	int rule_profile_code;
	int year_boundary_code;
	int month_boundary_code;
	int leap_month_mode_code;
	int day_boundary_code;
} lunar_ganzhi_month_summary;

LUNAR_API void LUNAR_CALL lunar_hli_rules_init(lunar_hli_rules*out);

LUNAR_API int LUNAR_CALL lunar_core_ganzhi(const char*ephem,const char*date,
										   const char*at_time,const char*tz,
										   const lunar_hli_rules*rules,
										   lunar_ganzhi_summary*out);

LUNAR_API int LUNAR_CALL lunar_core_ganzhi_month(
	const char*ephem,int year,int month,const char*at_time,const char*tz,
	const lunar_hli_rules*rules,lunar_ganzhi_month_summary*out);

LUNAR_API int LUNAR_CALL lunar_cmd_month(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_cal(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_year(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_event(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_dl(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_at(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_conv(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_zodiac(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_day(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_mview(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_next(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_range(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_search(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_fest(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_alm(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_info(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_cfg(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_comp(int argc,const char*const*argv);

LUNAR_API int LUNAR_CALL lunar_use_main(void);
LUNAR_API int LUNAR_CALL lunar_use_month(void);
LUNAR_API int LUNAR_CALL lunar_use_cal(void);
LUNAR_API int LUNAR_CALL lunar_use_year(void);
LUNAR_API int LUNAR_CALL lunar_use_event(void);
LUNAR_API int LUNAR_CALL lunar_use_dl(void);
LUNAR_API int LUNAR_CALL lunar_use_at(void);
LUNAR_API int LUNAR_CALL lunar_use_conv(void);
LUNAR_API int LUNAR_CALL lunar_use_zodiac(void);
LUNAR_API int LUNAR_CALL lunar_use_day(void);
LUNAR_API int LUNAR_CALL lunar_use_mview(void);
LUNAR_API int LUNAR_CALL lunar_use_next(void);
LUNAR_API int LUNAR_CALL lunar_use_range(void);
LUNAR_API int LUNAR_CALL lunar_use_search(void);
LUNAR_API int LUNAR_CALL lunar_use_fest(void);
LUNAR_API int LUNAR_CALL lunar_use_alm(void);
LUNAR_API int LUNAR_CALL lunar_use_info(void);
LUNAR_API int LUNAR_CALL lunar_use_cfg(void);
LUNAR_API int LUNAR_CALL lunar_use_comp(void);

#ifdef __cplusplus
}
#endif
