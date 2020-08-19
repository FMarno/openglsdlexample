cmake_minimum_required(VERSION 3.15)

project(SDL3D)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 CONFIG REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(example simple-example.cpp)
target_link_libraries(example PRIVATE SDL2::SDL2 SDL2::SDL2main GLEW::GLEW GLUT::GLUT glm)
#target_compile_options(cube PRIVATE -Werror -Wall -Wextra)
