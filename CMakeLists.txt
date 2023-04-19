project(playground)
cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

set(FB_ROOT CACHE STRING "/usr/local")

add_compile_options(-g)

macro(set_alternate_linker linker)
  find_program(LINKER_EXECUTABLE ld.${USE_ALTERNATE_LINKER} ${USE_ALTERNATE_LINKER})
  if(LINKER_EXECUTABLE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 12.0.0)
      add_link_options("-ld-path=${USE_ALTERNATE_LINKER}")
    else()
      add_link_options("-fuse-ld=${USE_ALTERNATE_LINKER}")
    endif()
  else()
    set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker" FORCE)
  endif()
endmacro()

set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker. Leave empty for system default; alternatives are 'gold', 'lld', 'bfd', 'mold'")
if(NOT "${USE_ALTERNATE_LINKER}" STREQUAL "")
  set_alternate_linker(${USE_ALTERNATE_LINKER})
endif()

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
file(GLOB test_srcs test/*.cpp
test/taskflow/*.cpp
test/executor/*.cpp
)


add_library(playground ${playground_srcs})
target_link_libraries(playground PUBLIC ${FOLLY_LIBRARIES})

add_executable(test ${test_srcs})
target_link_libraries(test PUBLIC playground GTest::gtest benchmark)
