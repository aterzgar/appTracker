cmake_minimum_required(VERSION 3.10)
project(ApplicationTracker)

set(CMAKE_CXX_STANDARD 11)

add_executable(ApplicationTracker main.cpp db_manager.cpp application_manager.cpp)
target_link_libraries(ApplicationTracker sqlite3)
