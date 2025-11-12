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
    std::cout << "Generating sample transactions..." << std::endl;
    
    std::vector<Transaction> transactions;
    const int numTransactions = 10000; // 10K transactions for testing
    
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
    if (argc == 3) {
        std::string host = argv[1];
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
        std::cout << "Connecting to broker at " << host << ":" << port << " ..." << std::endl;
        int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) { perror("socket"); return 1; }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid host: " << host << std::endl; close(sockfd); return 1;
        }
        if (connect(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); close(sockfd); return 1; }
        std::cout << "Connected. Streaming transactions..." << std::endl;

        char ackbuf[16];
        for (const auto& t : transactions) {
            std::string line = t.serialize();
            line.push_back('\n');
            if (send_all(sockfd, line.c_str(), line.size()) != 0) {
                std::cerr << "Failed to send transaction." << std::endl; break;
            }
            // Read ACK line (optional but nice)
            ssize_t n = recv(sockfd, ackbuf, sizeof(ackbuf)-1, 0);
            if (n > 0) { ackbuf[n] = '\0'; /* std::cout << "ACK: " << ackbuf; */ }
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