cmake_minimum_required (VERSION 3.8)

project(DiskBenchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows" AND NOT ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	message( FATAL_ERROR "Compilation system not supported" )
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/Libraries
	${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_HOST_SYSTEM_NAME}
)

add_executable(${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_HOST_SYSTEM_NAME}/SystemFile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_HOST_SYSTEM_NAME}/SystemFile.h
	${CMAKE_CURRENT_SOURCE_DIR}/DiskBenchmark.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DiskBenchmark.h
	${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
)

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	add_definitions(-DUNICODE -D_UNICODE)
else()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Libraries/libaio)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Libraries/libaio aio)
	target_link_libraries(${PROJECT_NAME} PRIVATE rt pthread aio)
	add_compile_options(-O3 -Wno-write-strings)
	#target_link_options(${PROJECT_NAME} PRIVATE -static)
endif()
