#!/bin/bash

# Simple test script for the order book simulator
# Tests basic functionality and catches regressions

echo "Running Order Book Simulator Tests..."
echo "===================================="

# Test 1: Build test
echo "Test 1: Build test"
if make clean && make; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

# Test 2: Help command
echo ""
echo "Test 2: Help command"
if ./order_book_simulator --help > /dev/null 2>&1; then
    echo "✅ Help command works"
else
    echo "❌ Help command failed"
    exit 1
fi

# Test 3: Small simulation
echo ""
echo "Test 3: Small simulation (50 orders)"
if timeout 10s ./order_book_simulator --orders 50 --threads 1 --no-csv --no-perf > /dev/null 2>&1; then
    echo "✅ Small simulation completed"
else
    echo "❌ Small simulation failed"
    exit 1
fi

# Test 4: Benchmark mode
echo ""
echo "Test 4: Benchmark mode"
if timeout 15s ./order_book_simulator --benchmark > /dev/null 2>&1; then
    echo "✅ Benchmark mode works"
else
    echo "❌ Benchmark mode failed"
    exit 1
fi

echo ""
echo "🎉 All tests passed!"
echo "The order book simulator is working correctly."
