#!/bin/bash

# Docker Demo Script - Easy way to test the distributed system

echo "=== Docker Distributed System Demo ==="
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo "❌ Docker is not installed!"
    echo "Please install Docker Desktop from: https://www.docker.com/products/docker-desktop"
    exit 1
fi

# Check if Docker is running
if ! docker info &> /dev/null; then
    echo "❌ Docker is not running!"
    echo "Please start Docker Desktop and try again."
    exit 1
fi

echo "✓ Docker is installed and running"
echo ""

# Stop any existing containers
echo "Cleaning up old containers..."
docker-compose down 2>/dev/null
sleep 2

# Build images
echo ""
echo "Building Docker images (this may take a minute)..."
docker-compose build

echo ""
echo "=========================================="
echo "Starting distributed system with 4 consumers"
echo "=========================================="
echo ""
echo "This will start:"
echo "  - 1 Broker container"
echo "  - 1 Producer container"
echo "  - 4 Consumer containers"
echo ""
echo "Each container is isolated with CPU and memory limits,"
echo "simulating a true distributed environment!"
echo ""

# Start all services
docker-compose up -d

echo ""
echo "✓ All containers started!"
echo ""
echo "View logs in real-time:"
echo "  Broker:     docker logs -f transaction-broker"
echo "  Consumer 1: docker logs -f transaction-consumer-1"
echo "  Consumer 2: docker logs -f transaction-consumer-2"
echo ""
echo "View all containers: docker ps"
echo "View resource usage: docker stats"
echo ""
echo "Monitor dashboard: http://localhost:8081"
echo ""
echo "Press Ctrl+C when done, then run: docker-compose down"
echo ""

# Follow broker logs
echo "Following broker logs (Ctrl+C to exit):"
docker logs -f transaction-broker
