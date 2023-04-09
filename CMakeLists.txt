project(playground)
cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

set(FB_ROOT CACHE STRING "/usr/local")

add_compile_options(-g)

list(APPEND CMAKE_PREFIX_PATH 
${FB_ROOT}/folly
${FB_ROOT}/fmt
)

find_package(Folly REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(GTest REQUIRED)


include_directories(${CMAKE_SOURCE_DIR})

file(GLOB playground_srcs playground/*.cpp)
file(GLOB test_srcs test/*.cpp)


add_library(playground ${playground_srcs})
target_link_libraries(playground PUBLIC ${FOLLY_LIBRARIES})

add_executable(test ${test_srcs})
target_link_libraries(test PUBLIC playground GTest::gtest benchmark)
