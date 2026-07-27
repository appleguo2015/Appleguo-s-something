cmake_minimum_required(VERSION 3.20)
project(VirtualSinger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib 5.5 REQUIRED)

add_executable(virtual_singer src/main.cpp)
target_link_libraries(virtual_singer PRIVATE raylib)

add_custom_command(TARGET virtual_singer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:virtual_singer>/assets)

