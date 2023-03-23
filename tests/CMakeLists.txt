
add_executable(all.t all.t.cpp configuration.t.cpp)
target_include_directories(all.t PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(all.t PRIVATE libtaperestapi)
add_test(NAME all.t COMMAND all.t)
