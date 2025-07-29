# Test CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Find required packages for testing
find_package(Qt5 COMPONENTS Widgets Test REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Enable Qt's MOC for tests
set(CMAKE_AUTOMOC ON)

# Include directories
include_directories(../src)

# Source files from main project (excluding main.cpp)
set(PROJECT_SOURCES
    ../src/db_manager.cpp
    ../src/mainwindow.cpp
)

set(PROJECT_HEADERS
    ../src/db_manager.h
    ../src/mainwindow.h
)

# Create the main test executable for database tests
add_executable(cvTracker_tests test_db_manager_standalone.cpp ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_link_libraries(cvTracker_tests 
    Qt5::Widgets
    Qt5::Test
    ${SQLITE3_LIBRARIES}
)

target_include_directories(cvTracker_tests PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_compile_options(cvTracker_tests PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Enable testing
enable_testing()

# Add the test
add_test(NAME database_tests COMMAND cvTracker_tests)