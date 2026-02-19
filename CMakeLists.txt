cmake_minimum_required(VERSION 3.20)

project(lunar LANGUAGES C CXX)

option(LUNAR_USE_ERFA "Build with ERFA support" OFF)

set(CMAKE_CXX_STANDARD 17)
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
    src/spc_ephem.cpp
    src/app_long.cpp
    src/rt_solver.cpp
    src/calendar.cpp
    src/json.cpp
    src/js_writer.cpp
    src/ics.cpp
    src/cli.cpp
    src/query.cpp
    src/interact.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/dependent/cspice/include
    )

    if(MSVC)
        target_compile_options(${tgt} PRIVATE /utf-8)
    endif()
endforeach()

if(LUNAR_USE_ERFA)
    file(GLOB ERFA_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/dependent/erfa/src/*.c"
    )
    list(REMOVE_ITEM ERFA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/dependent/erfa/src/t_erfa_c.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/dependent/erfa/src/t_erfa_c_extra.c"
    )
    add_library(erfa STATIC ${ERFA_SOURCES})
    target_include_directories(erfa PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/dependent/erfa/src
    )
    target_compile_definitions(erfa PRIVATE
        PACKAGE_VERSION=\"2.0.1\"
        PACKAGE_VERSION_MAJOR=2
        PACKAGE_VERSION_MINOR=0
        PACKAGE_VERSION_MICRO=1
        SOFA_VERSION=\"20231011\"
    )
    target_link_libraries(lunar PRIVATE erfa)
    target_link_libraries(lunar_dll PRIVATE erfa)
    target_compile_definitions(lunar PRIVATE USE_ERFA)
    target_compile_definitions(lunar_dll PRIVATE USE_ERFA)
endif()

target_link_libraries(lunar PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dependent/cspice/lib/cspice.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/dependent/cspice/lib/csupport.lib
)

target_link_libraries(lunar_dll PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dependent/cspice/lib/cspice.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/dependent/cspice/lib/csupport.lib
)
