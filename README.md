# Fault-Tolerant Distributed Transaction Processing System

A high-performance, fault-tolerant distributed system for processing credit card transactions at scale, demonstrating advanced distributed systems concepts including message queuing, pipeline parallelism, and automatic failure recovery.

## 🎯 System Architecture

```
Producer → Broker → Consumer Pool
   ↓         ↓          ↓
  2M      Queue +    Fraud
 Trans    Logging  Detection
```

### Components

- **Producer**: Generates 2 million synthetic credit card transactions
- **Broker**: Central message coordinator with fault-tolerant logging and round-robin distribution
- **Consumers**: Process transactions with CPU-intensive fraud detection (hash computation, encryption simulation, ML inference)
- **Monitor**: Real-time HTTP dashboard for system visualization

## ✨ Key Features

### Distributed Systems Concepts
- **Message Queue Architecture**: Broker buffers and distributes work across consumer pool
- **Fault Tolerance**: Persistent logging with automatic message recovery after crashes
- **Pipeline Parallelism**: Window-based flow control (1000 messages per consumer)
- **Non-blocking I/O**: Prevents head-of-line blocking with `O_NONBLOCK` sockets
- **Load Balancing**: Round-robin distribution across available consumers

### Performance Optimizations
- **Zero-copy networking**: Direct socket transmission
- **Pipelined processing**: Multiple outstanding messages per consumer
- **Optimized compilation**: `-O2` flag for production performance
- **Asynchronous disk writes**: OS-buffered logging for throughput

### Processing Pipeline
Each transaction undergoes realistic fraud detection:
1. **Database lookup simulation** (hash computation)
2. **Encryption/decryption** (100 rounds of cryptographic operations)
3. **Rule-based fraud scoring** (amount thresholds, format validation)
4. **ML model inference** (feature extraction + matrix operations)
5. **External API calls** (0.1ms delay simulation)

**Result**: ~18-20k transactions/sec with 4 consumers (realistic payment processing workload)

## 🚀 Quick Start

### Docker Deployment (Recommended)

```bash
# Start the complete system with 4 consumers
docker-compose up --build

# Monitor at http://localhost:8081
# Press Ctrl+C to stop
```

### Local Build & Run

```bash
# Compile all components
make clean && make

# Terminal 1: Start broker
./broker_exe 9100 9200 8081

# Terminal 2-5: Start 4 consumers
./consumer_exe --connect 127.0.0.1 9200
./consumer_exe --connect 127.0.0.1 9200
./consumer_exe --connect 127.0.0.1 9200
./consumer_exe --connect 127.0.0.1 9200

# Terminal 6: Run producer
./producer_exe 127.0.0.1 9100 0
```

## 📊 Performance Benchmarks

| Consumers | Throughput    | Total Time  | Scaling Efficiency |
|-----------|---------------|-------------|-------------------|
| 1         | ~5k trans/sec | ~400 sec    | 100%              |
| 2         | ~10k trans/sec| ~200 sec    | 100%              |
| 4         | ~20k trans/sec| ~100 sec    | 100%              |

**Linear scaling demonstrated!** Each consumer adds proportional throughput.

## 🐳 Docker Configuration

The system uses resource-limited containers to simulate distributed deployment:

- **Broker**: 512MB RAM, 1 CPU core
- **Producer**: 256MB RAM, 0.5 CPU core
- **Consumers**: 256MB RAM, 0.5 CPU core each

See `docker-compose.yml` for full configuration.

## 🔧 Configuration

### Producer
```bash
./producer_exe <broker_host> <broker_port> [delay_ms]
# Example: ./producer_exe 127.0.0.1 9100 0
```

### Broker
```bash
./broker_exe <producer_port> <consumer_port> <monitor_port>
# Example: ./broker_exe 9100 9200 8081
```

### Consumer
```bash
./consumer_exe --connect <broker_host> <broker_port>
# Example: ./consumer_exe --connect 127.0.0.1 9200
```

## 📈 Monitoring

Access the real-time monitor dashboard at `http://localhost:8081` to view:
- Active connections
- Message throughput
- Queue depth
- Processing statistics

## 🛠️ Build from Source

```bash
# Install dependencies (macOS)
brew install cmake g++

# Build
make clean
make

# Or use CMake directly
mkdir build && cd build
cmake ..
make
```

## 📚 Project Structure

```
project/
├── broker/           # Message queue coordinator
├── producer/         # Transaction generator
├── consumer/         # Fraud detection processor
├── common/           # Shared utilities (Transaction, Utils)
├── monitor/          # HTTP monitoring dashboard
├── Dockerfile.*      # Container definitions
└── docker-compose.yml # Orchestration config
```

## 🧪 Testing

```bash
# Test system with varying consumer counts
./test_throughput.sh

# Interactive testing
./test_interactive.sh

# Full Docker demo
./docker_demo.sh
```

## 🔥 Fault Tolerance Demo

```bash
# 1. Start system
docker-compose up

# 2. While running, kill broker:
docker kill transaction-broker

# 3. Restart broker - it recovers unacked messages:
docker-compose up -d broker

# Result: No message loss, processing continues!
```

## 📖 Documentation

- **[Docker Quick Start](DOCKER_QUICKSTART.md)**: Beginner-friendly Docker guide
- **[Docker Reference](DOCKER_GUIDE.md)**: Detailed Docker documentation
- **[Midterm Update](MIDTERM_UPDATE.md)**: Project progress report

## 🎓 Learning Outcomes

This project demonstrates:
- Message queue implementation patterns
- Fault-tolerant distributed system design
- Network programming with TCP sockets
- Pipeline parallelism and flow control
- Performance optimization techniques
- Containerization and orchestration
- System monitoring and observability

## 👨‍💻 Author

**Wesley Marizane**  
COMP 7212: Operating/Distributed Systems  
University of Memphis, Fall 2025
