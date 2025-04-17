# CMake minimum version required
cmake_minimum_required(VERSION 3.15)

# Project definition
project(TerraVecta VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Suppress deprecation warning for <experimental/filesystem>
add_definitions(-D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)

# Set the default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Use static runtime libraries in Debug mode to match Bullet Physics build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    # Set compile flags for static runtime
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

# --- Find Packages (vcpkg handles the 'how') ---
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

# Removed find_package for Bullet and manually linked Bullet libraries
# find_package(Bullet REQUIRED)

# Define paths for Bullet libraries (Debug vs Release)
set(BULLET_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/Bullet/lib)
set(BULLET_DEBUG_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/BulletBuild/lib/Debug)

# Set Bullet libraries based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Using Debug Bullet libraries")
    set(BULLET_COLLISION_LIB "${BULLET_DEBUG_LIB_DIR}/BulletCollision_Debug.lib")
    set(BULLET_DYNAMICS_LIB "${BULLET_DEBUG_LIB_DIR}/BulletDynamics_Debug.lib")
    set(BULLET_LINEAR_MATH_LIB "${BULLET_DEBUG_LIB_DIR}/LinearMath_Debug.lib")
else()
    message(STATUS "Using Release Bullet libraries")
    set(BULLET_COLLISION_LIB "${BULLET_LIB_DIR}/BulletCollision.lib")
    set(BULLET_DYNAMICS_LIB "${BULLET_LIB_DIR}/BulletDynamics.lib")
    set(BULLET_LINEAR_MATH_LIB "${BULLET_LIB_DIR}/LinearMath.lib")
endif()

# Add Bullet include paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/Bullet/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/Bullet/include/bullet)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/Bullet/include/bullet/LinearMath)

# Add include directories for assets and shaders
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/assets)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/shaders)

# --- Add GLAD Source ---
#set(GLAD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c)

# Define the executable and its source files
add_executable(TerraVecta
    src/main.cpp
    src/Application.cpp
    src/Window.cpp
    src/Shader.cpp
    src/glad.cpp
    src/Inventory.cpp
    src/CraftingSystem.cpp
    src/PhysicsEngine.cpp
    src/VoxelChunk.cpp
    src/TextureAtlas.cpp
    src/BiomeManager.cpp
    src/WaterSystem.cpp
    src/GameUI.cpp
    src/Biome.cpp
)

# Add include directories
target_include_directories(TerraVecta PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/stb
    # vcpkg paths handled automatically
)

# Add include directory for FastNoise
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/FastNoise)

# Define GLM_ENABLE_EXPERIMENTAL for TerraVecta target
target_compile_definitions(TerraVecta PUBLIC GLM_ENABLE_EXPERIMENTAL)

# --- Link Libraries ---
target_link_libraries(TerraVecta PRIVATE
    glfw         # Link against GLFW (found by find_package)
    OpenGL::GL   # Link against OpenGL (found by find_package)
    # FastNoise    # Link against FastNoise
    # glm is header-only
    # Windows dependencies handled by vcpkg's glfw3 target
    ${BULLET_COLLISION_LIB}
    ${BULLET_DYNAMICS_LIB}
    ${BULLET_LINEAR_MATH_LIB}
)

# Removed the add_subdirectory command for FastNoise as it does not contain a CMakeLists.txt file
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/FastNoise)

# Set output directory for executables explicitly for clarity
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR})

# Add custom commands to copy assets and shaders after the build
add_custom_command(TARGET TerraVecta POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    COMMENT "Copying assets and shaders to output directory"
)

# Optional: Print message during configuration
message(STATUS "Configuring TerraVecta project...")
message(STATUS "Targeting C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Found GLFW: ${glfw3_FOUND}")
message(STATUS "Found GLM: ${glm_FOUND}")
message(STATUS "Found OpenGL: ${OpenGL_FOUND}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Using Bullet Collision Library: ${BULLET_COLLISION_LIB}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Using static runtime (/MT) to match Bullet Debug libraries")
endif()

# Add testing support
include(CTest)
enable_testing()

# Add a test executable for unit tests
add_executable(TerraVectaTests
    src/tests/TestMain.cpp
    src/tests/TestVoxelChunk.cpp
    src/tests/TestApplication.cpp
)

# Add source files for Application and VoxelChunk to the test executable
target_sources(TerraVectaTests PRIVATE
    src/Application.cpp
    src/Window.cpp
    src/include/VoxelChunk.h
    src/VoxelChunk.cpp # Add VoxelChunk.cpp to the test executable
)

# Add Shader.cpp and glad.cpp to the test executable
target_sources(TerraVectaTests PRIVATE
    src/Shader.cpp
    src/glad.cpp
    src/Inventory.cpp
    src/CraftingSystem.cpp
    src/PhysicsEngine.cpp
    src/TextureAtlas.cpp
    src/BiomeManager.cpp
    src/WaterSystem.cpp
    src/GameUI.cpp
    src/Biome.cpp
)

# Add the custom command for TerraVectaTests *after* it's defined
add_custom_command(TARGET TerraVectaTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    COMMENT "Copying assets and shaders to test output directory"
)

# Add Catch2 as an external library
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v2.13.10 # Use a stable version
)
FetchContent_MakeAvailable(Catch2)

# Enable static library build for Catch2
set(CATCH_BUILD_STATIC_LIBRARY ON CACHE BOOL "Build Catch2 static library")

# Corrected the include directory for Catch2
include_directories(${catch2_SOURCE_DIR}/single_include/catch2)

# Add Catch2 library directory to linker paths
link_directories(${catch2_BINARY_DIR}/lib)

# Add include directory for glad
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# Remove duplicate target_include_directories for TerraVectaTests
# Ensure Bullet include directory is added to TerraVectaTests target (correct path)
target_include_directories(TerraVectaTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/stb
    # ... other include directories ...
)

# Define GLM_ENABLE_EXPERIMENTAL for TerraVectaTests target
target_compile_definitions(TerraVectaTests PRIVATE GLM_ENABLE_EXPERIMENTAL)

# Updated to use Catch2::Catch2 instead of Catch2::Catch2WithMain
# Link the test executable with necessary libraries
target_link_libraries(TerraVectaTests PRIVATE
    glfw
    OpenGL::GL
    ${BULLET_COLLISION_LIB}
    ${BULLET_DYNAMICS_LIB}
    ${BULLET_LINEAR_MATH_LIB}
    Catch2::Catch2
)

# Add the test executable to CTest
add_test(NAME TerraVectaTests COMMAND TerraVectaTests)
