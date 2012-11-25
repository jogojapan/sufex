cmake_minimum_required (VERSION 2.8)
project (Sufex)

set (Sufex_VERSION_MAJOR 0)
set (Sufex_VERSION_MINOR 1)

set (bin_dir bin)
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${bin_dir})
set (test_bin_dir bin/test)
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${test_bin_dir})

configure_file (
  "${PROJECT_SOURCE_DIR}/SufexConfig.h.in"
  "${PROJECT_BINARY_DIR}/SufexConfig.h"
)

# Removed -Wshadow from the below to avoid a deluge of warnings from
# Boost.
ADD_DEFINITIONS(-std=c++11 -g -O3 -pthread -Wall -W -Wpointer-arith -Wcast-qual -Wcast-align -D_GLIBCXX_USE_NANOSLEEP)

include_directories("${PROJECT_BINARY_DIR}")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-pthread -rdynamic")

set(Boost_USE_STATIC_LIBS      ON)
set(Boost_USE_MULTITHREADED    ON)
set(Boost_USE_STATIC_RUNTIME   OFF)
set(BOOST_MIN_VERSION          1.45.0)
set(Boost_DEBUG                TRUE)

if (NOT $ENV{BOOST_PATH} STREQUAL "")
  message (STATUS "Found BOOST_PATH as $ENV{BOOST_PATH}")
  set (BOOST_ROOT $ENV{BOOST_PATH})
  set (Boost_NO_SYSTEM_PATHS TRUE)
endif ()

find_package( Boost ${BOOST_MIN_VERSION} COMPONENTS filesystem system chrono)

if (Boost_FOUND)
   include_directories(${Boost_INCLUDE_DIRS})
   link_directories (${Boost_LIBRARY_DIR})
elseif (NOT Boost_FOUND)
  message (FATAL_ERROR "Fatal error: Boost (version >= ${BOOST_MIN_VERSION}) required.\n")
  message(STATUS "BOOST_ROOT is ${BOOST_ROOT}. Try setting this environment variable to a different path.")
endif ()

include("cmake/FindGlog.cmake")
if (GLOG_FOUND)
   include_directories(${GLOG_INCLUDE_DIRS})
   link_directories (${GLOG_LIBRARY_DIR})
elseif (NOT GLOG_FOUND)
  message (FATAL_ERROR "Fatal error: google-glog required, but not found.\n")
  message (STATUS "GLOG_PATH is $ENV{GLOG_PATH}. Try setting this environment variable to a different path.")
endif ()

add_executable (${bin_dir}/S2SParser s2s/S2SParser.cpp)
add_executable (${bin_dir}/S2SFileParser s2s/S2SFileParser.cpp)
add_executable (${bin_dir}/rulex rulex/rulex.cpp)
target_link_libraries (${bin_dir}/S2SFileParser ${Boost_LIBRARIES})

add_executable (${bin_dir}/combined_clock util/app/combined_clock.cpp)
target_link_libraries (${bin_dir}/combined_clock ${Boost_LIBRARIES} ${GLOG_LIBRARY})

enable_testing ()
add_executable (${test_bin_dir}/S2SParserTest s2s/test/S2SParserTest.cpp)
add_test (S2SParserTest ${test_bin_dir}/S2SParserTest)

add_executable (${test_bin_dir}/SuxBuilderTrigramTest sux/test/SuxBuilderTrigramTest.cpp)
target_link_libraries (${test_bin_dir}/SuxBuilderTrigramTest ${Boost_LIBRARIES} ${GLOG_LIBRARY})
add_test (SuxBuilderTrigramTest ${test_bin_dir}/SuxBuilderTrigramTest)
