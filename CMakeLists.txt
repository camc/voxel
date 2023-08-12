cmake_minimum_required(VERSION 3.9.4...3.26.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(voxel VERSION 0.1.0)

add_executable(voxel vendor/glad/src/glad.c src/main.cpp src/debug.cpp src/render/vertexarray.cpp src/render/image.cpp src/gfxm/camera.cpp src/mgr/manager.cpp src/mgr/threadpool.cpp src/mgr/chunkstore.cpp src/render/renderer.cpp src/worldgen/generator.cpp)
include_directories(include vendor/glad/include vendor/glfw/include vendor/libspng/spng vendor vendor/FastNoise2/include vendor/tracy/public)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

set(SPNG_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/libspng)

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/FastNoise2)

option(TRACY_ENABLE "Enable Tracy" OFF)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(vendor/tracy)

target_compile_options(voxel PRIVATE -Wall -Werror -mavx2)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET voxel PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(voxel glfw spng_static FastNoise Tracy::TracyClient)
