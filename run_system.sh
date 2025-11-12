#!/bin/bash
# Test script for distributed system with monitoring

echo "🚀 Starting Fault-Tolerant Distributed Transaction System with Monitoring"
echo ""
echo "📊 Monitor: http://localhost:8080"
echo "🔌 Broker ports: 9100 (producers), 9200 (consumers), 8081 (monitor)"
echo ""
echo "To test:"
echo "  Terminal 1: Already running broker"
echo "  Terminal 2: ./consumer_exe --connect 127.0.0.1 9200"
echo "  Terminal 3: ./consumer_exe --connect 127.0.0.1 9200"
echo "  Terminal 4: ./producer_exe 127.0.0.1 9100"
echo ""
echo "Starting broker..."

# Clean start
rm -f broker_log.txt

# Start broker with monitoring
./broker_exe 9100 9200 8081
