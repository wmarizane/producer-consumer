#include "../common/transaction.h"
#include "../common/utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

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

int main(int argc, char* argv[]) {
    std::cout << "=== Fault-Tolerant Distributed Consumer ===" << std::endl;
    
    // Default input file
    std::string inputFile = "transactions.txt";
    
    // Allow command-line argument for input file
    if (argc > 1) {
        inputFile = argv[1];
    }
    
    std::cout << "Reading transactions from: " << inputFile << std::endl;
    
    // Open input file
    std::ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file " << inputFile << std::endl;
        return 1;
    }
    
    Statistics stats;
    std::vector<Transaction> invalidTransactions;
    std::string line;
    int lineNumber = 0;
    
    // Process transactions
    std::cout << "\nProcessing transactions..." << std::endl;
    
    while (std::getline(inFile, line)) {
        lineNumber++;
        
        if (line.empty()) continue;
        
        try {
            Transaction t = Transaction::deserialize(line);
            stats.total_transactions++;
            stats.total_amount += t.amount;
            
            if (t.isValid()) {
                stats.valid_transactions++;
                stats.valid_amount += t.amount;
            } else {
                stats.invalid_transactions++;
                invalidTransactions.push_back(t);
            }
            
            // Show progress every 10 transactions
            if (stats.total_transactions % 10 == 0) {
                std::cout << "  Processed " << stats.total_transactions << " transactions..." << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line " << lineNumber << ": " << e.what() << std::endl;
        }
    }
    
    inFile.close();
    
    // Print statistics
    stats.print();
    
    // Show sample invalid transactions if any
    if (!invalidTransactions.empty()) {
        std::cout << "\n=== Sample Invalid Transactions ===" << std::endl;
        int samplesToShow = std::min(5, (int)invalidTransactions.size());
        for (int i = 0; i < samplesToShow; i++) {
            const auto& t = invalidTransactions[i];
            std::cout << "ID: " << t.transaction_id 
                      << ", Card: " << t.card_number
                      << ", Amount: $" << t.amount;
            
            // Determine why it's invalid
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