# Low-Latency Order Book Simulator Makefile
# Optimized for high-frequency trading applications

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -mtune=native
CXXFLAGS += -ffast-math -funroll-loops -flto
CXXFLAGS += -DNDEBUG -DNOMINMAX

# Debug flags (uncomment for debugging)
# CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -DDEBUG

# Include directories
INCLUDES = -Iinclude

# Library directories and libraries
LDFLAGS = -pthread
LIBS = -lpthread

# Directories
SRC_DIR = src
BUILD_DIR = build
DATA_DIR = data
ANALYSIS_DIR = analysis

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Target executable
TARGET = order_book_simulator

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(DATA_DIR)
	mkdir -p $(ANALYSIS_DIR)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	rm -f *.csv
	rm -f *.log
	@echo "Clean complete"

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential cmake
	@echo "Dependencies installed"

# Run the simulator with default parameters
run: $(TARGET)
	./$(TARGET)

# Run with performance profiling
profile: $(TARGET)
	perf record --call-graph dwarf ./$(TARGET)
	perf report

# Run with valgrind for memory checking
memcheck: $(TARGET)
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Run with thread sanitizer (debug build)
tsan: CXXFLAGS += -fsanitize=thread -g -O1
tsan: clean $(TARGET)
	./$(TARGET)

# Run with address sanitizer (debug build)
asan: CXXFLAGS += -fsanitize=address -g -O1
asan: clean $(TARGET)
	./$(TARGET)

# Benchmark different data structures
benchmark: $(TARGET)
	@echo "Running benchmark tests..."
	./$(TARGET) --benchmark

# Generate documentation
docs:
	doxygen Doxyfile
	@echo "Documentation generated in docs/"

# Package the project
package: clean
	tar -czf order_book_simulator.tar.gz \
		--exclude='*.o' \
		--exclude='*.csv' \
		--exclude='*.log' \
		--exclude='$(BUILD_DIR)' \
		--exclude='.git' \
		--exclude='*.tar.gz' \
		.
	@echo "Package created: order_book_simulator.tar.gz"

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build the simulator (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  run          - Build and run the simulator"
	@echo "  profile      - Run with performance profiling"
	@echo "  memcheck     - Run with memory leak checking"
	@echo "  tsan         - Run with thread sanitizer"
	@echo "  asan         - Run with address sanitizer"
	@echo "  benchmark    - Run benchmark tests"
	@echo "  docs         - Generate documentation"
	@echo "  package      - Create distribution package"
	@echo "  install-deps - Install system dependencies"
	@echo "  help         - Show this help message"

# Phony targets
.PHONY: all clean run profile memcheck tsan asan benchmark docs package install-deps help

# Dependency tracking
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MM $< -MT $(BUILD_DIR)/$*.o -MF $@
