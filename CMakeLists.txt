cmake_minimum_required(VERSION 3.16)

if (NOT DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "You must set VCPKG_ROOT env variable or pass CMAKE_TOOLCHAIN_FILE")
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(storm-tape VERSION 0.1.0)
include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(Boost_NO_WARN_NEW_VERSIONS 1)

# Add all warnings
string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wcast-qual -Wformat=2 -Wundef -Werror=float-equal -Wshadow -Wcast-align -Wunused -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough -Wextra-semi -Woverloaded-virtual -Wnon-virtual-dtor -Wold-style-cast")

# Enable Coverage
option(STORM_ENABLE_COVERAGE "Enable testing w/ coverage")
if (STORM_ENABLE_COVERAGE)
  string(APPEND CMAKE_CXX_FLAGS " -Og -g --coverage")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " --coverage")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " --coverage")
endif()

# Enable Testing
option(STORM_BUILD_TESTING "Enable testing")
if (STORM_BUILD_TESTING)
  string(APPEND CMAKE_CXX_FLAGS " -DENABLE_TESTING=ON")
endif()

# Enable sanitizers
option(STORM_SANITIZE_ADDRESS   "Enable address sanitizer")
option(STORM_SANITIZE_UNDEFINED "Enable undefined-behaviour sanitizer")
option(STORM_SANITIZE_THREAD    "Enable thread sanitizer")

if (STORM_SANITIZE_ADDRESS)
  string(APPEND CMAKE_CXX_FLAGS " -fsanitize=address -fno-omit-frame-pointer")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=address -fno-omit-frame-pointer")
endif()

if (STORM_SANITIZE_UNDEFINED)
  string(APPEND CMAKE_CXX_FLAGS " -fsanitize=undefined -fno-omit-frame-pointer")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=undefined -fno-omit-frame-pointer")
endif()

if (STORM_SANITIZE_THREAD)
  # TODO: add compatibility check with other sanitizers
  string(APPEND CMAKE_CXX_FLAGS " -fsanitize=thread -fno-omit-frame-pointer")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=thread -fno-omit-frame-pointer")
endif()

# Add "format-check" e "format-fix" executables for clang-format
include(cmake/lint-targets.cmake)

find_package(Crow CONFIG REQUIRED)
# fix for "warning: ISO C++11 requires whitespace after the macro name", due to "g++ -D_CROW_ICD-NOTFOUND"
if (NOT _CROW_ICD)
  set_target_properties(Crow::Crow PROPERTIES INTERFACE_COMPILE_DEFINITIONS "")
endif()
find_package(SOCI CONFIG REQUIRED)
find_package(Boost REQUIRED)

add_library(
  libtaperestapi
  OBJECT
  src/archiveinfo_response.cpp
  src/cancel_response.cpp
  src/database.cpp
  src/database_soci.cpp
  src/delete_response.cpp
  src/extended_attributes.cpp
  src/file.cpp
  src/io.cpp
  src/json.cpp
  src/local_storage.cpp
  src/release_response.cpp
  src/requests_with_paths.cpp
  src/routes.cpp
  src/stage_request.cpp
  src/stage_response.cpp
  src/status_response.cpp
  src/takeover_request.cpp
  src/tape_service.cpp
  src/types.cpp
  src/url.cpp
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
target_link_libraries(
  storm-tape 
  PRIVATE
  libtaperestapi 
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
install(TARGETS storm-tape DESTINATION "")
