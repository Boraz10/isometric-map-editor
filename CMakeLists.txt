cmake_minimum_required(VERSION 3.24...3.30)

# macOS settings (ignored on other platforms). These must come before project().
# std::filesystem needs macOS 10.15 or newer.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
# Build one universal binary that runs on both Intel and Apple Silicon Macs.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS CPU architectures")

project(IsometricMapEditor LANGUAGES C CXX)

# MSVC defaults to C++14, which has no std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies: download and build raylib
include(FetchContent)

set(RAYLIB_VERSION 6.0)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
)

FetchContent_MakeAvailable(raylib)

# Our program: every .c/.cpp file in src/ is compiled
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.c src/*.cpp src/*.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Output goes to build/IsometricMapEditor/ (Visual Studio adds a Debug/ or Release/ subfolder)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# The program loads its textures from GetApplicationDirectory()/resources,
# so copy src/resources next to the executable whenever it is rebuilt.
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/resources
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

# Frameworks raylib needs on macOS
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL")
endif()
