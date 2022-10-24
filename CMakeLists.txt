cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

project(storm-tape VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(Boost_NO_WARN_NEW_VERSIONS 1)


# compila sempre con i warning abilitati
string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")

# abilita testing
string(APPEND CMAKE_CXX_FLAGS " -DENABLE_TESTING")

# abilita l'address sanitizer in debug mode
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address -fno-omit-frame-pointer")
string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " -fsanitize=address -fno-omit-frame-pointer")

#include(CTest)

find_package(Crow CONFIG REQUIRED)
find_package(SOCI CONFIG REQUIRED)
find_package(Boost REQUIRED)

add_library(
  libtaperestapi
  OBJECT
  src/json.cpp
  src/routes.cpp
  src/stage_request.cpp
  src/tape_service.cpp
  src/stage_response.cpp
  src/status_response.cpp
  src/cancel_response.cpp
  src/delete_response.cpp
  src/release_response.cpp
  src/archive_response.cpp
  src/io.cpp
)

target_link_libraries(
  libtaperestapi
  PUBLIC
  Crow::Crow 
  SOCI::soci_sqlite3_static 
  SOCI::soci_core_static 
  SOCI::soci_empty_static 
  Boost::boost
)

add_executable(storm-tape src/main.cpp)

target_include_directories(
  storm-tape 
  PRIVATE 
  ${CROW_INCLUDE_DIRS} 
  ${MYSQL_INCLUDE_DIR})

target_link_libraries(
  storm-tape 
  PRIVATE
  libtaperestapi 
  )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
install(TARGETS storm-tape DESTINATION "")
