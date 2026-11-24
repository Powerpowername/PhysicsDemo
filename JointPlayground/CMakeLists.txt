cmake_minimum_required(VERSION 3.20)

project(JointPlayground)

find_package(PhysX CONFIG REQUIRED)

add_executable(JointPlayground
    src/main.cpp
    src/JointWorld.cpp
    src/JointView.cpp
)
target_link_libraries(JointPlayground PRIVATE PhysX::physx_lib Graphics::Workbench)
