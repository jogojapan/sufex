cmake_minimum_required (VERSION 2.8)
project (Sufex)

set (Sufex_VERSION_MAJOR 0)
set (Sufex_VERSION_MINOR 1)

set (bin_dir bin)
file (MAKE_DIRECTORY ../${bin_dir})
set (test_bin_dir bin/test)
file (MAKE_DIRECTORY ../${test_bin_dir})

configure_file (
  "${PROJECT_SOURCE_DIR}/SufexConfig.h.in"
  "${PROJECT_BINARY_DIR}/SufexConfig.h"
)

# Removed -Wshadow from the below to avoid a deluge of warnings from
# Boost.
ADD_DEFINITIONS(-std=c++0x -g -O3 -Wall -W -Wpointer-arith -Wcast-qual)

include_directories("${PROJECT_BINARY_DIR}")

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

find_package( Boost ${BOOST_MIN_VERSION} COMPONENTS filesystem system )

if (Boost_FOUND)
   include_directories(${Boost_INCLUDE_DIRS})
   link_directories (${Boost_LIBRARY_DIR})
elseif (NOT Boost_FOUND)
  message (FATAL_ERROR "Fatal error: Boost (version >= ${BOOST_MIN_VERSION}) required.\n")
  message(STATUS "BOOST_ROOT is ${BOOST_ROOT}. Try setting this environment variable to a different path.")
endif ()

add_executable (${bin_dir}/S2SParser s2s/S2SParser.cpp)
add_executable (${bin_dir}/S2SFileParser s2s/S2SFileParser.cpp)
target_link_libraries (${bin_dir}/S2SFileParser ${Boost_LIBRARIES})

enable_testing ()
add_executable (${test_bin_dir}/S2SParserTest s2s/test/S2SParserTest.cpp)
add_test (S2SParserTest ${test_bin_dir}/S2SParserTest)
