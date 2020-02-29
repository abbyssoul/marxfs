cmake_minimum_required(VERSION 3.12)
project(marxfs)
set(PROJECT_DESCRIPTION "9p archive filesystem")

option(COVERAGE "Generate coverage data" OFF)
option(SANITIZE "Enable 'sanitize' compiler flag" OFF)
option(PROFILE "Enable profile information" OFF)


# Include common compile flag
include(cmake/compile_flags.cmake)
include(GNUInstallDirs)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# ---------------------------------
# Build project dependencies
# ---------------------------------
add_subdirectory(src)
add_subdirectory(test EXCLUDE_FROM_ALL)

# ---------------------------------
# Show build configuration status:
# ---------------------------------
message(STATUS, "BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS, "CXXFLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS, "SANITIZE: ${SANITIZE}")
message(STATUS, "COVERAGE: ${COVERAGE}")
