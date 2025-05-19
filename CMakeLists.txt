cmake_minimum_required(VERSION 3.16)
project(TerraLink)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/*.rc")
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

# ZSTD
set(zstd_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/zstd")
find_package(zstd CONFIG REQUIRED)
target_link_libraries(TerraLink PRIVATE zstd::libzstd_shared)


# --- Install targets ---
install(TARGETS TerraLink RUNTIME DESTINATION bin)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/   DESTINATION assets)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/shaders/  DESTINATION shaders)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/registry/ DESTINATION registry)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/empty_saves")
install(DIRECTORY "${CMAKE_BINARY_DIR}/empty_saves/" DESTINATION saves)

install(FILES "${CMAKE_SOURCE_DIR}/game.settings" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/network.settings" DESTINATION .)

# --- CPack Configuration ---
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "TerraLink")
set(CPACK_PACKAGE_VENDOR "Grant Kopczenski")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Procedural terrain engine")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "9")
set(CPACK_PACKAGE_EXECUTABLES "TerraLink" "TerraLink")
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icon/icon.ico")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

install(FILES "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/glfw3.dll" DESTINATION bin)

install(FILES "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/zstd.dll" DESTINATION bin)

file(TO_CMAKE_PATH "$ENV{APPDATA}" _appdata_forward)
string(REPLACE "/" "\\\\" _appdata_win "${_appdata_forward}")

set(CPACK_NSIS_INSTALL_ROOT "${_appdata_win}")
set(CPACK_NSIS_INSTALL_DIRECTORY "TerraLink")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "TerraLink")

include(CPack)
