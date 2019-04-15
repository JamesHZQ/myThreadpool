cmake_minimum_required(VERSION 3.10)
project(myThreadpool)

set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(usr/local/include)
link_directories(/usr/local/lib)

add_executable(threadpool_test threadpoolTest.cpp)

target_link_libraries(threadpool_test pthread)
