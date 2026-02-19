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

LUNAR_API int LUNAR_CALL lunar_cmd_month(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_cal(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_year(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_event(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_dl(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_at(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_conv(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_day(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_mview(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_next(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_range(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_search(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_fest(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_alm(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_info(int argc,const char*const*argv);
LUNAR_API int LUNAR_CALL lunar_cmd_test(int argc,const char*const*argv);
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
LUNAR_API int LUNAR_CALL lunar_use_day(void);
LUNAR_API int LUNAR_CALL lunar_use_mview(void);
LUNAR_API int LUNAR_CALL lunar_use_next(void);
LUNAR_API int LUNAR_CALL lunar_use_range(void);
LUNAR_API int LUNAR_CALL lunar_use_search(void);
LUNAR_API int LUNAR_CALL lunar_use_fest(void);
LUNAR_API int LUNAR_CALL lunar_use_alm(void);
LUNAR_API int LUNAR_CALL lunar_use_info(void);
LUNAR_API int LUNAR_CALL lunar_use_test(void);
LUNAR_API int LUNAR_CALL lunar_use_cfg(void);
LUNAR_API int LUNAR_CALL lunar_use_comp(void);

#ifdef __cplusplus
}
#endif
