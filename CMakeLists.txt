cmake_minimum_required(VERSION 3.10)

project(gfxlab)
set(CMAKE_CXX_STANDARD 14)
include_directories(${gfxlab_INCLUDE_DIRS} "${gfxlab_SOURCE_DIR}/include")
add_subdirectory( ${gfxlab_SOURCE_DIR}/glfw )

add_executable(gfxlab main.cpp glad.c shader.h
        imconfig.h imgui.cpp imgui.h imgui_draw.cpp imgui_internal.h imgui_widgets.cpp imstb_rectpack.h imstb_textedit.h imstb_truetype.h imgui_impl_glfw.h imgui_impl_opengl3.h imgui_impl_glfw.cpp imgui_impl_opengl3.cpp)

target_link_libraries(gfxlab glfw)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)