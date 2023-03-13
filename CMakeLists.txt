cmake_minimum_required(VERSION 3.24)
project(TAS C)

set(CMAKE_C_STANDARD 17)

add_executable(TAS main.c varmgr.h varmgr.c)
add_executable(PREPPER prepper.c)
