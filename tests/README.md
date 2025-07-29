# CV Tracker Test Suite

This directory contains comprehensive automated tests for the CV Application Tracker project.

## Test Structure

### Test Files

1. **test_db_manager.cpp** - Database layer tests
   - Database connection and initialization
   - CRUD operations (Create, Read, Update, Delete)
   - Data validation and integrity
   - Analytics queries
   - Edge cases and error handling

2. **test_application_logic.cpp** - Application logic and UI tests
   - Main window initialization
   - UI component verification
   - Dialog functionality
   - Button interactions
   - Table setup and configuration

3. **test_analytics.cpp** - Analytics functionality tests
   - Statistics calculations
   - Status distribution analysis
   - Interview and rejection rate calculations
   - Edge cases with various data scenarios

4. **test_main.cpp** - Test runner
   - Orchestrates all test suites
   - Provides unified test execution

## Test Categories

### Unit Tests
- **Database Operations**: Test individual database functions
- **Data Validation**: Test input validation and constraints
- **Analytics Calculations**: Test statistical computations

### Integration Tests
- **UI Components**: Test user interface elements
- **Workflow Tests**: Test complete user workflows
- **Dialog Interactions**: Test dialog functionality

### Edge Case Tests
- **Empty Database**: Test behavior with no data
- **Invalid Input**: Test error handling
- **Boundary Conditions**: Test limits and edge values

## Running Tests

### Quick Test Run
```bash
# From project root directory
./run_tests.sh
```

### Manual Test Build
```bash
# Create test build directory
mkdir build_tests
cd build_tests

# Configure and build tests
cmake ../tests
make

# Run tests
./cvTracker_tests
```

### Integration with Main Build
```bash
# Build main project with tests enabled
mkdir build
cd build
cmake -DBUILD_TESTS=ON ..
make
ctest  # Run tests through CTest
```

## Test Coverage

### Database Manager Tests (test_db_manager.cpp)

#### Connection Tests
- ✅ `testOpenDatabase()` - Valid database creation
- ✅ `testOpenInvalidDatabase()` - Invalid path handling
- ✅ `testCloseDatabase()` - Proper connection cleanup

#### CRUD Operations
- ✅ `testAddApplication()` - Adding valid applications
- ✅ `testAddInvalidApplication()` - Invalid data rejection
- ✅ `testAddDuplicateApplication()` - Duplicate prevention
- ✅ `testGetAllApplications()` - Data retrieval
- ✅ `testUpdateApplication()` - Record modification
- ✅ `testDeleteApplication()` - Record removal

#### Validation
- ✅ `testDateValidation()` - Date format validation
- ✅ `testDuplicateDetection()` - Duplicate entry detection

#### Analytics
- ✅ `testGetTotalApplications()` - Total count calculation
- ✅ `testGetApplicationsByStatus()` - Status-based filtering
- ✅ `testGetStatusCounts()` - Status distribution analysis

### Application Logic Tests (test_application_logic.cpp)

#### UI Initialization
- ✅ `testMainWindowInitialization()` - Window setup
- ✅ `testButtonsExist()` - Button presence verification
- ✅ `testTableSetup()` - Table configuration

#### Dialog Tests
- ✅ `testAddApplicationDialog()` - Add dialog functionality
- ✅ `testEditApplicationDialog()` - Edit dialog functionality
- ✅ `testAnalyticsDialog()` - Analytics dialog functionality

#### Workflow Tests
- ✅ `testAddApplicationWorkflow()` - Add application flow
- ✅ `testEditApplicationWorkflow()` - Edit application flow
- ✅ `testDeleteApplicationWorkflow()` - Delete application flow
- ✅ `testRefreshFunctionality()` - Data refresh functionality

### Analytics Tests (test_analytics.cpp)

#### Basic Analytics
- ✅ `testEmptyDatabaseAnalytics()` - Empty state handling
- ✅ `testSingleApplicationAnalytics()` - Single record analysis
- ✅ `testMultipleApplicationsAnalytics()` - Multiple records analysis

#### Statistical Calculations
- ✅ `testStatusDistributionAnalytics()` - Status breakdown
- ✅ `testInterviewCalculations()` - Interview rate calculations
- ✅ `testRejectionRateCalculations()` - Rejection rate analysis
- ✅ `testOfferSuccessRate()` - Success rate calculations

#### Edge Cases
- ✅ `testAnalyticsWithAllSameStatus()` - Uniform status handling
- ✅ `testAnalyticsWithUnknownStatus()` - Custom status handling
- ✅ `testAnalyticsAfterDeletion()` - Post-deletion analytics
- ✅ `testAnalyticsAfterUpdate()` - Post-update analytics

## Test Data

Tests use temporary databases and mock data to ensure:
- **Isolation**: Each test runs independently
- **Repeatability**: Tests produce consistent results
- **Safety**: No interference with production data

### Sample Test Data
```cpp
Application testApp;
testApp.companyName = "Test Company";
testApp.applicationDate = "2024-01-15";
testApp.position = "Software Engineer";
testApp.contactPerson = "John Doe";
testApp.status = "Applied";
testApp.notes = "Test notes";
```

## Continuous Integration

The test suite is designed for CI/CD integration:

### GitHub Actions Example
```yaml
- name: Run Tests
  run: |
    chmod +x ./run_tests.sh
    ./run_tests.sh
```

### Test Exit Codes
- `0` - All tests passed
- `Non-zero` - Test failures occurred

## Adding New Tests

### Creating a New Test Class
```cpp
class TestNewFeature : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();    // Setup before all tests
    void init();           // Setup before each test
    void cleanup();        // Cleanup after each test
    void cleanupTestCase(); // Cleanup after all tests
    
    void testNewFunction(); // Your test method

private:
    // Test data and helper methods
};
```

### Test Naming Convention
- Test files: `test_<component>.cpp`
- Test classes: `Test<ComponentName>`
- Test methods: `test<FunctionName>()`

### Best Practices
1. **Arrange-Act-Assert**: Structure tests clearly
2. **Independent Tests**: Each test should be self-contained
3. **Descriptive Names**: Use clear, descriptive test names
4. **Edge Cases**: Test boundary conditions and error cases
5. **Mock Data**: Use temporary/mock data for isolation

## Troubleshooting

### Common Issues

#### Qt Test Framework Not Found
```bash
# Install Qt5 Test module
sudo apt-get install qtbase5-dev-tools
```

#### SQLite Not Found
```bash
# Install SQLite development libraries
sudo apt-get install libsqlite3-dev
```

#### Build Failures
- Ensure all dependencies are installed
- Check CMake version (minimum 3.10)
- Verify Qt5 installation

### Debug Mode
```bash
# Build tests in debug mode
cmake -DCMAKE_BUILD_TYPE=Debug ../tests
make
```

### Verbose Test Output
```bash
# Run tests with verbose output
./cvTracker_tests -v2
```

## Performance Considerations

- Tests use temporary databases for speed
- Parallel test execution is supported
- Memory usage is optimized for CI environments
- Test duration is typically under 30 seconds

## Future Enhancements

Planned test improvements:
- [ ] Performance benchmarking tests
- [ ] Memory leak detection
- [ ] UI automation tests with QTest mouse/keyboard simulation
- [ ] Load testing with large datasets
- [ ] Cross-platform compatibility tests