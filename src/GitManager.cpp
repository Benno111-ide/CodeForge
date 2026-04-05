#include "../include/GitManager.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#define CODEFORGE_GIT_POPEN _popen
#define CODEFORGE_GIT_PCLOSE _pclose
#else
#define CODEFORGE_GIT_POPEN popen
#define CODEFORGE_GIT_PCLOSE pclose
#endif

bool GitManager::isGitAvailable() const {
#ifdef _WIN32
    return std::system("git --version > NUL 2>&1") == 0;
#else
    return std::system("git --version > /dev/null 2>&1") == 0;
#endif
}

bool GitManager::isRepository(const std::string& path) const {
    return !findRepositoryRoot(path).empty();
}

std::string GitManager::findRepositoryRoot(const std::string& path) const {
    GitOperationResult result = runGitCommand(path, "git rev-parse --show-toplevel");
    if (!result.success) {
        return "";
    }

    std::string root = result.output;
    while (!root.empty() && (root.back() == '\n' || root.back() == '\r')) {
        root.pop_back();
    }
    return root;
}

std::string GitManager::getCurrentBranch(const std::string& repoPath) const {
    GitOperationResult result = runGitCommand(repoPath, "git branch --show-current");
    if (!result.success) {
        return "";
    }

    std::string branch = result.output;
    while (!branch.empty() && (branch.back() == '\n' || branch.back() == '\r')) {
        branch.pop_back();
    }
    return branch;
}

std::vector<GitStatusEntry> GitManager::getStatus(const std::string& repoPath) const {
    std::vector<GitStatusEntry> entries;
    GitOperationResult result = runGitCommand(repoPath, "git status --short");
    if (!result.success) {
        return entries;
    }

    std::stringstream stream(result.output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.size() < 3) {
            continue;
        }

        GitStatusEntry entry;
        entry.indexStatus = std::string(1, line[0]);
        entry.workTreeStatus = std::string(1, line[1]);
        entry.path = line.size() > 3 ? line.substr(3) : "";
        entries.push_back(entry);
    }

    return entries;
}

GitOperationResult GitManager::initRepository(const std::string& path) const {
    return runGitCommand(path, "git init");
}

GitOperationResult GitManager::stageAll(const std::string& repoPath) const {
    return runGitCommand(repoPath, "git add .");
}

GitOperationResult GitManager::commit(const std::string& repoPath, const std::string& message) const {
    std::string escapedMessage = message;
    size_t pos = 0;
    while ((pos = escapedMessage.find('"', pos)) != std::string::npos) {
        escapedMessage.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return runGitCommand(repoPath, "git commit -m \"" + escapedMessage + "\"");
}

GitOperationResult GitManager::pull(const std::string& repoPath) const {
    return runGitCommand(repoPath, "git pull");
}

GitOperationResult GitManager::push(const std::string& repoPath) const {
    return runGitCommand(repoPath, "git push");
}

GitOperationResult GitManager::runGitCommand(const std::string& workingDirectory, const std::string& command) const {
    GitOperationResult result;
    if (!isGitAvailable()) {
        result.output = "Git is not installed or is not available on PATH.";
        return result;
    }

    std::error_code ec;
    std::filesystem::path originalPath = std::filesystem::current_path(ec);
    if (ec) {
        result.output = "Failed to query current working directory.";
        return result;
    }

    std::filesystem::current_path(workingDirectory, ec);
    if (ec) {
        result.output = "Failed to switch to working directory: " + workingDirectory;
        return result;
    }

    std::string fullCommand = command + " 2>&1";

    std::array<char, 512> buffer{};
    FILE* pipe = CODEFORGE_GIT_POPEN(fullCommand.c_str(), "r");
    if (!pipe) {
        std::filesystem::current_path(originalPath, ec);
        result.output = "Failed to execute git command.";
        return result;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result.output += buffer.data();
    }

    int exitCode = CODEFORGE_GIT_PCLOSE(pipe);
    std::filesystem::current_path(originalPath, ec);
    result.success = (exitCode == 0);
    return result;
}
