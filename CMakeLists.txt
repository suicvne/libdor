cmake_minimum_required(VERSION 3.16)

project(libdor C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(dor
    src/DOR.c
    src/DORCards.c
    src/DORText.c
    src/PSU.c
)

target_include_directories(dor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_subdirectory(testapp)
