cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

project(storm-tape VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(SOCI CONFIG REQUIRED)
find_package(Boost REQUIRED)

add_executable(storm-tape main.cpp json.cpp)
target_include_directories(storm-tape PRIVATE ${CROW_INCLUDE_DIRS})
target_link_libraries(storm-tape PRIVATE Crow::Crow SOCI::soci_sqlite3_static SOCI::soci_core_static SOCI::soci_empty_static Boost::boost)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
