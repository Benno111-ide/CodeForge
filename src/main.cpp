#include "../include/Application.h"
#include <iostream>
#include <exception>

int main() {
    try {
        Application app;
        
        std::cout << "CodeForge - Professional C++ IDE" << std::endl;
        std::cout << "Starting application..." << std::endl;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return -1;
        }
        
        std::cout << "Application initialized successfully" << std::endl;
        
        app.run();
        
        std::cout << "Application shutting down..." << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown application error" << std::endl;
        return -1;
    }
}