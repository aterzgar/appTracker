cmake_minimum_required(VERSION 3.10)
project(cvTracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Enable Qt's MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/db_manager.cpp
)

set(HEADERS
    src/mainwindow.h
    src/db_manager.h
)

# Create executable
add_executable(cvTracker ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(cvTracker 
    Qt5::Widgets
    ${SQLITE3_LIBRARIES}
)

# Include directories for SQLite
target_include_directories(cvTracker PRIVATE ${SQLITE3_INCLUDE_DIRS})

# Compiler flags for SQLite
target_compile_options(cvTracker PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Set output directory
set_target_properties(cvTracker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)