#include "../common/transaction.h"
#include "../common/utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

// Simulate CPU-intensive fraud detection scoring
static double compute_fraud_score(const Transaction& t) {
    // 1. Simulate database lookup via hash computation
    std::string key = t.card_number + std::to_string(t.amount) + t.timestamp;
    uint64_t hash = 0;
    for (char c : key) {
        hash = hash * 31 + c;  // Prime multiplier for good distribution
    }
    
    // 2. Simulate encryption/decryption work (100 rounds of hashing)
    for (int i = 0; i < 100; i++) {
        hash = hash * 1103515245 + 12345;  // Linear congruential generator
        hash ^= (hash >> 16);               // Bit mixing
    }
    
    // 3. Rule-based fraud scoring
    double fraud_score = 0.0;
    fraud_score += (t.amount > 10000) ? 0.3 : 0.0;      // Large transactions suspicious
    fraud_score += (t.amount < 1) ? 0.2 : 0.0;          // Micro-transactions suspicious
    fraud_score += (t.card_number.length() != 16) ? 0.5 : 0.0;  // Invalid format
    
    // Add complexity based on card digits (force CPU to work through string)
    for (size_t i = 0; i < t.card_number.length(); i++) {
        if (t.card_number[i] >= '0' && t.card_number[i] <= '9') {
            fraud_score += (t.card_number[i] - '0') * 0.001;
        }
    }
    
    // 4. Simulate ML model inference (simple matrix operations)
    double features[10];
    double weights[10] = {0.1, 0.2, 0.15, 0.3, 0.05, 0.1, 0.2, 0.15, 0.05, 0.1};
    for (int i = 0; i < 10; i++) {
        features[i] = (t.amount + i * 100) / 10000.0;
    }
    double ml_score = 0;
    for (int i = 0; i < 10; i++) {
        ml_score += features[i] * weights[i];
    }
    fraud_score += ml_score * 0.1;
    
    // 5. Simulate network delay for external API calls (e.g., credit bureau check)
    std::this_thread::sleep_for(std::chrono::microseconds(100)); // 0.1ms per transaction
    
    // Use hash in calculation to prevent optimization
    fraud_score += (hash % 100) * 0.0001;
    
    return fraud_score;
}

struct Statistics {
    int total_transactions = 0;
    int valid_transactions = 0;
    int invalid_transactions = 0;
    double total_amount = 0.0;
    double valid_amount = 0.0;
    
    void print() const {
        std::cout << "\n=== Transaction Statistics ===" << std::endl;
        std::cout << "Total Transactions: " << total_transactions << std::endl;
        std::cout << "Valid Transactions: " << valid_transactions 
                  << " (" << (total_transactions > 0 ? (valid_transactions * 100.0 / total_transactions) : 0) 
                  << "%)" << std::endl;
        std::cout << "Invalid Transactions: " << invalid_transactions 
                  << " (" << (total_transactions > 0 ? (invalid_transactions * 100.0 / total_transactions) : 0) 
                  << "%)" << std::endl;
        std::cout << "Total Amount: $" << std::fixed << std::setprecision(2) << total_amount << std::endl;
        std::cout << "Valid Amount: $" << std::fixed << std::setprecision(2) << valid_amount << std::endl;
        std::cout << "Average Transaction: $" << std::fixed << std::setprecision(2) 
                  << (total_transactions > 0 ? total_amount / total_transactions : 0) << std::endl;
        std::cout << "Average Valid Transaction: $" << std::fixed << std::setprecision(2) 
                  << (valid_transactions > 0 ? valid_amount / valid_transactions : 0) << std::endl;
    }
};

