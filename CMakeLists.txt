cmake_minimum_required(VERSION 3.10)

project(gfxlab)
set(CMAKE_CXX_STANDARD 14)
include_directories(${gfxlab_INCLUDE_DIRS} "${gfxlab_SOURCE_DIR}/include")
add_subdirectory( ${gfxlab_SOURCE_DIR}/glfw )

file(GLOB imgui_src "./imgui/*.h" "./imgui/*.cpp")
add_executable(gfxlab main.cpp glad.c shader.h ${imgui_src} stb_image.h)

target_link_libraries(gfxlab glfw)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)