cmake_minimum_required(VERSION 3.15)
project(ParallelAss1 C)

set(CMAKE_C_STANDARD 99)

add_executable(ParallelAss1 main.c)
target_link_libraries(ParallelAss1 m)