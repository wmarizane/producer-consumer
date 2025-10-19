#include "../common/transaction.h"
#include "../common/utils.h"
#include <iostream>
#include <vector>
#include <fstream>

int main() {
    std::cout << "=== Fault-Tolerant Distributed Producer ===" << std::endl;
    std::cout << "Generating sample transactions..." << std::endl;
    
    std::vector<Transaction> transactions;
    const int numTransactions = 100; // Start small for testing
    
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
    
    // Save to file for now (will be socket communication later)
    std::ofstream outFile("transactions.txt");
    if (outFile.is_open()) {
        for (const auto& t : transactions) {
            outFile << t.serialize() << std::endl;
        }
        outFile.close();
        std::cout << "\n" << numTransactions << " transactions saved to transactions.txt" << std::endl;
    }
    
    std::cout << "Producer completed successfully!" << std::endl;
    return 0;
}