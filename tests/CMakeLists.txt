if(NOT TARGET GTest::gtest)
    find_package(GTest CONFIG QUIET)
endif()

if(NOT TARGET GTest::gtest)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(lunar_tests
    ${PROJECT_SOURCE_DIR}/src/c_api.cpp
    test_common.cpp
    test_core_cli.cpp
    test_eclipse.cpp
    test_solar_zodiac.cpp
    test_almanac_i18n.cpp
    test_interact.cpp
)
lunar_enable_runtime_optimizations(lunar_tests)

target_link_libraries(lunar_tests PRIVATE
    lunar_core
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(lunar_tests PRIVATE /utf-8)
endif()

gtest_discover_tests(lunar_tests
    DISCOVERY_MODE PRE_TEST
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
