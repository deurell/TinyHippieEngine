# cmake . -DCMAKE_TOOLCHAIN_FILE=/path/to/emscripten.cmake -DCMAKE_BUILD_TYPE=MinSizeRel -B web -G Ninja
cmake_minimum_required(VERSION 3.15)

project(tiny_hippie_engine LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)

if (APPLE)
    enable_language(OBJC)
endif ()

include(CTest)
include(FetchContent)

option(TINY_ENGINE_ENABLE_IMGUI "Build with ImGui debug UI" ON)
option(TINY_ENGINE_ENABLE_PHYSICS "Build physics support with Box3D" OFF)

set(PLATFORM_LIBS "")

if (WIN32)
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0 _SECURE_SCL=0)
elseif (UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    list(APPEND PLATFORM_LIBS X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread)
endif ()

if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --preload-file Shaders --preload-file Resources")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ../shell.html -s USE_GLFW=3 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")
endif ()

if (TINY_ENGINE_ENABLE_PHYSICS)
    set(BOX3D_SAMPLES OFF CACHE BOOL "" FORCE)
    set(BOX3D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BOX3D_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(BOX3D_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
            box3d
            GIT_REPOSITORY https://github.com/erincatto/box3d.git
            GIT_TAG v0.1.0
    )
    FetchContent_MakeAvailable(box3d)
endif ()

add_executable(tiny_hippie_engine
        source/glad.c
        source/main.cpp
        source/app.cpp
        source/logger.cpp
        source/camera.h
        source/cameranode.h source/cameranode.cpp
        source/animationclip.h source/animationclip.cpp
        source/animationplayer.h source/animationplayer.cpp
        source/meshasset.h source/meshasset.cpp
        source/meshassetcache.h source/meshassetcache.cpp
        source/meshnode.h source/meshnode.cpp
        source/meshvisualizer.h source/meshvisualizer.cpp
        source/model.h source/model.cpp
        source/particle.cpp
        source/particlesystem.cpp
        source/particlesystemnode.h source/particlesystemnode.cpp
        source/particlevisualizer.h source/particlevisualizer.cpp
        source/planenode.h source/planenode.cpp
        source/planevisualizer.h source/planevisualizer.cpp
        source/phongshapenode.h source/phongshapenode.cpp
        source/shapegeometry.h source/shapegeometry.cpp
        source/shapevisualizer.h source/shapevisualizer.cpp
        source/spritenode.h source/spritenode.cpp
        source/spritevisualizer.h source/spritevisualizer.cpp
        source/textnode.h source/textnode.cpp
        source/textvisualizer.h source/textvisualizer.cpp
        source/tilemapnode.h source/tilemapnode.cpp
        source/tilemapvisualizer.h source/tilemapvisualizer.cpp
        source/renderresourcecache.h source/renderresourcecache.cpp
        source/scenedescription.h source/scenedescription.cpp
        source/skinning.h source/skinning.cpp
        source/debugui.h source/debugui.cpp
        source/game/scenes/textstarterscene.h
        source/game/scenes/textstarterscene.cpp
        source/game/scenes/skeletalanimationblendscene.h
        source/game/scenes/skeletalanimationblendscene.cpp
        source/shader.h
        source/stb_image.h source/stb_image.cpp
        source/stb_truetype.h source/stb_truetype.cpp
        source/texture.h
        include/app.h
        include/audiosystem.h source/audiosystem.cpp
        include/iscene.h
        include/miniaudio.h
        include/particle.h
        include/particlesystem.h
        include/plane.h source/plane.cpp
        include/renderdevice.h source/openglrenderdevice.cpp
        include/scenelifecycle.h source/scenelifecycle.cpp
        include/scenemanager.h
        include/scenenode.h source/scenenode.cpp
        include/textsprite.h source/textsprite.cpp
        include/visualizerbase.h
)

if (TINY_ENGINE_ENABLE_PHYSICS)
    target_sources(tiny_hippie_engine PRIVATE
            source/physicscontext.h source/physicscontext.cpp
            source/physicsworld.h source/physicsworld.cpp
            source/physicsbodycomponent.h source/physicsbodycomponent.cpp
            source/physicsdebugvisualizer.h source/physicsdebugvisualizer.cpp
            source/physicsmeshvisualizer.h source/physicsmeshvisualizer.cpp
            source/game/scenes/physicstestscene.h
            source/game/scenes/physicstestscene.cpp
    )
endif ()

target_include_directories(tiny_hippie_engine
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_SOURCE_DIR}/transcoder
)

target_compile_definitions(tiny_hippie_engine PRIVATE GLFW_INCLUDE_NONE)
if (TINY_ENGINE_ENABLE_IMGUI)
    target_compile_definitions(tiny_hippie_engine PRIVATE USE_IMGUI)
    target_include_directories(tiny_hippie_engine PRIVATE ${CMAKE_SOURCE_DIR}/imgui)
endif ()
if (TINY_ENGINE_ENABLE_PHYSICS)
    target_compile_definitions(tiny_hippie_engine PRIVATE TINY_ENGINE_ENABLE_PHYSICS=1)
endif ()

