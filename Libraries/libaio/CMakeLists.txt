cmake_minimum_required(VERSION 3.14)

project(aio LANGUAGES C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "*.h" "*.c")
add_library(${PROJECT_NAME} STATIC ${SOURCES})
