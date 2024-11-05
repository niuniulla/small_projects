cmake_minimum_required(VERSION 3.22)

project(Quadtree)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -g -lSDL2 -lSDL2_ttf -lSDL2_image")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB src "./src/App.cpp")

add_executable(linear main_linear.cpp ${src})
add_executable(static main_staticquadtree.cpp ${src})
add_executable(trees main_trees.cpp ${src})
add_executable(dynamic main_dynamicquadtree.cpp ${src})