if (TINY_ENGINE_ENABLE_IMGUI)
    add_library(imgui STATIC
            imgui/imconfig.h
            imgui/imgui.cpp
            imgui/imgui.h
            imgui/imgui_draw.cpp
            imgui/imgui_impl_glfw.cpp
            imgui/imgui_impl_glfw.h
            imgui/imgui_impl_opengl3.cpp
            imgui/imgui_impl_opengl3.h
            imgui/imgui_impl_opengl3_loader.h
            imgui/imgui_internal.h
            imgui/imgui_tables.cpp
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
endif ()

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

if (EMSCRIPTEN)
    set_target_properties(
            tiny_hippie_engine
            PROPERTIES
            SUFFIX ".html"
    )
endif ()

set(BUILD_STATIC_LIB ON CACHE BOOL "static" FORCE)

if (NOT EMSCRIPTEN)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_SOURCE_DIR}/glfw)
    list(APPEND PLATFORM_LIBS glfw)
    set(LIBTYPE "STATIC" CACHE STRING "libtype" FORCE)

    if (TINY_ENGINE_ENABLE_IMGUI)
        target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
    endif ()
endif ()

if (APPLE)
    set(visibility_targets tiny_hippie_engine basisu)
    if (TINY_ENGINE_ENABLE_IMGUI)
        list(APPEND visibility_targets imgui)
    endif ()
    foreach (target ${visibility_targets})
        target_compile_options(${target} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
    endforeach ()
endif ()

if (TINY_ENGINE_ENABLE_PHYSICS AND TARGET box3d::box3d)
    target_link_libraries(tiny_hippie_engine PRIVATE box3d::box3d)
elseif (TINY_ENGINE_ENABLE_PHYSICS AND TARGET box3d)
    target_link_libraries(tiny_hippie_engine PRIVATE box3d)
endif ()

target_link_libraries(tiny_hippie_engine PRIVATE basisu ${PLATFORM_LIBS})
if (TINY_ENGINE_ENABLE_IMGUI)
    target_link_libraries(tiny_hippie_engine PRIVATE imgui)
endif ()

add_custom_target(tiny_hippie_engine_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
        ${CMAKE_CURRENT_BINARY_DIR}/Shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources
        ${CMAKE_CURRENT_BINARY_DIR}/Resources
        COMMENT "Copying runtime assets" VERBATIM
)
add_dependencies(tiny_hippie_engine tiny_hippie_engine_assets)

if (EMSCRIPTEN)
    foreach (asset_dir IN ITEMS Shaders Resources)
        add_custom_command(
                TARGET tiny_hippie_engine PRE_LINK
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/${asset_dir}
                ${CMAKE_CURRENT_BINARY_DIR}/${asset_dir}
                COMMENT "Preparing ${asset_dir} for packaging" VERBATIM
        )
    endforeach ()
endif ()

if (BUILD_TESTING AND NOT EMSCRIPTEN)

    find_package(GTest CONFIG QUIET)
    if (NOT GTest_FOUND)
        FetchContent_Declare(
                googletest
                URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif ()

    include(GoogleTest)

    add_executable(tiny_hippie_engine_tests
            tests/animationclip_test.cpp
            tests/animationplayer_test.cpp
            tests/cameranode_test.cpp
            tests/camera_test.cpp
            tests/iscene_test.cpp
            tests/meshasset_test.cpp
            tests/meshassetcache_test.cpp
            tests/skinning_test.cpp
            source/animationclip.cpp
            source/animationplayer.cpp
            source/meshassetcache.cpp
            source/skinning.cpp
            tests/scenelifecycle_test.cpp
            tests/scenemanager_test.cpp
            tests/scenenode_test.cpp
            tests/particlesystemnode_test.cpp
            tests/scenedescription_test.cpp
            source/logger.cpp
            source/cameranode.cpp
            source/meshasset.cpp
            source/meshnode.cpp
            source/meshvisualizer.cpp
            source/particlevisualizer.cpp
            source/particlesystemnode.cpp
            source/planenode.cpp
            source/planevisualizer.cpp
            source/phongshapenode.cpp
            source/renderresourcecache.cpp
            source/scenedescription.cpp
            source/scenelifecycle.cpp
            source/scenenode.cpp
            source/shapegeometry.cpp
            source/shapevisualizer.cpp
            source/spritenode.cpp
            source/spritevisualizer.cpp
            source/stb_image.cpp
            source/stb_truetype.cpp
            source/textnode.cpp
            source/textvisualizer.cpp
            source/tilemapnode.cpp
            source/tilemapvisualizer.cpp
    )

    if (TINY_ENGINE_ENABLE_PHYSICS)
        target_sources(tiny_hippie_engine_tests PRIVATE
                tests/physicsworld_test.cpp
                source/physicscontext.cpp
                source/physicsworld.cpp
                source/physicsbodycomponent.cpp
        )
    endif ()

    target_include_directories(tiny_hippie_engine_tests
            PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/source
            ${CMAKE_SOURCE_DIR}/transcoder
    )

    target_compile_definitions(tiny_hippie_engine_tests PRIVATE GLFW_INCLUDE_NONE)
    if (TINY_ENGINE_ENABLE_PHYSICS)
        target_compile_definitions(tiny_hippie_engine_tests PRIVATE TINY_ENGINE_ENABLE_PHYSICS=1)
    endif ()

    if (TINY_ENGINE_ENABLE_PHYSICS AND TARGET box3d::box3d)
        target_link_libraries(tiny_hippie_engine_tests PRIVATE box3d::box3d)
    elseif (TINY_ENGINE_ENABLE_PHYSICS AND TARGET box3d)
        target_link_libraries(tiny_hippie_engine_tests PRIVATE box3d)
    endif ()

    target_link_libraries(tiny_hippie_engine_tests PRIVATE GTest::gtest_main)

    gtest_discover_tests(tiny_hippie_engine_tests)
endif ()
