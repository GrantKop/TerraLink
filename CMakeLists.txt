cmake_minimum_required(VERSION 3.16)
project(TerraLink)

set(CMAKE_CXX_STANDARD 20)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
add_executable(TerraLink ${SOURCES})

# OpenMP
find_package(OpenMP REQUIRED)
if (OpenMP_CXX_FOUND)
    target_link_libraries(TerraLink PRIVATE OpenMP::OpenMP_CXX)
endif ()

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(TerraLink PRIVATE OpenGL::GL)

# GLFW
set(glfw3_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/glfw3")
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(TerraLink PRIVATE glfw)

# GLAD
set(glad_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/glad")
find_package(glad CONFIG REQUIRED)
target_link_libraries(TerraLink PRIVATE glad::glad)

# GLM (header-only)
set(glm_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/glm")
find_package(glm CONFIG REQUIRED)
target_include_directories(TerraLink PRIVATE ${glm_INCLUDE_DIRS})

# nlohmann-json (header-only)
set(nlohmann_json_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/nlohmann_json")
find_package(nlohmann_json CONFIG REQUIRED)
target_include_directories(TerraLink PRIVATE ${nlohmann_json_INCLUDE_DIRS})

# STB (header-only)
include_directories(${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include)
