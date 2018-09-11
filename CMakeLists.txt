cmake_minimum_required (VERSION 2.6)
project (GPUTilingTest)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)
add_executable(GPUTilingTest ${SOURCES})
include_directories(src)

#make VERBOSE=1

cmake_policy(SET CMP0012 NEW)
find_package(OpenMP REQUIRED)
find_package(SGL REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
target_link_libraries(GPUTilingTest GL)
target_link_libraries(GPUTilingTest GLEW)
target_link_libraries(GPUTilingTest sgl)

if(OPENMP_FOUND)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()