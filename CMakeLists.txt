cmake_minimum_required(VERSION 3.16)

project(waveletanalysis)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")

# set output dirs for executable and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# link local fftw
set(FFTW_ROOT "${CMAKE_SOURCE_DIR}/external/fftw3")
add_library(FFTW3::FFTW3 STATIC IMPORTED GLOBAL)
set_target_properties(FFTW3::FFTW3 PROPERTIES
    IMPORTED_LOCATION "${FFTW_ROOT}/lib/libfftw3.a"
    INTERFACE_INCLUDE_DIRECTORIES "${FFTW_ROOT}/include"
)


# project modules
add_subdirectory(src)

target_link_libraries(waveletanalysis PRIVATE
    FFTW3::FFTW3
    m
)







