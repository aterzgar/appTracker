#!/bin/bash

# CV Tracker Test Runner Script

set -e  # Exit on any error

echo "=== CV Tracker Test Suite ==="
echo "Building and running automated tests..."
echo

# Create build directory for tests
TEST_BUILD_DIR="build_tests"
if [ -d "$TEST_BUILD_DIR" ]; then
    echo "Cleaning previous test build..."
    rm -rf "$TEST_BUILD_DIR"
fi

mkdir -p "$TEST_BUILD_DIR"
cd "$TEST_BUILD_DIR"

echo "Configuring test build with CMake..."
cmake ../tests

echo "Building tests..."
make -j$(nproc)

echo
echo "=== Running Tests ==="
echo

# Run the tests
if [ -f "./cvTracker_tests" ]; then
    echo "Executing database test suite..."
    ./cvTracker_tests -v2  # Verbose output
    TEST_RESULT=$?
    
    echo
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✅ All tests passed successfully!"
        echo
        echo "Test Summary:"
        echo "- Database connection tests: ✅"
        echo "- CRUD operations tests: ✅"
        echo "- Data validation tests: ✅"
        echo "- Analytics tests: ✅"
    else
        echo "❌ Some tests failed. Exit code: $TEST_RESULT"
        echo
        echo "Check the output above for details on failed tests."
    fi
    
    echo
    echo "=== Test Information ==="
    echo "Test executable: ./cvTracker_tests"
    echo "Build directory: $TEST_BUILD_DIR"
    echo "Exit code: $TEST_RESULT"
    echo
    echo "To run tests manually:"
    echo "  cd $TEST_BUILD_DIR"
    echo "  ./cvTracker_tests"
    
    exit $TEST_RESULT
else
    echo "❌ Test executable not found!"
    echo "Build may have failed. Check the build output above."
    exit 1
fi