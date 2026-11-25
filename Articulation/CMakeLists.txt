cmake_minimum_required(VERSION 3.20)

project(Articulation)

find_package(PhysX CONFIG REQUIRED)

add_executable(Articulation src/main.cpp src/ArmWorld.cpp src/ArmView.cpp)
target_link_libraries(Articulation PRIVATE PhysX::physx_lib Graphics::Workbench)
