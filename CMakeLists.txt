cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)

project(dsiflipencode)

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

target_compile_features(dsiflipencode PUBLIC cxx_std_17)