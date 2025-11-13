#!/bin/bash

# Benchmark throughput with different numbers of consumers
# This script properly measures messages/second

echo "=== Distributed System Throughput Benchmark ==="
echo ""

# Kill any existing processes
pkill -f broker_exe 2>/dev/null
pkill -f consumer_exe 2>/dev/null
pkill -f producer_exe 2>/dev/null
sleep 1

# Number of transactions for testing
NUM_TRANS=100000

echo "Test configuration:"
echo "  - Transactions: $NUM_TRANS"
echo "  - Delay: 0ms (maximum throughput)"
echo ""

run_test() {
    local NUM_CONSUMERS=$1
    
    echo "=========================================="
    echo "Test: $NUM_CONSUMERS consumer(s)"
    echo "=========================================="
    
    # Clean log
    rm -f broker_log.txt
    
    # Start broker
    ./broker_exe 9100 9200 8081 > /dev/null 2>&1 &
    local BROKER_PID=$!
    sleep 1
    
    # Start consumers and redirect their output to temp files
    local CONSUMER_PIDS=()
    for i in $(seq 1 $NUM_CONSUMERS); do
        ./consumer_exe --connect 127.0.0.1 9200 > consumer_${i}.log 2>&1 &
        CONSUMER_PIDS+=($!)
    done
    sleep 1
    
    echo "Starting producer (send phase)..."
    local START_SEND=$(date +%s)
    
    # Run producer (no delay for max throughput)
    ./producer_exe 127.0.0.1 9100 0 > /dev/null 2>&1
    
    local END_SEND=$(date +%s)
    local SEND_TIME=$((END_SEND - START_SEND))
    
    echo "  Producer finished in ${SEND_TIME}s"
    echo "  Waiting for consumers to finish processing..."
    
    # Wait for consumers to finish (check their logs for completion)
    local START_WAIT=$(date +%s)
    while true; do
        # Check if all consumers are done (look for "completed successfully")
        local DONE_COUNT=0
        for i in $(seq 1 $NUM_CONSUMERS); do
            if grep -q "completed successfully" consumer_${i}.log 2>/dev/null; then
                ((DONE_COUNT++))
            fi
        done
        
        if [ $DONE_COUNT -eq $NUM_CONSUMERS ]; then
            break
        fi
        
        # Timeout after 30 seconds
        local NOW=$(date +%s)
        if [ $((NOW - START_WAIT)) -gt 30 ]; then
            echo "  Warning: Timeout waiting for consumers"
            break
        fi
        
        sleep 0.5
    done
    
    local END_WAIT=$(date +%s)
    local TOTAL_TIME=$((END_WAIT - START_SEND))
    
    # Calculate throughput
    if [ $TOTAL_TIME -eq 0 ]; then
        TOTAL_TIME=1  # Avoid division by zero
    fi
    local THROUGHPUT=$((NUM_TRANS / TOTAL_TIME))
    
    echo "Results:"
    echo "  - Total time (send + process): ${TOTAL_TIME}s"
    echo "  - Overall throughput: ~${THROUGHPUT} transactions/sec"
    echo "  - Per consumer: ~$((THROUGHPUT / NUM_CONSUMERS)) transactions/sec"
    echo ""
    
    # Stop everything
    kill ${CONSUMER_PIDS[@]} 2>/dev/null
    kill $BROKER_PID 2>/dev/null
    wait 2>/dev/null
    
    # Clean up temp files
    rm -f consumer_*.log
    
    sleep 2
}

# Run tests with different numbers of consumers
run_test 1
run_test 2
run_test 4

echo "=== Benchmark Complete ==="
echo ""
echo "Summary:"
echo "The throughput should increase proportionally with the number of consumers."
echo "If not, there may be a bottleneck in the network or processing logic."
