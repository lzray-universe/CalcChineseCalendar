cmake_minimum_required(VERSION 3.20)

project(lunar LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LUNAR_ENABLE_DIMENSION_TYPES "Enable compile-time tagged physical vectors" ON)
option(LUNAR_ENABLE_SERIES_FALLBACK "Enable VSOP87A/ELPMPP02 fallback when BSP is unavailable" ON)
option(LUNAR_BUILD_TESTS "Build GoogleTest-based test targets" ${BUILD_TESTING})

set(LUNAR_SERIES_SOURCES
    vsop87a/vsop87a.cpp
    vsop87a/vsop87a_ear.cpp
    vsop87a/vsop87a_emb.cpp
    vsop87a/vsop87a_jup.cpp
    vsop87a/vsop87a_mar.cpp
    vsop87a/vsop87a_mer.cpp
    vsop87a/vsop87a_nep.cpp
    vsop87a/vsop87a_sat.cpp
    vsop87a/vsop87a_ura.cpp
    vsop87a/vsop87a_ven.cpp
    elpmpp02/elpmpp02.cpp
    elpmpp02/elpmpp02_data_main_distance.cpp
    elpmpp02/elpmpp02_data_main_latitude.cpp
    elpmpp02/elpmpp02_data_main_longitude.cpp
    elpmpp02/elpmpp02_data_pert_distance_t0.cpp
    elpmpp02/elpmpp02_data_pert_distance_t1.cpp
    elpmpp02/elpmpp02_data_pert_distance_t2.cpp
    elpmpp02/elpmpp02_data_pert_distance_t3.cpp
    elpmpp02/elpmpp02_data_pert_latitude_t0.cpp
    elpmpp02/elpmpp02_data_pert_latitude_t1.cpp
    elpmpp02/elpmpp02_data_pert_latitude_t2.cpp
    elpmpp02/elpmpp02_data_pert_latitude_t3.cpp
    elpmpp02/elpmpp02_data_pert_longitude_t0.cpp
    elpmpp02/elpmpp02_data_pert_longitude_t1.cpp
    elpmpp02/elpmpp02_data_pert_longitude_t2.cpp
    elpmpp02/elpmpp02_data_pert_longitude_t3.cpp
)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(LUNAR_CORE_SOURCES
    src/math.cpp
    src/format.cpp
    src/time_scale.cpp
    src/frames.cpp
    src/precnut_core.cpp
    src/erfa.cpp
    src/spc_ephem.cpp
    src/app_long.cpp
    src/rt_solver.cpp
    src/calendar.cpp
    src/almanac.cpp
    src/lunar_eclipse.cpp
    src/solar_eclipse.cpp
    src/star_calc.cpp
    src/stars/stars.cpp
    src/json.cpp
    src/js_writer.cpp
    src/ics.cpp
    src/i18n/lang.cpp
    src/i18n/catalog_event.cpp
    src/i18n/catalog_astro.cpp
    src/i18n/catalog_almanac.cpp
    src/i18n/catalog_interact.cpp
    src/cli.cpp
    src/cli_common.cpp
    src/query.cpp
    src/interact.cpp
    src/global_context.cpp
    src/download.cpp
    src/entry.cpp
)

if(LUNAR_ENABLE_SERIES_FALLBACK)
    list(APPEND LUNAR_CORE_SOURCES
        ${LUNAR_SERIES_SOURCES}
    )
endif()

add_library(lunar_core STATIC
    ${LUNAR_CORE_SOURCES}
)

set_target_properties(lunar_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(lunar_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(lunar_core PUBLIC
    LUNAR_ENABLE_DIMENSION_TYPES=$<BOOL:${LUNAR_ENABLE_DIMENSION_TYPES}>
    LUNAR_ENABLE_SERIES_FALLBACK=$<BOOL:${LUNAR_ENABLE_SERIES_FALLBACK}>
)
if(MSVC)
    target_compile_options(lunar_core PRIVATE /utf-8)
endif()

add_executable(lunar
    src/main.cpp
)
target_link_libraries(lunar PRIVATE lunar_core)
target_include_directories(lunar PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
if(MSVC)
    target_compile_options(lunar PRIVATE /utf-8)
endif()

add_library(lunar_dll SHARED
    src/c_api.cpp
)
target_link_libraries(lunar_dll PRIVATE lunar_core)
target_include_directories(lunar_dll PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
target_compile_definitions(lunar_dll PRIVATE LUNAR_BUILD_DLL)
set_target_properties(lunar_dll PROPERTIES
    OUTPUT_NAME lunar
)
if(MSVC)
    target_compile_options(lunar_dll PRIVATE /utf-8)
endif()

if(LUNAR_BUILD_TESTS)
    add_subdirectory(tests)
endif()
