#!/bin/bash

# Test throughput scaling with different numbers of consumers
# This script tests the system with 1, 2, and 4 consumers

echo "=== Throughput Scaling Test ==="
echo "This will test the system with 1, 2, and 4 consumers"
echo ""

# Clean up old log
rm -f broker_log.txt

# Number of transactions for quick test
NUM_TRANS=100000

for NUM_CONSUMERS in 1 2 4; do
    echo "========================================"
    echo "Testing with $NUM_CONSUMERS consumer(s)"
    echo "========================================"
    
    # Start broker
    ./broker_exe 9100 9200 8081 > broker_output.txt 2>&1 &
    BROKER_PID=$!
    sleep 1
    
    # Start consumers
    CONSUMER_PIDS=()
    for i in $(seq 1 $NUM_CONSUMERS); do
        CONSUMER_PORT=$((9300 + i))
        ./consumer_exe --connect 127.0.0.1 9200 > consumer_${i}_output.txt 2>&1 &
        CONSUMER_PIDS+=($!)
    done
    sleep 1
    
    # Start producer with delay to control rate
    echo "Starting producer with 1ms delay between messages..."
    START_TIME=$(date +%s)
    ./producer_exe 127.0.0.1 9100 1 > producer_output.txt 2>&1
    END_TIME=$(date +%s)
    
    ELAPSED=$((END_TIME - START_TIME))
    if [ $ELAPSED -gt 0 ]; then
        THROUGHPUT=$((NUM_TRANS / ELAPSED))
        echo "Time: ${ELAPSED}s, Throughput: ~${THROUGHPUT} trans/sec"
    fi
    
    # Give consumers time to finish
    sleep 2
    
    # Stop everything
    kill ${CONSUMER_PIDS[@]} 2>/dev/null
    kill $BROKER_PID 2>/dev/null
    wait 2>/dev/null
    
    echo ""
    sleep 2
done

echo "=== Test Complete ==="
echo "Check *_output.txt files for detailed logs"
