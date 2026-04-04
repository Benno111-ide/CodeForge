#pragma once
#include <string>
#include <vector>

class ProjectManager {
public:
    ProjectManager() = default;
    ~ProjectManager() = default;
    
    bool createProject(const std::string& name, const std::string& path);
    bool loadProject(const std::string& path);
    bool saveProject();
    void closeProject();
    
    std::string getCurrentProjectPath() const { return projectPath; }
    std::string getCurrentProjectName() const { return projectName; }
    bool hasOpenProject() const { return !projectPath.empty(); }
    
private:
    std::string projectPath;
    std::string projectName;
};