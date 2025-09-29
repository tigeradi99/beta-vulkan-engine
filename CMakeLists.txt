cmake_minimum_required(VERSION 3.31)
project(x_engine)

set(CMAKE_CXX_STANDARD 17)

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
set(ASSIMP_DIR ${CMAKE_SOURCE_DIR}/external/assimp/include)

# Include Vulkan, GLFW, GLM, tinyobjloader, ImGUI and STB headers
include_directories(
        ${VULKAN_SDK_PATH}/Include
        ${GLFW_DIR}/include
        ${GLM_DIR}
        ${IMGUI_DIR}
        ${STB_DIR}
        ${ASSIMP_DIR}
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
        ${IMGUI_DIR}/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/imgui_impl_vulkan.cpp
)

add_executable(x_engine ${SRC_FILES} ${IMGUI_SRC_FILES})

# Link libraries
target_link_libraries(x_engine
        vulkan-1
        glfw3
        assimp-vc143-mt
)

# Place output in root
set_target_properties(x_engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# src include dirs
target_include_directories(x_engine PRIVATE ${CMAKE_SOURCE_DIR}/src)