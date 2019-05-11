cmake_minimum_required(VERSION 3.13)
project(ex4 C)

set(CMAKE_C_STANDARD 99)

add_executable(ex4 threadpool.c threadPool.h osqueue.c osqueue.h test.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")