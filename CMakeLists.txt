cmake_minimum_required(VERSION 3.16)
project(CodeForge LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug symbols and optimizations for debugging
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Include FetchContent module for downloading dependencies
include(FetchContent)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

# Fetch nlohmann/json for configuration files
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Download ImGui manually if not present
set(IMGUI_DIR "${CMAKE_BINARY_DIR}/imgui")
if(NOT EXISTS "${IMGUI_DIR}")
    message(STATUS "Downloading ImGui...")
    file(DOWNLOAD 
        "https://github.com/ocornut/imgui/archive/refs/tags/v1.90.1.tar.gz"
        "${CMAKE_BINARY_DIR}/imgui.tar.gz"
        SHOW_PROGRESS
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_BINARY_DIR}/imgui.tar.gz"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E rename 
            "${CMAKE_BINARY_DIR}/imgui-1.90.1" 
            "${IMGUI_DIR}"
    )
endif()

# Create ImGui library
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PRIVATE
    OpenGL::GL
    ${GLFW_LIBRARIES}
)

# Add local glad files (commented out due to conflicts with GLFW)
# add_library(glad STATIC 
#     ../third_party/glad/src/glad.c
# )
# target_include_directories(glad PUBLIC 
#     ../third_party/glad/include
# )
# set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)

# Create a dummy glad library for compatibility
add_library(glad INTERFACE)

# Create the main CodeForge executable
add_executable(CodeForge
    src/main.cpp
    src/Application.cpp
    src/BuildSystem.cpp
    src/CodeEditor.cpp
    src/CompilerEngine.cpp
    src/FileManager.cpp
    src/ProjectManager.cpp
    src/SettingsManager.cpp
    src/UIManager.cpp
)

target_include_directories(CodeForge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLFW_INCLUDE_DIRS}
)

target_link_libraries(CodeForge PRIVATE
    OpenGL::GL
    ${GLFW_LIBRARIES}
    nlohmann_json::nlohmann_json
    imgui
    glad
)

if (UNIX)
    target_link_libraries(CodeForge PRIVATE dl pthread)
endif()

# Copy examples to build directory
file(COPY examples/ DESTINATION ${CMAKE_BINARY_DIR}/examples/)

# Set output directory
set_target_properties(CodeForge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)