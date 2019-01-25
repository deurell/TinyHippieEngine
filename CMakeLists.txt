cmake_minimum_required(VERSION 3.13)

project(gfxlab)
set(CMAKE_CXX_STANDARD 14)
include_directories(${gfxlab_INCLUDE_DIRS} "${gfxlab_SOURCE_DIR}/include")
add_subdirectory( ${gfxlab_SOURCE_DIR}/glfw )

add_executable(gfxlab main.cpp glad.c)

target_link_libraries(gfxlab glfw)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)