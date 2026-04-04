#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

/**
 * Simple C++ Calculator Example
 * Demonstrates basic C++ concepts including:
 * - Classes and objects
 * - STL containers (vector)
 * - Function overloading
 * - Input/output operations
 */

class Calculator {
private:
    std::vector<double> history;
    std::vector<std::string> operations;

public:
    // Basic arithmetic operations
    double add(double a, double b) {
        double result = a + b;
        recordOperation("addition", result);
        return result;
    }
    
    double subtract(double a, double b) {
        double result = a - b;
        recordOperation("subtraction", result);
        return result;
    }
    
    double multiply(double a, double b) {
        double result = a * b;
        recordOperation("multiplication", result);
        return result;
    }
    
    double divide(double a, double b) {
        if (b == 0.0) {
            std::cout << "Error: Division by zero!" << std::endl;
            return 0.0;
        }
        double result = a / b;
        recordOperation("division", result);
        return result;
    }
    
    // Advanced operations
    double power(double base, int exponent) {
        double result = 1.0;
        for (int i = 0; i < exponent; ++i) {
            result *= base;
        }
        recordOperation("power", result);
        return result;
    }
    
    // History management
    void showHistory() const {
        std::cout << "\n=== Calculation History ===" << std::endl;
        if (history.empty()) {
            std::cout << "No calculations performed yet." << std::endl;
            return;
        }
        
        for (size_t i = 0; i < history.size(); ++i) {
            std::cout << i + 1 << ". " << operations[i] 
                     << " = " << history[i] << std::endl;
        }
    }
    
    void clearHistory() {
        history.clear();
        operations.clear();
        std::cout << "History cleared." << std::endl;
    }
    
    double getLastResult() const {
        return history.empty() ? 0.0 : history.back();
    }
    
    size_t getCalculationCount() const {
        return history.size();
    }

private:
    void recordOperation(const std::string& op, double result) {
        history.push_back(result);
        operations.push_back(op);
    }
};

// Function to display menu
void displayMenu() {
    std::cout << "\n=== C++ Calculator ===" << std::endl;
    std::cout << "1. Addition" << std::endl;
    std::cout << "2. Subtraction" << std::endl;
    std::cout << "3. Multiplication" << std::endl;
    std::cout << "4. Division" << std::endl;
    std::cout << "5. Power" << std::endl;
    std::cout << "6. Show History" << std::endl;
    std::cout << "7. Clear History" << std::endl;
    std::cout << "8. Exit" << std::endl;
    std::cout << "Choose an operation (1-8): ";
}

int main() {
    Calculator calc;
    int choice;
    double a, b;
    int exponent;
    
    std::cout << "Welcome to the C++ Calculator!" << std::endl;
    
    // Demo calculations
    std::cout << "\nPerforming some demo calculations..." << std::endl;
    std::cout << "10 + 5 = " << calc.add(10, 5) << std::endl;
    std::cout << "20 * 3 = " << calc.multiply(20, 3) << std::endl;
    std::cout << "15 - 7 = " << calc.subtract(15, 7) << std::endl;
    std::cout << "100 / 4 = " << calc.divide(100, 4) << std::endl;
    std::cout << "2^8 = " << calc.power(2, 8) << std::endl;
    
    calc.showHistory();
    
    // Interactive mode
    while (true) {
        displayMenu();
        std::cin >> choice;
        
        switch (choice) {
            case 1: // Addition
                std::cout << "Enter two numbers: ";
                std::cin >> a >> b;
                std::cout << "Result: " << calc.add(a, b) << std::endl;
                break;
                
            case 2: // Subtraction
                std::cout << "Enter two numbers: ";
                std::cin >> a >> b;
                std::cout << "Result: " << calc.subtract(a, b) << std::endl;
                break;
                
            case 3: // Multiplication
                std::cout << "Enter two numbers: ";
                std::cin >> a >> b;
                std::cout << "Result: " << calc.multiply(a, b) << std::endl;
                break;
                
            case 4: // Division
                std::cout << "Enter two numbers: ";
                std::cin >> a >> b;
                std::cout << "Result: " << calc.divide(a, b) << std::endl;
                break;
                
            case 5: // Power
                std::cout << "Enter base and exponent: ";
                std::cin >> a >> exponent;
                std::cout << "Result: " << calc.power(a, exponent) << std::endl;
                break;
                
            case 6: // Show History
                calc.showHistory();
                break;
                
            case 7: // Clear History
                calc.clearHistory();
                break;
                
            case 8: // Exit
                std::cout << "\nThank you for using the calculator!" << std::endl;
                std::cout << "Total calculations performed: " 
                         << calc.getCalculationCount() << std::endl;
                return 0;
                
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
        }
    }
    
    return 0;
}