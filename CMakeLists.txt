cmake_minimum_required(VERSION 3.10)

# Specify the project name and version
project(StemTools VERSION 1.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define the executable
add_executable(get-stem-spec-gpac get-stem-spec-gpac.cpp)
add_executable(get-stem-spec get-stem-spec.cpp)

# Find the mp4v2 library
find_package(MP4v2 REQUIRED)
IF (MP4V2_FOUND)
    MESSAGE(STATUS "MP4V2 found")
ELSE (MP4V2_FOUND)
    MESSAGE(FATAL_ERROR "MP4V2 not found")
ENDIF (MP4V2_FOUND)

# Find the GPAC library
find_package(GPAC REQUIRED)
IF (GPAC_FOUND)
    MESSAGE(STATUS "GPAC found")
ELSE (GPAC_FOUND)
    MESSAGE(FATAL_ERROR "GPAC not found")
ENDIF (GPAC_FOUND)

# Find the TagLib library
find_package(taglib CONFIG REQUIRED)

find_package(ZLIB REQUIRED)

target_include_directories(get-stem-spec-gpac PUBLIC ${GPAC_INCLUDE_DIR})
target_link_libraries(get-stem-spec-gpac PRIVATE ${GPAC_LIBRARIES})
