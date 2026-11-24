cmake_minimum_required(VERSION 3.20)

project(MousePicking)

find_package(PhysX CONFIG REQUIRED)
add_executable(MousePicking src/main.cpp src/PickingWorld.cpp src/PickingView.cpp)
target_link_libraries(MousePicking PRIVATE PhysX::physx_lib Graphics::Workbench)
