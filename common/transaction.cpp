#include "transaction.h"
#include "utils.h"
#include <sstream>
#include <iomanip>
#include <random>

Transaction::Transaction() : transaction_id(0), amount(0.0), merchant_id(0) {
    timestamp = getCurrentTimestamp();
}

Transaction::Transaction(long id, const std::string& card, double amt, int merchant, const std::string& loc)
    : transaction_id(id), card_number(card), amount(amt), merchant_id(merchant), location(loc) {
    timestamp = getCurrentTimestamp();
}

std::string Transaction::serialize() const {
    std::ostringstream oss;
    oss << transaction_id << "|" << card_number << "|" << std::fixed << std::setprecision(2) 
        << amount << "|" << timestamp << "|" << merchant_id << "|" << location;
    return oss.str();
}

Transaction Transaction::deserialize(const std::string& data) {
    Transaction t;
    std::istringstream iss(data);
    std::string token;
    
    // Parse pipe-separated values
    if (std::getline(iss, token, '|')) t.transaction_id = std::stol(token);
    if (std::getline(iss, token, '|')) t.card_number = token;
    if (std::getline(iss, token, '|')) t.amount = std::stod(token);
    if (std::getline(iss, token, '|')) t.timestamp = token;
    if (std::getline(iss, token, '|')) t.merchant_id = std::stoi(token);
    if (std::getline(iss, token, '|')) t.location = token;
    
    return t;
}

bool Transaction::isValid() const {
    // Check amount is positive
    if (amount <= 0) return false;
    
    // Check card number using Luhn algorithm
    return Utils::luhnCheck(card_number);
}

std::string Transaction::getCurrentTimestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}