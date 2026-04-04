#include "FileManager.h"
#include <fstream>
#include <filesystem>

bool FileManager::loadFile(const std::string& path, std::string& content) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    content = std::string((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return true;
}

bool FileManager::saveFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    file << content;
    return file.good();
}

std::vector<std::string> FileManager::listDirectory(const std::string& path) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            files.push_back(entry.path().filename().string());
        }
    } catch (...) {
        // Handle error
    }
    return files;
}

bool FileManager::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool FileManager::createDirectory(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (...) {
        return false;
    }
}

std::string FileManager::getCurrentDirectory() {
    try {
        return std::filesystem::current_path().string();
    } catch (...) {
        return "";
    }
}

void FileManager::setCurrentDirectory(const std::string& path) {
    try {
        std::filesystem::current_path(path);
    } catch (...) {
        // Handle error
    }
}