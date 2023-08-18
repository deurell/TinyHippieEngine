# cmake . -DCMAKE_TOOLCHAIN_FILE=/Users/mikael/tools/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=MinSizeRel -B web -G Ninja
cmake_minimum_required(VERSION 3.15)

project(tiny_hippie_engine)
set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_IMGUI")

if (WIN32)
    add_compile_options(-D_ITERATOR_DEBUG_LEVEL=0 -D_SECURE_SCL=0)
elseif (UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(LIBS ${LIBS} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread)
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
elseif (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --preload-file Shaders --preload-file Resources")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ../shell.html -s USE_GLFW=3 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")
endif ()

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_SOURCE_DIR}/assimp-5.0.1/include ${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/transcoder
)

add_executable(tiny_hippie_engine source/glad.c source/main.cpp source/app.cpp source/camera.h
        source/mesh.h source/model.h source/texture.h source/shader.h source/stb_image.h source/stb_truetype.h
        source/demoscene.cpp source/c64scene.cpp source/truetypescene.cpp source/textsprite.cpp include/app.h include/iscene.h include/demoscene.h include/c64scene.h include/truetypescene.h include/textsprite.h
        include/introscene.h source/introscene.cpp include/plane.h source/plane.cpp include/wildcopperscene.h
        source/wildcopperscene.cpp include/miniaudio.h source/audioplayer.cpp include/audioplayer.h source/simplescene.cpp include/simplescene.h
        include/particlescene.h source/particlescene.cpp include/particle.h source/particle.cpp source/particlesystem.cpp include/particlesystem.h
        include/scenenode.h
        source/scenenode.cpp
        include/icomponent.h
        include/planecomponent.h source/planenode.cpp source/simplenode.h
        source/labnodescene.h
        source/labnodescene.cpp)

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

