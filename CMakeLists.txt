cmake_minimum_required(VERSION 3.21)
project(x_engine)

set(CMAKE_CXX_STANDARD 17)

# ---- Options ----
option(XE_ASSIMP_SHARED "Build Assimp as a shared lib (assimp.dll)" ON)  # flip OFF for static

# Vulkan SDK
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
else()
    message(FATAL_ERROR "VULKAN_SDK not set")
endif()

# Paths
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/external/glfw)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(GLM_DIR ${CMAKE_SOURCE_DIR}/external/glm)
set(STB_DIR ${CMAKE_SOURCE_DIR}/external/stb)
set(ASSIMP_DIR ${CMAKE_SOURCE_DIR}/external/assimp)


# ---- Third-party: GLFW (static) ----
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)           # default static for libs unless overridden
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR})

# ---- Third-party: GLM (header-only) ----
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLM_DIR})

# ---- Third-party: Assimp (shared or static) ----
if (XE_ASSIMP_SHARED)
  set(ASSIMP_BUILD_SHARED_LIBS ON  CACHE BOOL "" FORCE)
else()
  set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER   ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER  ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER   ON  CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT            ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS   OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL              OFF CACHE BOOL "" FORCE)
add_subdirectory(${ASSIMP_DIR})

# Include Vulkan, GLFW, GLM, tinyobjloader, ImGUI and STB headers
include_directories(
        ${VULKAN_SDK_PATH}/Include
        ${GLFW_DIR}/include
        ${GLM_DIR}
        ${IMGUI_DIR}
        ${STB_DIR}
        ${ASSIMP_DIR}/include
)

# Link directories
link_directories(
        ${VULKAN_SDK_PATH}/Lib
        ${CMAKE_SOURCE_DIR}/external/glfw/lib
        ${CMAKE_SOURCE_DIR}/external/assimp/lib
)

# Source files
file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h)

# ImGui sources
set(IMGUI_SRC_FILES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_executable(x_engine ${SRC_FILES} ${IMGUI_SRC_FILES})

# Place output in root
set_target_properties(x_engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# src include dirs
target_include_directories(x_engine PRIVATE 
        ${CMAKE_SOURCE_DIR}/src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${STB_DIR})

target_compile_definitions(x_engine PRIVATE GLFW_INCLUDE_NONE)

# Link libraries
target_link_libraries(x_engine
        vulkan-1
        glfw
        glm::glm
        assimp
)

add_custom_command(TARGET x_engine POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_RUNTIME_DLLS:x_engine> $<TARGET_FILE_DIR:x_engine>
  COMMAND_EXPAND_LISTS
)
