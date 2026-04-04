#pragma once
#include <string>

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();
    
    void loadSettings();
    void saveSettings();
    
    // Editor settings
    bool getShowLineNumbers() const { return showLineNumbers; }
    void setShowLineNumbers(bool show) { showLineNumbers = show; }
    
    bool getSyntaxHighlighting() const { return syntaxHighlighting; }
    void setSyntaxHighlighting(bool enable) { syntaxHighlighting = enable; }
    
    int getTabSize() const { return tabSize; }
    void setTabSize(int size) { tabSize = size; }
    
    // Compiler settings
    std::string getDefaultCompiler() const { return defaultCompiler; }
    void setDefaultCompiler(const std::string& compiler) { defaultCompiler = compiler; }
    
private:
    std::string settingsFile;
    
    // Editor settings
    bool showLineNumbers = true;
    bool syntaxHighlighting = true;
    int tabSize = 4;
    
    // Compiler settings
    std::string defaultCompiler = "g++";
};