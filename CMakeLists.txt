cmake_minimum_required(VERSION 3.1...3.14)
project(snake LANGUAGES CXX)

find_package(Curses REQUIRED)
find_package(Threads REQUIRED)

add_executable(snake snake.cpp)
target_link_libraries(snake "${CURSES_LIBRARIES}" Threads::Threads)