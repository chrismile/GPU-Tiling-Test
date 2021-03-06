cmake_minimum_required (VERSION 3.11)
project (GPUTilingTest)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

set (CMAKE_CXX_STANDARD 11)

set(DATA_PATH "${CMAKE_SOURCE_DIR}/Data" CACHE PATH "location of folder 'Data'")
add_definitions(-DDATA_PATH=\"${DATA_PATH}\")

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)
IF(WIN32)
	add_executable(GPUTilingTest WIN32 ${SOURCES})
ELSE()
	add_executable(GPUTilingTest ${SOURCES})
ENDIF()
include_directories(src)

#make VERBOSE=1

set(FPHSA_NAME_MISMATCHED TRUE)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(PNG REQUIRED)
if((${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles") OR (${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles"))
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
	target_link_libraries(GPUTilingTest mingw32)
endif()
target_link_libraries(GPUTilingTest SDL2::Main)

cmake_policy(SET CMP0012 NEW)
find_package(OpenMP REQUIRED)
find_package(sgl REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
target_link_libraries(GPUTilingTest sgl ${Boost_LIBRARIES} ${OPENGL_LIBRARIES} GLEW::GLEW)
include_directories(${sgl_INCLUDES} ${Boost_INCLUDE_DIR} ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDES})

if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
