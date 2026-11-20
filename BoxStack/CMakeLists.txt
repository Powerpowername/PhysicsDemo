cmake_minimum_required(VERSION 3.20)

project(BoxStack)

find_package(PhysX CONFIG REQUIRED)

add_executable(BoxStack src/main.cpp src/StackWorld.cpp src/BoxStackView.cpp src/BallRenderer.cpp)
target_link_libraries(BoxStack PRIVATE PhysX::physx_lib Graphics::Workbench)
