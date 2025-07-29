#!/bin/bash

# CV Tracker Test Runner Script

# Don't exit on error so we can properly collect all test results
set +e

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

# Use set -e for build steps as we want to exit if build fails
set -e
echo "Configuring test build with CMake..."
cmake ../tests

echo "Building tests..."
make -j$(nproc)
set +e  # Disable exit on error for test runs

echo
echo "=== Running Tests ==="
echo

# Set initial test status
DB_TEST_RESULT=0
ANALYTICS_TEST_RESULT=0
LOGIC_TEST_RESULT=0

# Function to run tests and extract detailed information
run_test() {
    local test_exe=$1
    local test_name=$2
    local output_file="${test_name}_output.log"
    
    echo "Executing $test_name..."
    $test_exe -v2 | tee $output_file
    local result=$?
    
    # Extract test summary information
    local passed=$(grep -o '[0-9]* passed' $output_file | awk '{print $1}')
    local failed=$(grep -o '[0-9]* failed' $output_file | awk '{print $1}')
    local skipped=$(grep -o '[0-9]* skipped' $output_file | awk '{print $1}')
    
    # Save detailed information
    echo "$test_name: $passed passed, $failed failed, $skipped skipped (Exit code: $result)" >> test_summary.txt
    
    # Extract failed test details for later reporting
    if [ "$result" -ne 0 ]; then
        echo "Failed tests in $test_name:" >> failed_tests.txt
        grep -B 1 -A 3 "FAIL" $output_file >> failed_tests.txt
        echo "" >> failed_tests.txt
    fi
    
    return $result
}

# Check if test executables exist
if [ -d "./bin" ] && [ -f "./bin/test_db_manager" ] && [ -f "./bin/test_analytics" ] && [ -f "./bin/test_application_logic" ]; then
    # Run each test suite individually
    rm -f test_summary.txt failed_tests.txt
    
    run_test "./bin/test_db_manager" "Database manager tests"
    DB_TEST_RESULT=$?
    
    run_test "./bin/test_analytics" "Analytics tests"
    ANALYTICS_TEST_RESULT=$?
    
    run_test "./bin/test_application_logic" "Application logic tests"
    LOGIC_TEST_RESULT=$?
    
    # Check overall test results
    TEST_RESULT=$(( $DB_TEST_RESULT + $ANALYTICS_TEST_RESULT + $LOGIC_TEST_RESULT ))
    
    echo
    echo "=== Test Summary ==="
    cat test_summary.txt
    
    echo
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✅ All tests passed successfully!"
    else
        echo "❌ Some tests failed. Detailed failure information:"
        echo
        if [ -f failed_tests.txt ]; then
            cat failed_tests.txt
        fi
    fi
    
    echo
    echo "=== Test Information ==="
    echo "Test executables located in: ./bin/"
    echo "Build directory: $TEST_BUILD_DIR"
    echo "Overall exit code: $TEST_RESULT"
    echo
    echo "To run tests manually:"
    echo "  cd $TEST_BUILD_DIR"
    echo "  ./bin/test_db_manager"
    echo "  ./bin/test_analytics"
    echo "  ./bin/test_application_logic"
    echo
    echo "To run all tests with CTest:"
    echo "  cd $TEST_BUILD_DIR"
    echo "  make run_all_tests"
    
    exit $TEST_RESULT
else
    echo "❌ Test executables not found!"
    echo "Build may have failed. Check the build output above."
    exit 1
fi