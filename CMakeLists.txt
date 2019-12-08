cmake_minimum_required(VERSION 3.10)

project(gfxlab)
set(CMAKE_CXX_STANDARD 17)
set(Emscripten 0)

if(Emscripten)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEmscripten=1 -s USE_GLFW=3 -s USE_WEBGL2=1 -s ALLOW_MEMORY_GROWTH=1 --preload-file Shaders --preload-file Resources")
  SET(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

include_directories(${gfxlab_INCLUDE_DIRS} "${gfxlab_SOURCE_DIR}/include")
file(GLOB imgui_src "./imgui/*.h" "./imgui/*.cpp")

add_executable(gfxlab glad.c ${imgui_src} stb_image.h
  main.cpp texture.h shader.h logger.h
)

if(NOT Emscripten)
  add_subdirectory( ${gfxlab_SOURCE_DIR}/glfw)
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
endif()
