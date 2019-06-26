cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)

project(dsiflipencode)

set(SOURCES
    src/adpcm_encoder.cpp
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

if(MSVC)
  target_compile_options(dsiflipencode PRIVATE /W4 /WX)
else(MSVC)
  target_compile_options(dsiflipencode PRIVATE -Wall -Wextra -pedantic)
endif(MSVC)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")