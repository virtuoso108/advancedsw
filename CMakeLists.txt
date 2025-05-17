cmake_minimum_required(VERSION 3.14)
project(advancedsw)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# 소스 파일 목록 (main.cpp 제외)
file(GLOB APP_SOURCES_NO_MAIN
    src/app/domain/*.cpp
    src/app/application/*.cpp
    src/app/presentation/*.cpp
    src/app/exception/*.cpp
)

# 메인 프로그램
add_executable(app_program src/app/main.cpp ${APP_SOURCES_NO_MAIN})

# 테스트 파일 목록
file(GLOB TEST_SOURCES
    src/test/*.cpp
)

# DVM 테스트 실행 파일
add_executable(dvm_test src/test/dvm_test.cpp ${APP_SOURCES_NO_MAIN})
target_link_libraries(dvm_test GTest::gtest_main GTest::gmock_main)

# Sale 테스트 실행 파일
add_executable(sale_test src/test/sale_test.cpp ${APP_SOURCES_NO_MAIN})
target_link_libraries(sale_test GTest::gtest_main)

# Domain 테스트 실행 파일
add_executable(domain_test src/test/domain_test.cpp ${APP_SOURCES_NO_MAIN})
target_link_libraries(domain_test GTest::gtest_main GTest::gmock_main)

# OtherDVM 테스트 실행 파일
add_executable(otherdvm_test src/test/otherdvm_test.cpp ${APP_SOURCES_NO_MAIN})
target_link_libraries(otherdvm_test GTest::gtest_main GTest::gmock_main)

# Controller 테스트 실행 파일
add_executable(controller_test src/test/controller_test.cpp ${APP_SOURCES_NO_MAIN})
target_link_libraries(controller_test GTest::gtest_main GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(dvm_test)
gtest_discover_tests(sale_test)
gtest_discover_tests(domain_test) 
gtest_discover_tests(otherdvm_test)
gtest_discover_tests(controller_test)