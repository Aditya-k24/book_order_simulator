#!/bin/bash

# Low-Latency Order Book Simulator - Demo Script
# This script demonstrates different simulation modes

echo "=========================================="
echo "  Low-Latency Order Book Simulator Demo"
echo "=========================================="

# Check if executable exists
if [ ! -f "./order_book_simulator" ]; then
    echo "Building simulator..."
    make clean && make
fi

echo ""
echo "1. Quick Demo (100 orders, 2 threads):"
echo "--------------------------------------"
./order_book_simulator --orders 100 --threads 2 --no-csv

echo ""
echo "2. Performance Test (1000 orders):"
echo "---------------------------------"
./order_book_simulator --orders 1000 --threads 4 --no-csv

echo ""
echo "3. High Fill Rate Simulation (aggressive orders):"
echo "------------------------------------------------"
./order_book_simulator --aggressive --orders 500 --no-csv

echo ""
echo "Demo completed! Check the output above for performance metrics."
echo "For more options, run: ./order_book_simulator --help"
