cmake_minimum_required(VERSION 3.16)

project(libdor C)

option(LIBDOR_BUILD_DOCS "Build docs" ON)
option(LIBDOR_BUILD_SAMPLE "Build sample" ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # set(RIPPLE_VERBOSE ON CACHE BOOL "" FORCE)
    set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
endif()

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

if(LIBDOR_BUILD_DOCS)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
      add_custom_target(docs ALL
          COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DOCS
          COMMENT "Generating Doxygen documentation"
          VERBATIM
      )
  else()
      message(STATUS "Doxygen not found; documentation will not be generated during the build.")
  endif()
endif()

if(LIBDOR_BUILD_SAMPLE)
  add_subdirectory(testapp)
endif()