// Process a single transaction line and update stats; returns true if processed
static bool process_line(const std::string& line, Statistics& stats, std::vector<Transaction>& invalidTransactions, int lineNumber) {
    if (line.empty()) return false;
    try {
        Transaction t = Transaction::deserialize(line);
        stats.total_transactions++;
        stats.total_amount += t.amount;
        
        // Perform CPU-intensive fraud detection
        double fraud_score = compute_fraud_score(t);
        bool passed_fraud_check = (fraud_score < 0.8);  // Threshold for fraud detection
        
        if (t.isValid() && passed_fraud_check) {
            stats.valid_transactions++;
            stats.valid_amount += t.amount;
        } else {
            stats.invalid_transactions++;
            invalidTransactions.push_back(t);
        }
        if (stats.total_transactions % 50000 == 0) {
            std::cout << "  Processed " << stats.total_transactions << " transactions..." << std::endl;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing line " << lineNumber << ": " << e.what() << std::endl;
        return false;
    }
}

// Run as TCP server on given port, read newline-delimited records, send "ACK\n"
static int run_server(uint16_t port) {
    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(server_fd); return 1; }
    if (listen(server_fd, 5) < 0) { perror("listen"); close(server_fd); return 1; }
    std::cout << "Listening on 0.0.0.0:" << port << " ..." << std::endl;

    Statistics stats;
    std::vector<Transaction> invalids;
    int client_fd;
    sockaddr_in cli{}; socklen_t clilen = sizeof(cli);
    client_fd = accept(server_fd, (sockaddr*)&cli, &clilen);
    if (client_fd < 0) { perror("accept"); close(server_fd); return 1; }
    std::cout << "Client connected: " << inet_ntoa(cli.sin_addr) << ":" << ntohs(cli.sin_port) << std::endl;

    std::string buffer;
    buffer.reserve(8192);
    char buf[1024];
    int lineNumber = 0;
    while (true) {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n < 0) { perror("recv"); break; }
        if (n == 0) { break; } // EOF
        buffer.append(buf, buf + n);
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            lineNumber++;
            bool ok = process_line(line, stats, invalids, lineNumber);
            // Send ACK for each received line regardless of valid/invalid
            const char* ack = ok ? "ACK\n" : "ERR\n";
            send(client_fd, ack, strlen(ack), 0);
        }
    }

    close(client_fd);
    close(server_fd);

    // Print statistics
    stats.print();
    if (!invalids.empty()) {
        std::cout << "\n=== Sample Invalid Transactions ===" << std::endl;
        int samplesToShow = std::min(5, (int)invalids.size());
        for (int i = 0; i < samplesToShow; i++) {
            const auto& t = invalids[i];
            std::cout << "ID: " << t.transaction_id 
                      << ", Card: " << t.card_number
                      << ", Amount: $" << t.amount;
            if (t.amount <= 0) {
                std::cout << " [Invalid: Amount <= 0]";
            } else if (!Utils::luhnCheck(t.card_number)) {
                std::cout << " [Invalid: Failed Luhn check]";
            }
            std::cout << std::endl;
        }
        if ((int)invalids.size() > samplesToShow) {
            std::cout << "... and " << (invalids.size() - samplesToShow) << " more invalid transactions" << std::endl;
        }
    }
    std::cout << "\nConsumer server completed successfully!" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Fault-Tolerant Distributed Consumer ===" << std::endl;

    // Socket server mode: --server <port>
    if (argc >= 3 && std::string(argv[1]) == "--server") {
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
        return run_server(port);
    }

    // Socket client mode: --connect <host> <port>
    if (argc >= 4 && std::string(argv[1]) == "--connect") {
        std::string host = argv[2];
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[3]));
        
        // Connect to broker consumer port and process pushed records
        int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) { perror("socket"); return 1; }
        
        // Resolve hostname using getaddrinfo (supports both IP addresses and hostnames like "broker")
        struct addrinfo hints{}, *result;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        std::string port_str = std::to_string(port);
        
        int rv = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
        if (rv != 0) {
            std::cerr << "getaddrinfo failed: " << gai_strerror(rv) << std::endl;
            close(sockfd);
            return 1;
        }
        
        if (connect(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
            perror("connect");
            freeaddrinfo(result);
            close(sockfd);
            return 1;
        }
        
        freeaddrinfo(result);
        std::cout << "Connected to broker at " << host << ":" << port << std::endl;

        Statistics stats; std::vector<Transaction> invalids; std::string buffer; buffer.reserve(8192);
        char buf[1024]; int lineNumber = 0;
        while (true) {
            ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
            if (n < 0) { perror("recv"); break; }
            if (n == 0) { break; }
            buffer.append(buf, buf + n);
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                lineNumber++;
                bool ok = process_line(line, stats, invalids, lineNumber);
                const char* ack = ok ? "ACK\n" : "ERR\n";
                send(sockfd, ack, strlen(ack), 0);
            }
        }
        close(sockfd);
        stats.print();
        if (!invalids.empty()) {
            std::cout << "\n=== Sample Invalid Transactions ===" << std::endl;
            int samplesToShow = std::min(5, (int)invalids.size());
            for (int i = 0; i < samplesToShow; i++) {
                const auto& t = invalids[i];
                std::cout << "ID: " << t.transaction_id 
                          << ", Card: " << t.card_number
                          << ", Amount: $" << t.amount;
                if (t.amount <= 0) {
                    std::cout << " [Invalid: Amount <= 0]";
                } else if (!Utils::luhnCheck(t.card_number)) {
                    std::cout << " [Invalid: Failed Luhn check]";
                }
                std::cout << std::endl;
            }
        }
        std::cout << "\nConsumer client completed successfully!" << std::endl;
        return 0;
    }

    // Default: file mode
    std::string inputFile = "transactions.txt";
    if (argc > 1) { inputFile = argv[1]; }
    std::cout << "Reading transactions from: " << inputFile << std::endl;

    std::ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file " << inputFile << std::endl;
        return 1;
    }

    Statistics stats;
    std::vector<Transaction> invalidTransactions;
    std::string line;
    int lineNumber = 0;
    std::cout << "\nProcessing transactions..." << std::endl;
    while (std::getline(inFile, line)) {
        lineNumber++;
        process_line(line, stats, invalidTransactions, lineNumber);
    }
    inFile.close();

    stats.print();
    if (!invalidTransactions.empty()) {
        std::cout << "\n=== Sample Invalid Transactions ===" << std::endl;
        int samplesToShow = std::min(5, (int)invalidTransactions.size());
        for (int i = 0; i < samplesToShow; i++) {
            const auto& t = invalidTransactions[i];
            std::cout << "ID: " << t.transaction_id 
                      << ", Card: " << t.card_number
                      << ", Amount: $" << t.amount;
            if (t.amount <= 0) {
                std::cout << " [Invalid: Amount <= 0]";
            } else if (!Utils::luhnCheck(t.card_number)) {
                std::cout << " [Invalid: Failed Luhn check]";
            }
            std::cout << std::endl;
        }
        if (invalidTransactions.size() > samplesToShow) {
            std::cout << "... and " << (invalidTransactions.size() - samplesToShow) 
                      << " more invalid transactions" << std::endl;
        }
    }
    std::cout << "\nConsumer completed successfully!" << std::endl;
    return 0;
}