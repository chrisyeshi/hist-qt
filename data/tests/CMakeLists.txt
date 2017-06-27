cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

file(COPY s3d_run DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# add_executable(input input.cpp)
# target_link_libraries(input histdata)
# add_test(input input)

add_executable(sparsefull sparsefull.cpp)
target_link_libraries(sparsefull histdata)
add_test(sparsefull sparsefull)

add_executable(3dto2d 3dto2d.cpp)
target_link_libraries(3dto2d histdata)
add_test(3dto2d 3dto2d)

add_executable(extent extent.cpp)
target_link_libraries(extent histdata)
add_test(extent extent)

# add_executable(volasgrid volasgrid.cpp)
# target_link_libraries(volasgrid histdata)
# add_test(volasgrid volasgrid)

add_executable(checkrange checkrange.cpp)
target_link_libraries(checkrange histdata)
add_test(checkrange checkrange)

add_executable(merge merge.cpp)
target_link_libraries(merge histdata)
add_test(merge merge)