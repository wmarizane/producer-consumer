# Simple Development Plan

## Overview
This plan breaks down the project into manageable commits that can be done over several weeks, making it realistic for a class project.

## Phase 1: Foundation (Week 1)
**Commit 1: Project setup** ✅
- Basic project structure
- CMakeLists.txt
- README.md

**Commit 2: Transaction data structure**
- `common/transaction.h` and `common/transaction.cpp`
- Basic transaction class with JSON-like data
- Simple utility functions

**Commit 3: Credit card validation**
- Luhn algorithm implementation
- Transaction validation functions

## Phase 2: Basic Communication (Week 2-3)  
**Commit 4: Simple producer**
- `producer/producer.cpp`
- Generate transactions and save to file
- Basic transaction generation logic

**Commit 5: Simple consumer**
- `consumer/consumer.cpp` 
- Read transactions from file and validate them
- Statistics reporting

**Commit 6: Socket communication**
- Replace file I/O with simple sockets
- Producer sends directly to consumer

## Phase 3: Add Broker (Week 4-5)
**Commit 7: Basic broker**
- `broker/broker.cpp`
- Accept connections from producers
- Forward to consumers

**Commit 8: Message queuing**
- In-memory queue in broker
- Handle multiple producers and consumers

## Phase 4: Fault Tolerance (Week 6-7)
**Commit 9: Message acknowledgments**
- ACK/NACK system between broker and consumer
- Retry unacknowledged messages

**Commit 10: Persistence**
- Broker saves messages to log files
- Recovery on restart

**Commit 11: Failure simulation**
- Graceful shutdown and crash simulation
- Test recovery mechanisms

## Phase 5: Testing (Week 8)
**Commit 12: Scale testing**
- Generate 100K+ transactions
- Performance measurements

**Commit 13: Final polish**
- Documentation updates
- Demo preparation

## Guidelines
- Each commit should be small and focused
- Test as you go - make sure each commit compiles and runs
- Document your progress in commit messages
- Take your time - this should span 8 weeks of work