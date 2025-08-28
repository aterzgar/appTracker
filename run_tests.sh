#!/bin/bash

# APP Tracker Test Runner Script

# Don't exit on error so we can properly collect all test results
set +e

echo "=== APP Tracker Test Suite ==="
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
# Function to run tests and extract detailed information
run_test() {
    local test_exe=$1
    local test_name=$2
    local output_file="${test_name// /_}_output.log"  # Replace spaces with underscores
    
    echo "Executing $test_name..."
    $test_exe -v2 | tee "$output_file"
    local result=$?
    
    # Extract test summary information from the "Totals:" line
    local totals_line=$(grep "^Totals:" "$output_file")
    local passed=0
    local failed=0
    local skipped=0
    
    if [ -n "$totals_line" ]; then
        passed=$(echo "$totals_line" | grep -o '[0-9]\+ passed' | grep -o '[0-9]\+')
        failed=$(echo "$totals_line" | grep -o '[0-9]\+ failed' | grep -o '[0-9]\+')
        skipped=$(echo "$totals_line" | grep -o '[0-9]\+ skipped' | grep -o '[0-9]\+')
        
        # Set defaults if not found
        passed=${passed:-0}
        failed=${failed:-0}
        skipped=${skipped:-0}
    fi
    
    # Save detailed information
    echo "$test_name: $passed passed, $failed failed, $skipped skipped (Exit code: $result)" >> test_summary.txt
    
    # Extract failed test details for later reporting
    if [ "$result" -ne 0 ]; then
        echo "Failed tests in $test_name:" >> failed_tests.txt
        grep -B 1 -A 3 "FAIL" "$output_file" >> failed_tests.txt
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
    echo "┌─────────────────────────────────────────────────────────────┐"
    echo "│                    📋 What's Next?                          │"
    echo "└─────────────────────────────────────────────────────────────┘"
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "🎉 Great job! All tests are passing."
        echo "   Your code is ready for the next step."
    else
        echo "🔧 Some tests need attention. Here's how to debug:"
        echo ""
        echo "   📍 Quick Fix Options:"
        echo "   • Review the failure details above"
        echo "   • Check the specific test files mentioned in the errors"
        echo "   • Run individual test suites to isolate issues"
    fi
    
    echo ""
    echo "🚀 Available Commands:"
    echo ""
    echo "   Run Individual Test Suites:"
    echo "   ├─ Database Tests:      cd $TEST_BUILD_DIR && ./bin/test_db_manager"
    echo "   ├─ Analytics Tests:     cd $TEST_BUILD_DIR && ./bin/test_analytics"
    echo "   └─ Application Tests:   cd $TEST_BUILD_DIR && ./bin/test_application_logic"
    echo ""
    echo "   Run All Tests (Alternative Methods):"
    echo "   ├─ With CTest:          cd $TEST_BUILD_DIR && make run_all_tests"
    echo "   └─ Re-run this script:  ./$(basename "$0")"
    echo ""
    echo "📁 Build Information:"
    echo "   • Test executables: $TEST_BUILD_DIR/bin/"
    echo "   • Log files: $TEST_BUILD_DIR/*_output.log"
    echo "   • Exit code: $TEST_RESULT"
    
    if [ $TEST_RESULT -ne 0 ]; then
        echo ""
        echo "💡 Debugging Tips:"
        echo "   • Check test log files for detailed error messages"
        echo "   • Run tests with verbose output: ./bin/test_name -v2"
        echo "   • Focus on one failing test at a time"
    fi
    
    echo ""
    echo "────────────────────────────────────────────────────────────────"
    
    exit $TEST_RESULT
else
    echo "❌ Test executables not found!"
    echo "Build may have failed. Check the build output above."
    echo ""
    echo "💡 Troubleshooting:"
    echo "   • Make sure you're in the correct directory"
    echo "   • Check if CMake configuration succeeded"
    echo "   • Verify all dependencies are installed"
    echo "   • Try cleaning and rebuilding: rm -rf $TEST_BUILD_DIR"
    exit 1
fi