add_library(basisu STATIC
        transcoder/basisu_file_headers.h
        transcoder/basisu_global_selector_cb.h
        transcoder/basisu_global_selector_palette.h
        transcoder/basisu_transcoder_internal.h
        transcoder/basisu_transcoder.cpp
        transcoder/basisu_transcoder.h
        transcoder/basisu.h
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set_target_properties(
            tiny_hippie_engine
            PROPERTIES
            SUFFIX ".html"
    )
endif ()

set(BUILD_STATIC_LIB ON CACHE BOOL "static" FORCE)
set(ASSIMP_BUILD_STATIC_LIB ON CACHE BOOL "static" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable export" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable test" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable tools" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable tools" FORCE)
set(ASSIMP_BUILD_3DS_IMPORTER OFF CACHE BOOL "Disable 3DS" FORCE)
set(ASSIMP_BUILD_3D_IMPORTER OFF CACHE BOOL "Disable 3D" FORCE)
set(ASSIMP_BUILD_3MF_IMPORTER OFF CACHE BOOL "Disable 3MF" FORCE)
set(ASSIMP_BUILD_AC_IMPORTER OFF CACHE BOOL "Disable AC" FORCE)
set(ASSIMP_BUILD_AMF_IMPORTER OFF CACHE BOOL "Disable AMF" FORCE)
set(ASSIMP_BUILD_ASE_IMPORTER OFF CACHE BOOL "Disable ASE" FORCE)
set(ASSIMP_BUILD_B3D_IMPORTER OFF CACHE BOOL "Disable B3D" FORCE)
set(ASSIMP_BUILD_BLEND_IMPORTER OFF CACHE BOOL "Disbale Blend" FORCE)
set(ASSIMP_BUILD_BVH_IMPORTER OFF CACHE BOOL "Disable BVH" FORCE)
set(ASSIMP_BUILD_COB_IMPORTER OFF CACHE BOOL "Disable COB" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER OFF CACHE BOOL "Disable Collada" FORCE)
set(ASSIMP_BUILD_CSM_IMPORTER OFF CACHE BOOL "Disable CSM" FORCE)
set(ASSIMP_BUILD_DXF_IMPORTER OFF CACHE BOOL "Disable DXF" FORCE)
set(ASSIMP_BUILD_HMP_IMPORTER OFF CACHE BOOL "Disable HMP" FORCE)
set(ASSIMP_BUILTER OFF CACHE BOOL "Disable LWS" FORCE)
set(ASSIMP_BUILD_NDO_IMPORTER OFF CACHE BOOL "Disable NDO" FORCE)
set(ASSIMP_BUILD_NFF_IMPORTER OFF CACHE BOOL "Disable NFF" FORCE)
set(ASSIMP_BUILD_NONFREE_C4D_IMPORTER OFF CACHE BOOL "Disable C4D" FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER OFF CACHE BOOL "Disable PLY" FORCE)
set(ASSIMP_BUILD_SIB_IMPORTER OFF CACHE BOOL "Disable SIB" FORCE)
set(ASSIMP_BUILD_SMD_IMPORTER OFF CACHE BOOL "Disable SMD" FORCE)
set(ASSIMP_BUILD_STL_IMPORTER OFF CACHE BOOL "Disable STL" FORCE)
set(ASSIMP_BUILD_TERRAGEN_IMPORTER OFF CACHE BOOL "Disable Terragen" FORCE)
set(ASSIMP_BUILD_X3D_IMPORTER OFF CACHE BOOL "Disable X3D" FORCE)
set(ASSIMP_BUILD_XGL_IMPORTER OFF CACHE BOOL "Disable XGL" FORCE)
set(ASSIMP_BUILD_X_IMPORTER OFF CACHE BOOL "Disable X" FORCE)
set(ASSIMP_BUILD_ASSBIN_IMPORTER OFF CACHE BOOL "Disable ASSBIN" FORCE)
set(ASSIMP_BUILD_MD2_IMPORTER OFF CACHE BOOL "Disable MD2" FORCE)
set(ASSIMP_BUILD_MD3_IMPORTER OFF CACHE BOOL "Disable MD3" FORCE)
set(ASSIMP_BUILD_MD5_IMPORTER OFF CACHE BOOL "Disable MD5" FORCE)
set(ASSIMP_BUILD_MDC_IMPORTER OFF CACHE BOOL "Disable MDC" FORCE)
set(ASSIMP_BUILD_MDL_IMPORTER OFF CACHE BOOL "Disable MDL" FORCE)
set(ASSIMP_BUILD_MS3D_IMPORTER OFF CACHE BOOL "Disable MS3D" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "Enable OBJ" FORCE)
set(ASSIMP_BUILD_OFF_IMPORTER OFF CACHE BOOL "Disable OFF" FORCE)
set(ASSIMP_BUILD_OGRE_IMPORTER OFF CACHE BOOL "Disable OGRE" FORCE)
set(ASSIMP_BUILD_OPENGEX_IMPORTER OFF CACHE BOOL "Disable OPENGEX" FORCE)
set(ASSIMP_BUILD_Q3BSP_IMPORTER OFF CACHE BOOL "Disable Q3BSP" FORCE)
set(ASSIMP_BUILD_Q3D_IMPORTER OFF CACHE BOOL "Disable Q3D" FORCE)
set(ASSIMP_BUILD_RAW_IMPORTER OFF CACHE BOOL "Disable RAW" FORCE)
set(ASSIMP_BUILD_M3D_IMPORTER OFF CACHE BOOL "Disable M3D" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER OFF CACHE BOOL "Disable FBX" FORCE)
set(ASSIMP_BUILD_MMD_IMPORTER OFF CACHE BOOL "Disable MMD" FORCE)
set(ASSIMP_BUILD_STEP_IMPORTER OFF CACHE BOOL "Disable STEP" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER OFF CACHE BOOL "Disable GLTF" FORCE)
set(ASSIMP_BUILD_IRRMESH_IMPORTER OFF CACHE BOOL "Disable IRRMESH" FORCE)
set(ASSIMP_BUILD_IRR_IMPORTER OFF CACHE BOOL "Disable IRR" FORCE)
set(ASSIMP_BUILD_LWO_IMPORTER OFF CACHE BOOL "Disable LWO" FORCE)
set(ASSIMP_BUILD_LWS_IMPORTER OFF CACHE BOOL "Disable LWS" FORCE)
set(ASSIMP_BUILD_IFC_IMPORTER OFF CACHE BOOL "Disable IFC" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/assimp)
set(LIBS ${LIBS} assimp)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_SOURCE_DIR}/glfw)
    set(LIBS ${LIBS} glfw)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GLAD")
    set(LIBTYPE "STATIC" CACHE STRING "libtype" FORCE)

    add_custom_command(
            TARGET tiny_hippie_engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Shaders ${CMAKE_CURRENT_BINARY_DIR}/Shaders
            COMMENT "Copying shaders" VERBATIM
    )
    add_custom_command(
            TARGET tiny_hippie_engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Resources ${CMAKE_CURRENT_BINARY_DIR}/Resources
            COMMENT "Copying resources" VERBATIM
    )
else ()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Resources)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Shaders)
endif ()

target_link_libraries(tiny_hippie_engine PRIVATE ${LIBS} imgui basisu)
