
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_modules")

include(soci_parse_version)
soci_parse_version(
  ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE SOCI_VERSION
)

project(SOCI
  VERSION ${SOCI_VERSION}
  DESCRIPTION "C++ database access library"
  HOMEPAGE_URL "https://soci.sourceforge.net/"
  LANGUAGES C CXX
)

include(soci_utils)

if (NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 14)
  set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(CMakeDependentOption)
include(CheckIPOSupported)

check_ipo_supported(RESULT LTO_AVAILABLE)



option(SOCI_TESTS "Enable building SOCI test cases" ${PROJECT_IS_TOP_LEVEL})
option(WITH_BOOST "Enable Boost support" ON)
cmake_dependent_option(SOCI_LTO "Enable link time optimizations in release builds" ON "LTO_AVAILABLE" OFF)


# Configure LTO for anything but Debug builds (if enabled in the first place)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${SOCI_LTO})
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

if (NOT APPLE)
  # This makes runtime loaders look for library dependencies
  # in the same directory as the library is located in.
  # For details see Craig Scott's CppCon 2019 talk

  # Note: The variable's content is set to $ORIGIN literally,
  # this is NOT a butchered cmake variable expansion
  set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()


set(SOCI_LIB_TYPE "STATIC")


include(soci_compiler_options)
include(soci_install_dirs)

add_subdirectory(src)

include(soci_compat)


# add_executable(testsqlite test.cpp)
# target_include_directories(testsqlite PRIVATE include)
# target_link_libraries(testsqlite SOCI::soci)