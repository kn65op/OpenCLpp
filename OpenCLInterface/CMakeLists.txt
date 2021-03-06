project(opencl-interface)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH "../cmake")

find_package(OpenCL 1.1 REQUIRED)

set(opencl-interface-srcs
  OpenCLAlgorithm.cpp
  OpenCLInvertImage.cpp
  OpenCLBayerFilter.cpp
  OpenCLDevice.cpp
  OpenCLException.cpp
  OpenCLAlgorithmsStream.cpp
  OpenCLFloatToInt.cpp
  OpenCLIntToFloat.cpp)
  
set(opencl-interface-headers
  OpenCLAlgorithm.h
  OpenCLInvertImage.h
  OpenCLBayerFilter.h
  OpenCLDevice.h
  OpenCLException.h
  OpenCLAlgorithmsStream.h
  OpenCLFloatToInt.h
  OpenCLIntToFloat.h)

message(${OPENCL_INCLUDE_DIR})
include_directories(${OPENCL_INCLUDE_DIR})

add_library(opencl-interface SHARED ${opencl-interface-srcs})
target_link_libraries(opencl-interface ${OPENCL_LIBRARY})