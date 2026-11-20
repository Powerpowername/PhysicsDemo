cmake_minimum_required(VERSION 3.20)

project(RigidBody)

find_package(PhysX CONFIG REQUIRED)

add_executable(RigidBody src/main.cpp src/PhysicsWorld.cpp src/RigidBodyView.cpp)
target_link_libraries(RigidBody PRIVATE PhysX::physx_lib Graphics::Workbench)
