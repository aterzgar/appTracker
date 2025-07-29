# Test CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(cvTracker_tests)

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

# Create separate test executables for each test file
# This avoids multiple definition errors and AutoMoc issues

# Database Manager Tests
add_executable(test_db_manager test_db_manager.cpp ${PROJECT_SOURCES})
target_link_libraries(test_db_manager 
    Qt5::Widgets
    Qt5::Test
    ${SQLITE3_LIBRARIES}
)
target_include_directories(test_db_manager PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_compile_options(test_db_manager PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Analytics Tests
add_executable(test_analytics test_analytics.cpp ${PROJECT_SOURCES})
target_link_libraries(test_analytics 
    Qt5::Widgets
    Qt5::Test
    ${SQLITE3_LIBRARIES}
)
target_include_directories(test_analytics PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_compile_options(test_analytics PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Application Logic Tests
add_executable(test_application_logic test_application_logic.cpp ${PROJECT_SOURCES})
target_link_libraries(test_application_logic 
    Qt5::Widgets
    Qt5::Test
    ${SQLITE3_LIBRARIES}
)
target_include_directories(test_application_logic PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_compile_options(test_application_logic PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Enable testing
enable_testing()

# Add individual tests
add_test(NAME database_tests COMMAND test_db_manager)
add_test(NAME analytics_tests COMMAND test_analytics)
add_test(NAME application_logic_tests COMMAND test_application_logic)

# Test runner executable
add_executable(test_main test_main.cpp)
target_link_libraries(test_main Qt5::Core)

# Create a convenience target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_db_manager test_analytics test_application_logic
    COMMENT "Running all tests"
)