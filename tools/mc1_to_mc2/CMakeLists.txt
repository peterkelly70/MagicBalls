cmake_minimum_required(VERSION 3.16)
project(mc1_to_mc2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mc1_to_mc2 main.cpp)

if(MSVC)
    target_compile_options(mc1_to_mc2 PRIVATE /W4)
else()
    target_compile_options(mc1_to_mc2 PRIVATE -Wall -Wextra -Wpedantic)
endif()
