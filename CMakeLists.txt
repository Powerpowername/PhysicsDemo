cmake_minimum_required(VERSION 3.20)

project(PhysXCodeSpace)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# Prefer a machine-installed GLM, while keeping a reproducible fallback for
# fresh machines (the same target name is used in both cases).
find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
    message(FATAL_ERROR "GLM was not found. Install libglm-dev or set glm_DIR.")
endif()

# GLAD is generated as part of the build so its loader matches the OpenGL API
# requested by the application.  This avoids relying on a distribution
# specific GLAD package or a checked-in generated binary.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/cmake
                 ${CMAKE_CURRENT_BINARY_DIR}/glad-cmake)
glad_add_library(glad REPRODUCIBLE API gl:core=4.1)

add_subdirectory(HelloPhysX)

