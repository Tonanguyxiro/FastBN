cmake_minimum_required(VERSION 3.10)
project(BayesianNetwork)

set(CMAKE_CXX_STANDARD 14)


file(GLOB SRC ./*.cpp)

add_library(lib_src ${SRC}) # For Google Test
target_link_libraries(lib_src tinyxml2 ARFF)