#pragma once

#include <string>
#include <vector>

struct GitStatusEntry {
    std::string path;
    std::string indexStatus;
    std::string workTreeStatus;
};

struct GitOperationResult {
    bool success = false;
    std::string output;
};

class GitManager {
public:
    bool isGitAvailable() const;
    bool isRepository(const std::string& path) const;
    std::string findRepositoryRoot(const std::string& path) const;
    std::string getCurrentBranch(const std::string& repoPath) const;
    std::vector<GitStatusEntry> getStatus(const std::string& repoPath) const;

    GitOperationResult initRepository(const std::string& path) const;
    GitOperationResult stageAll(const std::string& repoPath) const;
    GitOperationResult commit(const std::string& repoPath, const std::string& message) const;
    GitOperationResult pull(const std::string& repoPath) const;
    GitOperationResult push(const std::string& repoPath) const;

private:
    GitOperationResult runGitCommand(const std::string& workingDirectory, const std::string& command) const;
};
