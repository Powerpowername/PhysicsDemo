cmake_minimum_required(VERSION 3.20)

project(HelloPhysX)

find_package(PhysX CONFIG REQUIRED)

add_executable(HelloPhysX)
target_sources(HelloPhysX PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_include_directories(HelloPhysX PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(HelloPhysX PRIVATE
    PhysX::physx_lib
    Graphics::Window)
