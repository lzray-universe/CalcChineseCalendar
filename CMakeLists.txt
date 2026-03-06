cmake_minimum_required(VERSION 3.20)

project(lunar LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    src/cli.cpp
    src/cli_common.cpp
    src/query.cpp
    src/interact.cpp
    src/global_context.cpp
    src/download.cpp
    src/entry.cpp
)

add_executable(lunar
    src/main.cpp
    ${LUNAR_CORE_SOURCES}
)

add_library(lunar_dll SHARED
    src/c_api.cpp
    ${LUNAR_CORE_SOURCES}
)

set_target_properties(lunar_dll PROPERTIES
    OUTPUT_NAME lunar
)
target_compile_definitions(lunar_dll PRIVATE LUNAR_BUILD_DLL)

foreach(tgt lunar lunar_dll)
    target_include_directories(${tgt} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    if(MSVC)
        target_compile_options(${tgt} PRIVATE /utf-8)
    endif()
endforeach()
