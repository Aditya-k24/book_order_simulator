#!/bin/bash

# Installation script for Low-Latency Order Book Simulator
# This script sets up the project and runs initial tests

echo "=========================================="
echo "  Installing Order Book Simulator"
echo "=========================================="

# Check if make is available
if ! command -v make &> /dev/null; then
    echo "❌ Make is not installed. Please install build-essential first:"
    echo "   Ubuntu/Debian: sudo apt-get install build-essential"
    echo "   macOS: xcode-select --install"
    exit 1
fi

# Check if g++ is available
if ! command -v g++ &> /dev/null; then
    echo "❌ g++ compiler is not installed. Please install a C++ compiler."
    exit 1
fi

echo "✅ Build tools found"

# Build the project
echo ""
echo "Building the simulator..."
if make clean && make; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

# Run tests
echo ""
echo "Running tests..."
if ./run_tests.sh; then
    echo "✅ All tests passed"
else
    echo "❌ Tests failed"
    exit 1
fi

# Make scripts executable
chmod +x demo.sh
chmod +x run_tests.sh

echo ""
echo "🎉 Installation completed successfully!"
echo ""
echo "Quick start:"
echo "  ./demo.sh              # Run a quick demonstration"
echo "  ./order_book_simulator --help    # See all options"
echo "  ./run_tests.sh         # Run tests"
echo ""
echo "Repository: https://github.com/Aditya-k24/book_order_simulator"
