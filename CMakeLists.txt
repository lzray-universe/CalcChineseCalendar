cmake_minimum_required(VERSION 3.20)

project(lunar LANGUAGES C CXX)

include(CTest)
include(CheckIPOSupported)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LUNAR_ENABLE_DIMENSION_TYPES "Enable compile-time tagged physical vectors" ON)
option(LUNAR_ENABLE_SERIES_FALLBACK "Enable VSOP87A/ELPMPP02 fallback when BSP is unavailable" ON)
option(LUNAR_BUILD_TESTS "Build GoogleTest-based test targets" ${BUILD_TESTING})
option(LUNAR_ENABLE_IPO "Enable link-time/interprocedural optimization for release-like builds" ON)
option(LUNAR_ENABLE_FAST_MATH "Enable faster but less strict floating-point optimizations" OFF)

if(LUNAR_ENABLE_IPO AND NOT EMSCRIPTEN)
    check_ipo_supported(RESULT LUNAR_IPO_SUPPORTED OUTPUT LUNAR_IPO_OUTPUT LANGUAGES C CXX)
    if(LUNAR_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    else()
        message(STATUS "IPO/LTO disabled: ${LUNAR_IPO_OUTPUT}")
    endif()
endif()

function(lunar_enable_runtime_optimizations target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Release>:/Gw>
            $<$<CONFIG:RelWithDebInfo>:/Gw>
            $<$<CONFIG:RelWithDebInfo>:/Ob2>
        )
    elseif(NOT EMSCRIPTEN)
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:RelWithDebInfo>:-O3>
        )
    endif()

    if(LUNAR_ENABLE_FAST_MATH AND NOT EMSCRIPTEN)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE
                $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/fp:fast>
            )
        else()
            target_compile_options(${target_name} PRIVATE
                $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-ffast-math>
            )
        endif()
    endif()
endfunction()

if(EMSCRIPTEN)
    set(LUNAR_EMSCRIPTEN_COMPILE_OPTIONS
        -fexceptions
    )
    set(LUNAR_EMSCRIPTEN_LINK_OPTIONS
        -fexceptions
        -lnodefs.js
        -lworkerfs.js
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sFORCE_FILESYSTEM=1"
        "SHELL:-sEXIT_RUNTIME=1"
        "SHELL:-sENVIRONMENT=node,web,worker"
        "SHELL:-sEXPORTED_RUNTIME_METHODS=['FS','callMain']"
        "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/src/emscripten/lunar_pre.js"
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/emscripten/lunar_worker.js
        ${CMAKE_CURRENT_BINARY_DIR}/lunar_worker.js
        COPYONLY
    )
endif()

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
lunar_enable_runtime_optimizations(lunar_core)

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
if(EMSCRIPTEN)
    target_compile_options(lunar_core PUBLIC
        ${LUNAR_EMSCRIPTEN_COMPILE_OPTIONS}
    )
endif()
if(MSVC)
    target_compile_options(lunar_core PRIVATE /utf-8)
endif()

add_executable(lunar
    src/main.cpp
)
lunar_enable_runtime_optimizations(lunar)
target_link_libraries(lunar PRIVATE lunar_core)
target_include_directories(lunar PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
if(MSVC)
    target_compile_options(lunar PRIVATE /utf-8)
endif()
if(EMSCRIPTEN)
    target_link_options(lunar PRIVATE
        ${LUNAR_EMSCRIPTEN_LINK_OPTIONS}
    )
endif()

if(EMSCRIPTEN)
    set(LUNAR_DLL_LIBRARY_TYPE STATIC)
else()
    set(LUNAR_DLL_LIBRARY_TYPE SHARED)
endif()

add_library(lunar_dll ${LUNAR_DLL_LIBRARY_TYPE}
    src/c_api.cpp
)
lunar_enable_runtime_optimizations(lunar_dll)
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
