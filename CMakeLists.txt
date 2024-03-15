cmake_minimum_required (VERSION 3.8)

project(DiskBenchmark LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	add_definitions(-DUNICODE -D_UNICODE)
elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Libraries/libaio)
	file(GLOB LIB_SOURCES "Libraries/libaio/*.c")
	add_compile_options(-O3 -Wno-write-strings)
	#target_link_options(${PROJECT_NAME} PRIVATE -static)
else()
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
	${LIB_SOURCES}
)

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	target_link_libraries(${PROJECT_NAME} PRIVATE rt pthread)
endif()