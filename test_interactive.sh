#!/bin/bash

# Simple test to see throughput with different consumer counts

echo "=== Simple Throughput Test ==="
echo ""

# Kill any existing processes
pkill -f broker_exe
pkill -f consumer_exe
pkill -f producer_exe
sleep 1

# Clean slate
rm -f broker_log.txt

# Start broker
echo "Starting broker..."
./broker_exe 9100 9200 8081 &
BROKER_PID=$!
sleep 2

# Function to test with N consumers
test_with_consumers() {
    local N=$1
    echo ""
    echo "=== Testing with $N consumer(s) ==="
    
    # Start consumers
    for i in $(seq 1 $N); do
        ./consumer_exe --connect 127.0.0.1 9200 > /dev/null 2>&1 &
    done
    sleep 2
    
    echo "Consumers connected. Check broker output above."
    echo "Now run: ./producer_exe 127.0.0.1 9100 1"
    echo "Watch the [Stats] lines in broker output"
    echo ""
}

# Start with 1 consumer
test_with_consumers 1

echo "Press Enter to add another consumer..."
read

# Add one more consumer
./consumer_exe --connect 127.0.0.1 9200 > /dev/null 2>&1 &
echo "Added consumer 2. Now you have 2 consumers total."
echo ""

echo "Press Ctrl+C to stop all processes"
wait $BROKER_PID
