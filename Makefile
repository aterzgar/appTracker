# Simple wrapper around CMake for convenient development.
#
# Usage:
#   make          – configure + build
#   make run      – build + run the app
#   make test     – build + run tests
#   make clean    – remove build directory
#   make rebuild  – clean + full rebuild
#

BUILD_DIR = build

.PHONY: all run test clean rebuild

all: configure build

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

build:
	@$(MAKE) -C $(BUILD_DIR)

run: all
	@echo ""
	@echo "▶ Running appTracker..."
	@$(BUILD_DIR)/bin/appTracker

test:
	@mkdir -p build_tests
	@cd build_tests && cmake -DBUILD_TESTS=ON ..
	@$(MAKE) -C build_tests
	@cd build_tests && ctest --output-on-failure

clean:
	@rm -rf $(BUILD_DIR) build_tests

rebuild: clean all
