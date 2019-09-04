cmake_minimum_required(VERSION 3.10)

project(gfxlab)
set(CMAKE_CXX_STANDARD 17)
include_directories(${gfxlab_INCLUDE_DIRS} "${gfxlab_SOURCE_DIR}/include")
add_subdirectory( ${gfxlab_SOURCE_DIR}/glfw)

file(GLOB imgui_src "./imgui/*.h" "./imgui/*.cpp")

add_executable(gfxlab glad.c ${imgui_src} stb_image.h
        main.cpp texture.h shader.h
)

target_link_libraries(gfxlab glfw)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

add_custom_command(
        TARGET gfxlab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders ${CMAKE_CURRENT_BINARY_DIR}/Shaders
        COMMENT "Copying shaders" VERBATIM
)

add_custom_command(
        TARGET gfxlab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources ${CMAKE_CURRENT_BINARY_DIR}/Resources
        COMMENT "Copying resources" VERBATIM
)
