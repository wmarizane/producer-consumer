#pragma once
#include <string>
#include <ctime>

struct Transaction {
    long transaction_id;
    std::string card_number;
    double amount;
    std::string timestamp;
    int merchant_id;
    std::string location;
    
    // Constructor
    Transaction();
    Transaction(long id, const std::string& card, double amt, int merchant, const std::string& loc);
    
    // Serialize to string for network transmission
    std::string serialize() const;
    
    // Deserialize from string
    static Transaction deserialize(const std::string& data);
    
    // Validate transaction (Luhn algorithm for card, amount > 0)
    bool isValid() const;
    
    // Generate timestamp string
    static std::string getCurrentTimestamp();
};