cmake_minimum_required(VERSION 3.16)
project(CodeForge LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug symbols and optimizations for debugging
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Include FetchContent module for downloading dependencies
include(FetchContent)

# Find required packages
find_package(OpenGL REQUIRED)

if(TARGET OpenGL::GL)
    set(CODEFORGE_OPENGL_TARGET OpenGL::GL)
elseif(TARGET OpenGL::OpenGL)
    set(CODEFORGE_OPENGL_TARGET OpenGL::OpenGL)
else()
    message(FATAL_ERROR "No suitable OpenGL target was provided by find_package(OpenGL).")
endif()

# Prefer a system GLFW when available, but fall back to building it from source
# so CI can produce Windows, Linux, and macOS builds from the same CMake setup.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_search_module(GLFW QUIET glfw3)
endif()

if(NOT GLFW_FOUND AND NOT TARGET glfw)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

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

if(TARGET glfw)
    target_link_libraries(imgui PRIVATE ${CODEFORGE_OPENGL_TARGET} glfw)
else()
    target_link_libraries(imgui PRIVATE ${CODEFORGE_OPENGL_TARGET} ${GLFW_LIBRARIES})
endif()

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
    src/CodexManager.cpp
    src/FileManager.cpp
    src/GitManager.cpp
    src/ProjectManager.cpp
    src/SettingsManager.cpp
    src/UIManager.cpp
)

target_include_directories(CodeForge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(GLFW_INCLUDE_DIRS)
    target_include_directories(CodeForge PRIVATE ${GLFW_INCLUDE_DIRS})
endif()

target_link_libraries(CodeForge PRIVATE
    ${CODEFORGE_OPENGL_TARGET}
    nlohmann_json::nlohmann_json
    imgui
    glad
)

if(TARGET glfw)
    target_link_libraries(CodeForge PRIVATE glfw)
else()
    target_link_libraries(CodeForge PRIVATE ${GLFW_LIBRARIES})
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(CodeForge PRIVATE dl pthread)
endif()

# Copy examples to build directory
file(COPY examples/ DESTINATION ${CMAKE_BINARY_DIR}/examples/)

# Set output directory
set_target_properties(CodeForge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
