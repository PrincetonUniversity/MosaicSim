cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
 
Project(sim-apollo)
 
ADD_EXECUTABLE(sim-apollo sim-apollo.cpp)
