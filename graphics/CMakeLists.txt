add_library(graphics_window STATIC src/Window.cpp)
add_library(Graphics::Window ALIAS graphics_window)
target_compile_features(graphics_window PUBLIC cxx_std_17)
target_include_directories(graphics_window PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(graphics_window PUBLIC glad imgui glm::glm PRIVATE glfw OpenGL::GL)

add_library(graphics_scene_renderer STATIC src/SceneRenderer.cpp src/Shader.cpp src/Primitives.cpp)
add_library(Graphics::SceneRenderer ALIAS graphics_scene_renderer)
target_compile_features(graphics_scene_renderer PUBLIC cxx_std_17)
target_include_directories(graphics_scene_renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(graphics_scene_renderer PUBLIC glad glm::glm PRIVATE OpenGL::GL)

# Preserve each project's directory when copying its editable shader assets.
file(GLOB_RECURSE shaders CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    "shaders/*.glsl")
foreach(shader IN LISTS shaders)
    configure_file("shaders/${shader}" "shaders/${shader}" COPYONLY)
endforeach()
target_compile_definitions(graphics_scene_renderer PRIVATE
    GRAPHICS_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")

add_library(graphics_workbench STATIC src/Workbench.cpp)
add_library(Graphics::Workbench ALIAS graphics_workbench)
target_link_libraries(graphics_workbench PUBLIC Graphics::SceneRenderer imgui PRIVATE Graphics::Window)
