cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(dsiflipencode)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

if(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif()

set(SOURCES
    src/lodepng.cpp
    src/main.cpp
    src/util.cpp
)

add_executable(dsiflipencode ${SOURCES})

target_include_directories(dsiflipencode
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)