cmake_minimum_required(VERSION 3.5...4.3)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -flto")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)    
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(jpeg)
add_executable(jpeg src/main.cpp)
target_include_directories(jpeg PRIVATE thirdparty)