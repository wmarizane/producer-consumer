# Docker Quick Start Guide

## What is Docker?
Docker packages your application into "containers" - isolated environments that run independently. Think of them as lightweight virtual machines. Each container has its own CPU, memory, and network, simulating a truly distributed system.

## Installation

### macOS:
1. Download Docker Desktop: https://www.docker.com/products/docker-desktop
2. Install and start Docker Desktop
3. Wait for Docker to start (whale icon in menu bar should be steady)

### Verify Installation:
```bash
docker --version
docker-compose --version
```

## Running the Distributed System

### 1. Start Everything (Simple)
```bash
# Build and start all containers
docker-compose up --build

# This will:
# - Build broker, producer, and consumer images
# - Start 1 broker, 1 producer, 4 consumers
# - Each in their own isolated container
```

### 2. View Logs
```bash
# In another terminal, watch specific components:
docker logs -f transaction-broker      # Broker logs
docker logs -f transaction-consumer-1  # Consumer 1 logs
docker logs -f transaction-consumer-2  # Consumer 2 logs
```

### 3. Scale Consumers (Dynamic!)
```bash
# Add more consumers while running:
docker-compose up --scale consumer1=8 -d

# Or stop and restart with different count:
docker-compose down
docker-compose up --scale consumer1=2 --scale consumer2=2 --scale consumer3=2
```

### 4. Monitor
Open your browser to: http://localhost:8081
(The monitor dashboard will show the distributed topology)

### 5. Stop Everything
```bash
# Stop all containers
docker-compose down

# Stop and remove all data
docker-compose down -v
```

## Running with Different Configurations

### Test with 1 Consumer:
```bash
docker-compose up broker consumer1
```

### Test with 2 Consumers:
```bash
docker-compose up broker consumer1 consumer2
```

### Test with All 4 Consumers:
```bash
docker-compose up
```

## Useful Commands

### View Running Containers:
```bash
docker ps
```

### View Resource Usage:
```bash
docker stats
```

### Access Container Shell:
```bash
docker exec -it transaction-broker sh
```

### Clean Up Everything:
```bash
docker-compose down
docker system prune -a  # Remove all unused images/containers
```

## What's Different from Local?

### Local (Before Docker):
- All processes share CPU/memory/network
- Limited by single machine's loopback interface
- Hard to demonstrate true scaling

### Docker (Now):
- Each component isolated with resource limits
- Separate network interfaces
- Each container can be on different "machines" (or actually different physical machines!)
- True distributed system behavior

## Architecture

```
┌─────────────────────────────────────────────┐
│          Docker Network Bridge               │
│                                              │
│  ┌──────────┐         ┌──────────┐         │
│  │ Producer │────────▶│  Broker  │         │
│  │Container │         │Container │         │
│  └──────────┘         │(Port 9100│         │
│                       │Port 9200)│         │
│                       └────┬─────┘         │
│                            │                │
│         ┌──────────────────┼────────┐       │
│         │                  │        │       │
│    ┌────▼─────┐      ┌────▼─────┐ │       │
│    │Consumer 1│      │Consumer 2│ │       │
│    │Container │      │Container │ │       │
│    └──────────┘      └──────────┘ │       │
│         │                  │       │       │
│    ┌────▼─────┐      ┌────▼─────┐ │       │
│    │Consumer 3│      │Consumer 4│ │       │
│    │Container │      │Container │ │       │
│    └──────────┘      └──────────┘         │
└─────────────────────────────────────────────┘
```

## Troubleshooting

### "Cannot connect to Docker daemon"
- Make sure Docker Desktop is running

### "Port already in use"
- Stop local broker/consumer/producer: `pkill -f broker_exe`
- Or change ports in docker-compose.yml

### "Out of memory"
- Increase Docker Desktop memory limit in Preferences
- Reduce number of consumers

### Containers exit immediately:
- Check logs: `docker logs transaction-broker`
- Rebuild: `docker-compose build --no-cache`
