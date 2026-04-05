#pragma once

#include <string>

struct CodexRunOptions {
    std::string prompt;
    std::string workingDirectory;
    std::string model;
    std::string sandboxMode = "workspace-write";
    bool useFullAuto = true;
    bool ephemeral = false;
    bool skipGitRepoCheck = false;
};

struct CodexRunResult {
    bool success = false;
    std::string command;
    std::string output;
};

class CodexManager {
public:
    bool isCodexAvailable() const;
    CodexRunResult runPrompt(const CodexRunOptions& options) const;
    CodexRunResult reviewRepository(const CodexRunOptions& options) const;

private:
    CodexRunResult runCodexCommand(const std::string& workingDirectory,
                                   const std::string& command,
                                   const std::string& prompt) const;
    std::string buildExecArguments(const CodexRunOptions& options) const;
    std::string buildReviewArguments(const CodexRunOptions& options) const;
    std::string shellQuote(const std::string& value) const;
};
