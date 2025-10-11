#pragma once
#include <string>
#include <vector>

class Utils {
public:
    // Luhn algorithm for credit card validation
    static bool luhnCheck(const std::string& cardNumber);
    
    // Generate random credit card number (for testing)
    static std::string generateCreditCardNumber();
    
    // Generate random transaction ID
    static long generateTransactionId();
    
    // Generate random amount between min and max
    static double generateRandomAmount(double min = 1.0, double max = 1000.0);
    
    // Generate random merchant ID
    static int generateMerchantId();
    
    // Get random location from predefined list
    static std::string getRandomLocation();
    
private:
    static std::vector<std::string> locations;
};