cmake_minimum_required(VERSION 3.3)
project(hellompi)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)

set(SOURCE_FILES main.c MatrixMultiplication_20170077.c)
add_executable(hellompi ${SOURCE_FILES} MatrixMultiplication_20170077.c)