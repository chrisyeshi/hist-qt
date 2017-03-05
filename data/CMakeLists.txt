cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

add_compile_options(-std=c++11)
enable_testing()
add_subdirectory(tests)

set(SOURCES DataPool.cpp Histogram.cpp)
set(HEADERS DataPool.h   Histogram.h   Extent.h)

add_library(histdata ${SOURCES} ${HEADERS})