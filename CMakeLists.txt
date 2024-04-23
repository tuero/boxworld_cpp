cmake_minimum_required (VERSION 3.12)

project(boxworld)

# Sources
set(BOXWORLD_SOURCES
    src/definitions.h
    src/boxworld_base.cpp 
    src/boxworld_base.h 
)

# Build library
add_library(boxworld STATIC ${BOXWORLD_SOURCES})
target_compile_features(boxworld PUBLIC cxx_std_17)
target_include_directories(boxworld PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_include_directories(boxworld PUBLIC ${PROJECT_SOURCE_DIR}/include/libnop/include)

# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
