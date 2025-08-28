# CV Tracker Makefile

.PHONY: all build clean test test-clean install help

# Default target
all: build

# Build the main application
build:
	@echo "Building APP Tracker..."
	@mkdir -p build
	@cd build && cmake .. && make -j$$(nproc)
	@echo "✅ Build complete! Executable: build/bin/appTracker"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build
	@rm -rf build_tests
	@echo "✅ Clean complete!"

# Run tests
test:
	@echo "Running test suite..."
	@chmod +x ./run_tests.sh
	@./run_tests.sh

# Clean only test artifacts
test-clean:
	@echo "Cleaning test artifacts..."
	@rm -rf build_tests
	@echo "✅ Test clean complete!"

# Build with tests enabled
build-with-tests:
	@echo "Building APP Tracker with tests..."
	@mkdir -p build
	@cd build && cmake -DBUILD_TESTS=ON .. && make -j$$(nproc)
	@echo "✅ Build with tests complete!"

# Install (placeholder for future package installation)
install: build
	@echo "Installing APP Tracker..."
	@echo "⚠️  Installation not yet implemented"

# Run application
run: build
	@echo "Running APP Tracker..."
	@./build/bin/appTracker

# Development setup
dev-setup:
	@echo "Setting up development environment..."
	@sudo apt-get update
	@sudo apt-get install -y \
		build-essential \
		cmake \
		qt5-default \
		qtbase5-dev \
		qtbase5-dev-tools \
		libsqlite3-dev \
		pkg-config
	@echo "✅ Development environment ready!"

# Help target
help:
	@echo "APP Tracker Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  build           - Build the main application"
	@echo "  clean           - Clean all build artifacts"
	@echo "  test            - Run the test suite"
	@echo "  test-clean      - Clean only test artifacts"
	@echo "  build-with-tests- Build with tests enabled"
	@echo "  run             - Build and run the application"
	@echo "  dev-setup       - Install development dependencies"
	@echo "  install         - Install the application (not implemented)"
	@echo "  help            - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make build      # Build the application"
	@echo "  make test       # Run all tests"
	@echo "  make clean      # Clean everything"
	@echo "  make run        # Build and run"