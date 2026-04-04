#pragma once
#include <string>

class BuildSystem {
public:
    BuildSystem() = default;
    ~BuildSystem() = default;
    
    bool generateMakefile(const std::string& projectPath);
    bool generateCMakeFile(const std::string& projectPath);
    bool build(const std::string& projectPath);
    bool clean(const std::string& projectPath);
};