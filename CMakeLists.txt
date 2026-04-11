# cmake . -DCMAKE_TOOLCHAIN_FILE=/path/to/emscripten.cmake -DCMAKE_BUILD_TYPE=MinSizeRel -B web -G Ninja
cmake_minimum_required(VERSION 3.15)

project(tiny_hippie_engine LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)

include(CTest)

add_compile_definitions(USE_IMGUI)

set(PLATFORM_LIBS "")

if(WIN32)
add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0 _SECURE_SCL=0)
elseif(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
list(APPEND PLATFORM_LIBS X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread)
endif()

if(EMSCRIPTEN)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --preload-file Shaders --preload-file Resources")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ../shell.html -s USE_GLFW=3 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")
endif()

add_executable(tiny_hippie_engine source/glad.c source/main.cpp source/app.cpp source/logger.cpp source/camera.h
        source/debugui.h source/debugui.cpp
        include/renderdevice.h source/openglrenderdevice.cpp include/scenelifecycle.h source/scenelifecycle.cpp
        source/mesh.h source/model.h source/texture.h source/shader.h source/stb_image.h source/stb_truetype.h
        source/demoscene.cpp source/c64scene.cpp source/truetypescene.cpp source/textsprite.cpp include/app.h include/iscene.h include/demoscene.h include/c64scene.h include/truetypescene.h include/textsprite.h
        include/introscene.h source/introscene.cpp include/plane.h source/plane.cpp include/wildcopperscene.h
        source/wildcopperscene.cpp include/miniaudio.h source/audioplayer.cpp include/audioplayer.h source/simplescene.cpp include/simplescene.h
        include/particlescene.h source/particlescene.cpp include/particle.h source/particle.cpp source/particlesystem.cpp include/particlesystem.h
        include/scenenode.h
        source/scenenode.cpp
        include/visualizerbase.h
        source/planenode.cpp
        source/planenode.h
        source/nodeexamplescene.h
        source/nodeexamplescene.cpp
        source/textnode.cpp
        source/textnode.h
        source/textvisualizer.h
        source/textvisualizer.cpp
        source/planevisualizer.h
        source/planevisualizer.cpp
        source/glosifyscene.cpp
        source/glosifyscene.h
        source/spritenode.cpp
        source/spritenode.h
        source/spritevisualizer.cpp
        source/spritevisualizer.h
        source/quicknodescene.cpp
        source/quicknodescene.h
        source/particlesystemnode.cpp
        source/particlesystemnode.h
        source/particlenodescene.cpp
        source/model.cpp
        include/particlenodescene.h
)

target_include_directories(tiny_hippie_engine
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_SOURCE_DIR}/imgui
        ${CMAKE_SOURCE_DIR}/transcoder
)

target_compile_definitions(tiny_hippie_engine PRIVATE GLFW_INCLUDE_NONE)

add_library(imgui STATIC
        imgui/imconfig.h
        imgui/imgui.cpp
        imgui/imgui.h
        imgui/imgui_draw.cpp
        imgui/imgui_impl_glfw.cpp
        imgui/imgui_impl_glfw.h
        imgui/imgui_impl_opengl3.cpp
        imgui/imgui_impl_opengl3.h
        imgui/imgui_internal.h
        imgui/imgui_widgets.cpp
        imgui/imstb_rectpack.h
        imgui/imstb_textedit.h
        imgui/imstb_truetype.h
)

target_include_directories(imgui
        PUBLIC ${CMAKE_SOURCE_DIR}/imgui
        PRIVATE ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(imgui PRIVATE GLFW_INCLUDE_NONE)

add_library(basisu STATIC
        transcoder/basisu_file_headers.h
        transcoder/basisu_global_selector_cb.h
        transcoder/basisu_global_selector_palette.h
        transcoder/basisu_transcoder_internal.h
        transcoder/basisu_transcoder.cpp
        transcoder/basisu_transcoder.h
        transcoder/basisu.h
)

target_include_directories(basisu PUBLIC ${CMAKE_SOURCE_DIR}/transcoder)

if(EMSCRIPTEN)
set_target_properties(
            tiny_hippie_engine
            PROPERTIES
            SUFFIX ".html"
)
endif()

set(BUILD_STATIC_LIB ON CACHE BOOL "static" FORCE)

if(NOT EMSCRIPTEN)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/glfw)
list(APPEND PLATFORM_LIBS glfw)
set(LIBTYPE "STATIC" CACHE STRING "libtype" FORCE)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
endif()

if(APPLE)
foreach(target tiny_hippie_engine imgui basisu)
    target_compile_options(${target} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
endforeach()
endif()

target_link_libraries(tiny_hippie_engine PRIVATE imgui basisu ${PLATFORM_LIBS})

foreach(asset_dir IN ITEMS Shaders Resources)
add_custom_command(
        TARGET tiny_hippie_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/${asset_dir}
                $<TARGET_FILE_DIR:tiny_hippie_engine>/${asset_dir}
        COMMENT "Copying ${asset_dir}" VERBATIM
)
endforeach()

if(EMSCRIPTEN)
foreach(asset_dir IN ITEMS Shaders Resources)
    add_custom_command(
            TARGET tiny_hippie_engine PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_CURRENT_SOURCE_DIR}/${asset_dir}
                    ${CMAKE_CURRENT_BINARY_DIR}/${asset_dir}
            COMMENT "Preparing ${asset_dir} for packaging" VERBATIM
    )
endforeach()
endif()

if(BUILD_TESTING AND NOT EMSCRIPTEN)
include(FetchContent)

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(tiny_hippie_engine_tests
        tests/camera_test.cpp
        tests/iscene_test.cpp
        tests/scenelifecycle_test.cpp
        tests/scenemanager_test.cpp
        tests/scenenode_test.cpp
        source/scenelifecycle.cpp
        source/scenenode.cpp
)

target_include_directories(tiny_hippie_engine_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/source
)

target_compile_definitions(tiny_hippie_engine_tests PRIVATE GLFW_INCLUDE_NONE)

target_link_libraries(tiny_hippie_engine_tests PRIVATE GTest::gtest_main)

gtest_discover_tests(tiny_hippie_engine_tests)
endif()
