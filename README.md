# Fault-Tolerant Distributed Producer-Consumer System

## Project Overview
A fault-tolerant distributed producer-consumer system implemented in C++ that demonstrates key distributed systems concepts through a credit card transaction processing pipeline.

## Architecture
- **Producers**: Generate synthetic credit card transactions
- **Message Brokers**: Queue and distribute transactions with fault tolerance
- **Consumers**: Validate transactions and generate statistics

## Key Features
- Fault tolerance with node failure recovery
- Socket-based inter-process communication
- Persistent message queues
- Credit card validation using Luhn algorithm
- Scalable to 100K+ transactions

## Development Milestones

### Phase 1: Foundation (Current)
- [x] Project structure setup
- [ ] Basic transaction data structures
- [ ] Shared utilities and common classes

### Phase 2: Basic Communication
- [ ] Socket-based producer-consumer communication
- [ ] Simple message passing without brokers

### Phase 3: Message Broker Implementation
- [ ] Broker message queuing functionality
- [ ] Producer-to-broker communication
- [ ] Broker-to-consumer distribution

### Phase 4: Fault Tolerance
- [ ] Node failure simulation
- [ ] Message acknowledgment system
- [ ] Persistent logging and recovery

### Phase 5: Testing & Optimization
- [ ] Scale testing with 100K+ transactions
- [ ] Performance measurement and optimization
- [ ] Final integration and documentation

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

Instructions will be added as components are implemented

## Author

Wesley Marizane  
COMP 7212: Operating/Distributed Systems  
University of Memphis, Fall 2025
