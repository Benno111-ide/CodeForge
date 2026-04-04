#include "ProjectManager.h"

bool ProjectManager::createProject(const std::string& name, const std::string& path) {
    projectName = name;
    projectPath = path;
    return true;
}

bool ProjectManager::loadProject(const std::string& path) {
    projectPath = path;
    // Extract project name from path
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        projectName = path.substr(pos + 1);
    } else {
        projectName = path;
    }
    return true;
}

bool ProjectManager::saveProject() {
    // Stub implementation
    return true;
}

void ProjectManager::closeProject() {
    projectPath.clear();
    projectName.clear();
}