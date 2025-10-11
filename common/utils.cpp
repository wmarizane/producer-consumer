#include "utils.h"
#include <random>
#include <algorithm>
#include <ctime>

std::vector<std::string> Utils::locations = {"NY", "CA", "TX", "FL", "IL", "PA", "OH", "GA", "NC", "MI"};

bool Utils::luhnCheck(const std::string& cardNumber) {
    // Remove any spaces or dashes
    std::string cleanNumber;
    for (char c : cardNumber) {
        if (std::isdigit(c)) {
            cleanNumber += c;
        }
    }
    
    if (cleanNumber.length() < 13 || cleanNumber.length() > 19) {
        return false;
    }
    
    int sum = 0;
    bool alternate = false;
    
    // Process digits from right to left
    for (int i = cleanNumber.length() - 1; i >= 0; i--) {
        int digit = cleanNumber[i] - '0';
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) {
                digit = (digit % 10) + 1;
            }
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    return (sum % 10) == 0;
}

std::string Utils::generateCreditCardNumber() {
    // Generate a valid credit card number starting with 4 (Visa)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);
    
    std::string number = "4"; // Visa prefix
    
    // Add 14 more digits
    for (int i = 0; i < 14; i++) {
        number += std::to_string(dis(gen));
    }
    
    // Calculate check digit using Luhn algorithm
    int sum = 0;
    bool alternate = true;
    
    for (int i = number.length() - 1; i >= 0; i--) {
        int digit = number[i] - '0';
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) {
                digit = (digit % 10) + 1;
            }
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    int checkDigit = (10 - (sum % 10)) % 10;
    number += std::to_string(checkDigit);
    
    return number;
}

long Utils::generateTransactionId() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<long> dis(100000, 999999999);
    return dis(gen);
}

double Utils::generateRandomAmount(double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(min, max);
    return std::round(dis(gen) * 100.0) / 100.0; // Round to 2 decimal places
}

int Utils::generateMerchantId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(1, 999);
    return dis(gen);
}

std::string Utils::getRandomLocation() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<size_t> dis(0, locations.size() - 1);
    return locations[dis(gen)];
}