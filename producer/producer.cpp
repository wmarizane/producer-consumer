#include "../common/transaction.h"
#include "../common/utils.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

static int send_all(int fd, const char* data, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(fd, data + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Fault-Tolerant Distributed Producer ===" << std::endl;
    
    // Check for delay parameter
    int delay_ms = 0;  // Default: no delay
    if (argc >= 4) {
        delay_ms = std::stoi(argv[3]);
        std::cout << "Delay between messages: " << delay_ms << "ms" << std::endl;
    }
    
    std::cout << "Generating sample transactions..." << std::endl;
    
    std::vector<Transaction> transactions;
    const int numTransactions = 2000000; // 2 million transactions - good balance for demo
    
    // Generate transactions
    for (int i = 0; i < numTransactions; i++) {
        Transaction t(
            Utils::generateTransactionId(),
            Utils::generateCreditCardNumber(),
            Utils::generateRandomAmount(),
            Utils::generateMerchantId(),
            Utils::getRandomLocation()
        );
        transactions.push_back(t);
    }
    
    // Display some sample transactions
    std::cout << "\nSample transactions generated:" << std::endl;
    for (int i = 0; i < std::min(5, (int)transactions.size()); i++) {
        const auto& t = transactions[i];
        std::cout << "ID: " << t.transaction_id 
                  << ", Card: " << t.card_number.substr(0, 4) << "****"
                  << ", Amount: $" << t.amount
                  << ", Valid: " << (t.isValid() ? "YES" : "NO") << std::endl;
    }

    // If host and port are provided, stream to socket instead of file
    if (argc >= 3) {
        std::string host = argv[1];
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
        std::cout << "Connecting to broker at " << host << ":" << port << " ..." << std::endl;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
        std::cout << "Connected. Streaming transactions..." << std::endl;

        char ackbuf[16];
        int count = 0;
        for (const auto& t : transactions) {
            std::string line = t.serialize();
            line.push_back('\n');
            if (send_all(sockfd, line.c_str(), line.size()) != 0) {
                std::cerr << "Failed to send transaction." << std::endl; break;
            }
            // Don't wait for ACK - send as fast as possible
            // The broker will buffer and the TCP flow control will handle backpressure
            
            count++;
            if (count % 10000 == 0) {
                std::cout << "Sent " << count << " transactions..." << std::endl;
            }
            
            // Add delay if specified
            if (delay_ms > 0) {
                usleep(delay_ms * 1000);  // Convert ms to microseconds
            }
        }

        close(sockfd);
        std::cout << "\nFinished streaming " << numTransactions << " transactions to socket." << std::endl;
    } else {
        // Default: Save to file
        std::ofstream outFile("transactions.txt");
        if (outFile.is_open()) {
            for (const auto& t : transactions) {
                outFile << t.serialize() << std::endl;
            }
            outFile.close();
            std::cout << "\n" << numTransactions << " transactions saved to transactions.txt" << std::endl;
        }
    }
    
    std::cout << "Producer completed successfully!" << std::endl;
    return 0;
}