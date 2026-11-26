cmake_minimum_required(VERSION 3.20)

project(Cloth)

add_executable(Cloth src/main.cpp src/ClothWorld.cpp src/ClothView.cpp)
target_link_libraries(Cloth PRIVATE Graphics::Workbench)
