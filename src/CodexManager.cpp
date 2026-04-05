#include "../include/CodexManager.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#define CODEFORGE_CODEX_POPEN _popen
#define CODEFORGE_CODEX_PCLOSE _pclose
#else
#define CODEFORGE_CODEX_POPEN popen
#define CODEFORGE_CODEX_PCLOSE pclose
#endif

bool CodexManager::isCodexAvailable() const {
#ifdef _WIN32
    return std::system("codex --version > NUL 2>&1") == 0;
#else
    return std::system("codex --version > /dev/null 2>&1") == 0;
#endif
}

CodexRunResult CodexManager::runPrompt(const CodexRunOptions& options) const {
    std::string command = "codex exec --color never ";
    command += buildExecArguments(options);
    command += "- ";
    return runCodexCommand(options.workingDirectory, command, options.prompt);
}

CodexRunResult CodexManager::reviewRepository(const CodexRunOptions& options) const {
    std::string command = "codex exec review ";
    command += buildReviewArguments(options);
    command += "- ";
    return runCodexCommand(options.workingDirectory, command, options.prompt);
}

CodexRunResult CodexManager::runCodexCommand(const std::string& workingDirectory,
                                             const std::string& command,
                                             const std::string& prompt) const {
    CodexRunResult result;
    result.command = command;

    if (!isCodexAvailable()) {
        result.output = "Codex is not installed or is not available on PATH.";
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

    std::filesystem::path promptFile = std::filesystem::temp_directory_path(ec) /
                                       std::filesystem::path("codeforge_codex_prompt.txt");
    if (ec) {
        std::filesystem::current_path(originalPath, ec);
        result.output = "Failed to allocate a prompt file for Codex.";
        return result;
    }

    {
        std::ofstream stream(promptFile);
        if (!stream.is_open()) {
            std::filesystem::current_path(originalPath, ec);
            result.output = "Failed to create the temporary Codex prompt file.";
            return result;
        }
        stream << prompt;
    }

    std::string fullCommand = command + " < " + shellQuote(promptFile.string()) + " 2>&1";
    std::array<char, 512> buffer{};
    FILE* pipe = CODEFORGE_CODEX_POPEN(fullCommand.c_str(), "r");
    if (!pipe) {
        std::filesystem::remove(promptFile, ec);
        std::filesystem::current_path(originalPath, ec);
        result.output = "Failed to execute Codex.";
        return result;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result.output += buffer.data();
    }

    int exitCode = CODEFORGE_CODEX_PCLOSE(pipe);
    std::filesystem::remove(promptFile, ec);
    std::filesystem::current_path(originalPath, ec);
    result.success = (exitCode == 0);
    return result;
}

std::string CodexManager::buildExecArguments(const CodexRunOptions& options) const {
    std::string args;

    if (!options.model.empty()) {
        args += "--model " + shellQuote(options.model) + " ";
    }

    if (options.useFullAuto) {
        args += "--full-auto ";
    } else if (!options.sandboxMode.empty()) {
        args += "--sandbox " + shellQuote(options.sandboxMode) + " ";
    }

    if (options.ephemeral) {
        args += "--ephemeral ";
    }

    if (options.skipGitRepoCheck) {
        args += "--skip-git-repo-check ";
    }

    return args;
}

std::string CodexManager::buildReviewArguments(const CodexRunOptions& options) const {
    std::string args;

    if (!options.model.empty()) {
        args += "--model " + shellQuote(options.model) + " ";
    }

    if (options.useFullAuto) {
        args += "--full-auto ";
    }

    if (options.ephemeral) {
        args += "--ephemeral ";
    }

    if (options.skipGitRepoCheck) {
        args += "--skip-git-repo-check ";
    }

    return args;
}

std::string CodexManager::shellQuote(const std::string& value) const {
    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"' || ch == '\\') {
            escaped += '\\';
        }
        escaped += ch;
    }
    escaped += "\"";
    return escaped;
}
