#pragma once
#include <string>
#include <vector>

class FileManager {
public:
    FileManager() = default;
    ~FileManager() = default;
    
    bool loadFile(const std::string& path, std::string& content);
    bool saveFile(const std::string& path, const std::string& content);
    std::vector<std::string> listDirectory(const std::string& path);
    bool fileExists(const std::string& path);
    bool createDirectory(const std::string& path);
    std::string getCurrentDirectory();
    void setCurrentDirectory(const std::string& path);
};