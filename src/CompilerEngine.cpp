#include "../include/CompilerEngine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <cstdlib>
#include <chrono>
#include <regex>
#include <filesystem>

#ifdef _WIN32
#define CODEFORGE_POPEN _popen
#define CODEFORGE_PCLOSE _pclose
#else
#define CODEFORGE_POPEN popen
#define CODEFORGE_PCLOSE pclose
#endif

CompilerEngine::CompilerEngine() 
    : compilerPath("g++"), cppStandard("17"), optimizationLevel(0), 
      debugSymbols(true), warningsAsErrors(false), currentStatus(CompileStatus::Ready) {
}

CompilerEngine::~CompilerEngine() = default;

CompileResult CompilerEngine::compile(const std::string& sourceCode, const std::string& outputPath) {
    currentStatus = CompileStatus::Compiling;
    
    std::string tempFile = createTemporaryFile(sourceCode);
    std::string output = outputPath.empty() ? "a.out" : outputPath;
    std::string command = generateCompileCommand(tempFile, output);
    
    auto start = std::chrono::high_resolution_clock::now();
    CompileResult result = executeCompile(command, tempFile);
    auto end = std::chrono::high_resolution_clock::now();
    
    result.compileTime = std::chrono::duration<float>(end - start).count();
    
    // Clean up temporary file
    std::remove(tempFile.c_str());
    
    currentStatus = result.status;
    return result;
}

CompileResult CompilerEngine::compileFile(const std::string& filePath, const std::string& outputPath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        CompileResult result;
        result.status = CompileStatus::Error;
        result.success = false;
        result.output = "Error: Could not open file " + filePath;
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return compile(buffer.str(), outputPath);
}

CompileResult CompilerEngine::compileProject(const std::string& projectPath) {
    (void)projectPath;  // Suppress unused parameter warning
    // Stub implementation
    CompileResult result;
    result.status = CompileStatus::Error;
    result.success = false;
    result.output = "Project compilation not yet implemented";
    return result;
}

void CompilerEngine::compileAsync(const std::string& sourceCode, std::function<void(const CompileResult&)> callback) {
    std::thread([this, sourceCode, callback]() {
        CompileResult result = compile(sourceCode);
        if (callback) {
            callback(result);
        }
    }).detach();
}

void CompilerEngine::addIncludePath(const std::string& path) {
    includePaths.push_back(path);
}

void CompilerEngine::addLibraryPath(const std::string& path) {
    libraryPaths.push_back(path);
}

void CompilerEngine::addLibrary(const std::string& library) {
    libraries.push_back(library);
}

void CompilerEngine::addDefine(const std::string& define) {
    defines.push_back(define);
}

void CompilerEngine::addCompilerFlag(const std::string& flag) {
    compilerFlags.push_back(flag);
}

std::string CompilerEngine::generateCompileCommand(const std::string& inputFile, const std::string& outputFile) const {
    std::string cmd = compilerPath + " ";
    
    // C++ standard
    cmd += "-std=c++" + cppStandard + " ";
    
    // Optimization
    cmd += "-O" + std::to_string(optimizationLevel) + " ";
    
    // Debug symbols
    if (debugSymbols) {
        cmd += "-g ";
    }
    
    // Warnings as errors
    if (warningsAsErrors) {
        cmd += "-Werror ";
    }
    
    // Include paths
    for (const auto& path : includePaths) {
        cmd += "-I\"" + path + "\" ";
    }
    
    // Library paths
    for (const auto& path : libraryPaths) {
        cmd += "-L\"" + path + "\" ";
    }
    
    // Defines
    for (const auto& define : defines) {
        cmd += "-D" + define + " ";
    }
    
    // Additional flags
    for (const auto& flag : compilerFlags) {
        cmd += flag + " ";
    }
    
    // Input and output
    cmd += "\"" + inputFile + "\" ";
    cmd += "-o \"" + outputFile + "\" ";
    
    // Libraries
    for (const auto& lib : libraries) {
        cmd += "-l" + lib + " ";
    }
    
    return cmd;
}

std::vector<std::string> CompilerEngine::detectCompilers() {
    std::vector<std::string> compilers;
    
    // Common compiler names
    std::vector<std::string> candidates = {"g++", "clang++", "gcc", "clang"};
    
    for (const auto& compiler : candidates) {
        if (isCompilerAvailable(compiler)) {
            compilers.push_back(compiler);
        }
    }
    
    return compilers;
}

bool CompilerEngine::isCompilerAvailable(const std::string& compiler) {
    std::string command = compiler + " --version";
#ifdef _WIN32
    command += " > NUL 2>&1";
#else
    command += " > /dev/null 2>&1";
#endif
    return system(command.c_str()) == 0;
}

std::string CompilerEngine::getCompilerVersion(const std::string& compiler) {
    std::string command = compiler + " --version 2>&1";
    FILE* pipe = CODEFORGE_POPEN(command.c_str(), "r");
    if (!pipe) return "Unknown";
    
    char buffer[256];
    std::string result = "";
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result = buffer;
    }
    CODEFORGE_PCLOSE(pipe);
    
    return result;
}

CompileResult CompilerEngine::executeCompile(const std::string& command, const std::string& inputFile) {
    (void)inputFile;  // Suppress unused parameter warning
    CompileResult result;
    result.success = false;
    
    // Execute command and capture output
    std::string fullCommand = command + " 2>&1";
    FILE* pipe = CODEFORGE_POPEN(fullCommand.c_str(), "r");
    if (!pipe) {
        result.status = CompileStatus::Error;
        result.output = "Error: Could not execute compiler";
        return result;
    }
    
    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    int exitCode = CODEFORGE_PCLOSE(pipe);
    result.output = output;
    
    // Parse compiler output for messages
    result.messages = parseCompilerOutput(output);
    
    // Determine status
    if (exitCode == 0) {
        bool hasWarnings = false;
        for (const auto& msg : result.messages) {
            if (msg.isWarning) {
                hasWarnings = true;
                break;
            }
        }
        result.status = hasWarnings ? CompileStatus::Warning : CompileStatus::Success;
        result.success = true;
    } else {
        result.status = CompileStatus::Error;
        result.success = false;
    }
    
    return result;
}

std::vector<CompileMessage> CompilerEngine::parseCompilerOutput(const std::string& output) {
    std::vector<CompileMessage> messages;
    
    // Simple regex to parse GCC/Clang output
    std::regex errorRegex(R"(([^:]+):(\d+):(\d+):\s*(error|warning):\s*(.+))");
    std::sregex_iterator iter(output.begin(), output.end(), errorRegex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        if (match.size() == 6) {
            std::string file = match[1];
            int line = std::stoi(match[2]);
            int column = std::stoi(match[3]);
            std::string type = match[4];
            std::string message = match[5];
            
            bool isError = (type == "error");
            bool isWarning = (type == "warning");
            
            messages.emplace_back(file, line, column, message, isError, isWarning);
        }
    }
    
    return messages;
}

std::string CompilerEngine::createTemporaryFile(const std::string& content, const std::string& extension) {
    std::filesystem::path filename = std::filesystem::temp_directory_path() /
        ("codeforge_temp_source" + extension);
    std::ofstream file(filename);
    if (file.is_open()) {
        file << content;
        file.close();
    }
    return filename.string();
}
