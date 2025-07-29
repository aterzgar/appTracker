cmake_minimum_required(VERSION 3.10)
project(cvTracker_tests)

# Find required packages for testing
find_package(Qt5 COMPONENTS Widgets Test REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Enable Qt's MOC, UIC, and RCC for tests if needed
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Always include parent src directory for headers
include_directories(../src)

set(PROJECT_SOURCES
    ../src/db_manager.cpp
    ../src/mainwindow.cpp
)

# 1. Database Manager Tests
add_executable(test_db_manager test_db_manager.cpp ${PROJECT_SOURCES})
target_link_libraries(test_db_manager
    Qt5::Widgets
    Qt5::Test
    ${SQLITE3_LIBRARIES}
)
target_include_directories(test_db_manager PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_compile_options(test_db_manager PRIVATE ${SQLITE3_CFLAGS_OTHER})
set_target_properties(test_db_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 2. Analytics Tests
add_executable(test_analytics test_analytics.cpp ${PROJECT_SOURCES})
target_link_libraries(test_analytics
    Qt5::Widgets
    Qt5::Test
    ${SQLITE3_LIBRARIES}
)
target_include_directories(test_analytics PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_compile_options(test_analytics PRIVATE ${SQLITE3_CFLAGS_OTHER})
set_target_properties(test_analytics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 3. Application Logic Tests
add_executable(test_application_logic test_application_logic.cpp ${PROJECT_SOURCES})
target_link_libraries(test_application_logic
    Qt5::Widgets
    Qt5::Test
    ${SQLITE3_LIBRARIES}
)
target_include_directories(test_application_logic PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_compile_options(test_application_logic PRIVATE ${SQLITE3_CFLAGS_OTHER})
set_target_properties(test_application_logic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Register tests with CTest
enable_testing()
add_test(NAME database_tests COMMAND ${CMAKE_BINARY_DIR}/bin/test_db_manager)
add_test(NAME analytics_tests COMMAND ${CMAKE_BINARY_DIR}/bin/test_analytics)
add_test(NAME application_logic_tests COMMAND ${CMAKE_BINARY_DIR}/bin/test_application_logic)

# Create a convenience target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_db_manager test_analytics test_application_logic
    COMMENT "Running all tests"
)