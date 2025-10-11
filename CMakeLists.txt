cmake_minimum_required(VERSION 3.10)
project(FaultTolerantSystem)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags for debugging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread")

# Producer executable
add_executable(producer
    producer/producer.cpp
    common/transaction.cpp
    common/utils.cpp
)

# Broker executable  
add_executable(broker
    broker/broker.cpp
    common/transaction.cpp
    common/utils.cpp
)

# Consumer executable
add_executable(consumer
    consumer/consumer.cpp
    common/transaction.cpp
    common/utils.cpp
)