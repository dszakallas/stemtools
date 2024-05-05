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

if(WITH_DEMUCS_TORCH)
  execute_process(COMMAND python3 -c "import torch;print(torch.utils.cmake_prefix_path)"
    OUTPUT_VARIABLE TORCH_CMAKE_PREFIX_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  list(APPEND CMAKE_PREFIX_PATH ${TORCH_CMAKE_PREFIX_PATH})
  find_package(Torch REQUIRED)
  find_package(avcpp REQUIRED)
  find_package(Threads REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
  add_executable(demucs-torch
    src/demucs/demucs.cpp
    src/demucs/demucs-torch.cpp
    src/common/error.cpp
    src/audio/audio.cpp
  )
  target_include_directories(demucs-torch PUBLIC ${TORCH_INCLUDE_DIRS} ${avcpp_INCLUDE_DIRS})
  target_link_libraries(demucs-torch PRIVATE ${TORCH_LIBRARIES} avcpp::avcpp-static)
  if(APPLE)
    target_link_libraries(demucs-torch PRIVATE "-framework Security")
  endif()

endif()

