cmake_minimum_required(VERSION 3.20)

project(PhysXCodeSpace)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# GLAD builds offline, using the bundled registry and templates and local Jinja2.
# Prefer the newest supported Python over unrelated versioned tools early in PATH.
set(Python_FIND_STRATEGY VERSION)
find_package(Python 3.8 COMPONENTS Interpreter REQUIRED)
execute_process(COMMAND "${Python_EXECUTABLE}" -c "import jinja2"
    RESULT_VARIABLE jinja2_status ERROR_QUIET)
if(NOT jinja2_status EQUAL 0)
    message(FATAL_ERROR "GLAD requires Jinja2 in ${Python_EXECUTABLE}. "
        "Install its Jinja2 package, or set -DPython_EXECUTABLE=/path/to/python3.")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/cmake
                 ${CMAKE_CURRENT_BINARY_DIR}/glad-cmake)
glad_add_library(glad REPRODUCIBLE API gl:core=4.1)

add_subdirectory(third_party/imgui)
add_subdirectory(graphics)

# Physics projects are siblings of RigidBody and define their own targets.
file(GLOB project_files CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*/CMakeLists.txt")
list(REMOVE_ITEM project_files "graphics/CMakeLists.txt" "third_party/CMakeLists.txt")
foreach(project_file IN LISTS project_files)
    get_filename_component(project_dir "${project_file}" DIRECTORY)
    if(project_dir MATCHES "^\\." OR "${CMAKE_CURRENT_SOURCE_DIR}/${project_dir}" STREQUAL CMAKE_BINARY_DIR)
        continue()
    endif()
    add_subdirectory("${project_dir}")
endforeach()

