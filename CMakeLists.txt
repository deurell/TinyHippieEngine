# cmake -DCMAKE_TOOLCHAIN_FILE=/Users/mikael/emscripten/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Debug -B web -G Ninja
cmake_minimum_required(VERSION 3.16)

project(gfxlab_project)
set(CMAKE_CXX_STANDARD 17)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEmscripten=1 -s USE_GLFW=3 -s USE_WEBGL2=1 -s ALLOW_MEMORY_GROWTH=1 --preload-file Shaders --preload-file Resources")
  set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

include_directories(${gfxlab_INCLUDE_DIRS} ${gfxlab_project_SOURCE_DIR}/include ${gfxlab_project_SOURCE_DIR}/assimp-5.0.1/include)

file(GLOB imgui_src "./imgui/*.h" "./imgui/*.cpp")
file(GLOB basis_src "./transcoder/*.h" "./transcoder/*.cpp")
add_executable(gfxlab glad.c ${imgui_src} ${basis_src} main.cpp app.cpp)

option(ENABLE_BOOST_WORKAROUND ON)
option(ASSIMP_BUILD_ZLIB OFF)
option(BUILD_STATIC_LIB ON)
option(ASSIMP_BUILD_STATIC_LIB ON)
option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_WERROR OFF)
option(ASSIMP_BUILD_NO_EXPORT ON)
option(BUILD_DOCS OFF)
add_subdirectory(${gfxlab_project_SOURCE_DIR}/assimp-5.0.1)
set(LIBS ${LIBS} assimp)

if (WIN32) 
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_ITERATOR_DEBUG_LEVEL=0 -D_SECURE_SCL=0")
elseif(UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  set(LIBS ${LIBS} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread)
  set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
endif()


if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  add_subdirectory(${gfxlab_project_SOURCE_DIR}/glfw)
  set(LIBS ${LIBS} glfw)
  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GLAD")
  
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
else()
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Resources)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Shaders)
endif()

target_link_libraries(gfxlab ${LIBS})