cmake_minimum_required(VERSION 3.8)

if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CLANG_EXECUTABLE clang++)
    if(CLANG_EXECUTABLE)
        message(STATUS "Using clang++: ${CLANG_EXECUTABLE}")
        set(CMAKE_CXX_COMPILER "${CLANG_EXECUTABLE}" CACHE STRING "C++ compiler" FORCE)
    else()
        message(WARNING "clang++ not found, using default C++ compiler")
    endif()
endif()

project(PrismCascade VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests and sample programs" OFF)

add_subdirectory(src)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
