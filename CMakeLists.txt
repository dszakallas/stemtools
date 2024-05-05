cmake_minimum_required(VERSION 3.10)

# Specify the project name and version
project(StemTools VERSION 1.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(get-stem-spec src/nistem/get-stem-spec.cpp)

if(WITH_NISTEM_GPAC)
  find_package(GPAC REQUIRED)
  add_executable(get-stem-spec-gpac src/nistem/get-stem-spec-gpac.cpp)
  target_include_directories(get-stem-spec-gpac PUBLIC ${GPAC_INCLUDE_DIR})
  target_link_libraries(get-stem-spec-gpac PRIVATE ${GPAC_LIBRARIES})
endif()


add_executable(demucs-test)
set(DEMUCS_TEST_SOURCES src/demucs/demucs.cpp src/demucs/demucs-test.cpp src/common/error.cpp src/audio/audio.cpp)

if(WITH_DEMUCS_TORCH)
  execute_process(COMMAND python3 -c "import torch;print(torch.utils.cmake_prefix_path)"
    OUTPUT_VARIABLE TORCH_CMAKE_PREFIX_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  list(APPEND CMAKE_PREFIX_PATH ${TORCH_CMAKE_PREFIX_PATH})
  find_package(Torch REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
  add_definitions(-DDEMUCS_TORCH)

  list(APPEND DEMUCS_TEST_SOURCES src/demucs/_torch/demucs.cpp)

  target_include_directories(demucs-test PUBLIC ${TORCH_INCLUDE_DIRS})
  target_link_libraries(demucs-test PRIVATE ${TORCH_LIBRARIES})
endif()

if(APPLE)
    target_link_libraries(demucs-test PRIVATE "-framework Security")
endif()

find_package(avcpp REQUIRED)
find_package(Threads REQUIRED)
find_package(argparse REQUIRED)
target_sources(demucs-test PRIVATE ${DEMUCS_TEST_SOURCES})
target_include_directories(demucs-test PUBLIC ${avcpp_INCLUDE_DIRS} ${argparse_INCLUDE_DIRS})
target_link_libraries(demucs-test PRIVATE avcpp::avcpp-static Threads::Threads)

