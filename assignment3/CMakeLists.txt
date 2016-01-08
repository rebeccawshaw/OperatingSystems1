cmake_minimum_required(VERSION 2.8)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -g")
add_executable (primescanner Condition.cxx primescanner.cxx  Tasks.cxx )
target_link_libraries (primescanner boost_program_options pthread)