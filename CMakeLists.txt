cmake_minimum_required (VERSION 3.12)

project(boxworld)

include(FetchContent)

# Required for python modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Pybind11
message("Configuring Pybind11")
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG master
    SYSTEM
)
FetchContent_MakeAvailable(pybind11)

# Sources
set(BOXWORLD_SOURCES
    src/definitions.h
    src/boxworld_base.cpp 
    src/boxworld_base.h 
)

# CPP library
add_library(boxworld STATIC ${BOXWORLD_SOURCES})
target_compile_features(boxworld PUBLIC cxx_std_20)
target_include_directories(boxworld PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Python module
pybind11_add_module(pyboxworld EXCLUDE_FROM_ALL python/pyboxworld.cpp)
target_link_libraries(pyboxworld PRIVATE boxworld)
install(TARGETS pyboxworld DESTINATION .)

# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
