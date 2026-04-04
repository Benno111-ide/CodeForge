#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>

enum class CompileStatus {
    Ready,
    Compiling,
    Success,
    Error,
    Warning
};

struct CompileMessage {
    std::string file;
    int line;
    int column;
    std::string message;
    bool isError;
    bool isWarning;
    
    CompileMessage(const std::string& f, int l, int c, const std::string& msg, bool err = false, bool warn = false)
        : file(f), line(l), column(c), message(msg), isError(err), isWarning(warn) {}
};

struct CompileResult {
    CompileStatus status;
    std::vector<CompileMessage> messages;
    std::string output;
    float compileTime;
    bool success;
};

class CompilerEngine {
public:
    CompilerEngine();
    ~CompilerEngine();
    
    // Compilation
    CompileResult compile(const std::string& sourceCode, const std::string& outputPath = "");
    CompileResult compileFile(const std::string& filePath, const std::string& outputPath = "");
    CompileResult compileProject(const std::string& projectPath);
    
    void compileAsync(const std::string& sourceCode, std::function<void(const CompileResult&)> callback);
    
    // Compiler settings
    void setCompiler(const std::string& compiler) { compilerPath = compiler; }
    void setCppStandard(const std::string& standard) { cppStandard = standard; }
    void setOptimizationLevel(int level) { optimizationLevel = level; }
    void setDebugSymbols(bool enable) { debugSymbols = enable; }
    void setWarningsAsErrors(bool enable) { warningsAsErrors = enable; }
    
    void addIncludePath(const std::string& path);
    void addLibraryPath(const std::string& path);
    void addLibrary(const std::string& library);
    void addDefine(const std::string& define);
    void addCompilerFlag(const std::string& flag);
    
    // Getters
    const std::string& getCompiler() const { return compilerPath; }
    const std::string& getCppStandard() const { return cppStandard; }
    int getOptimizationLevel() const { return optimizationLevel; }
    bool getDebugSymbols() const { return debugSymbols; }
    bool getWarningsAsErrors() const { return warningsAsErrors; }
    
    const std::vector<std::string>& getIncludePaths() const { return includePaths; }
    const std::vector<std::string>& getLibraryPaths() const { return libraryPaths; }
    const std::vector<std::string>& getLibraries() const { return libraries; }
    const std::vector<std::string>& getDefines() const { return defines; }
    const std::vector<std::string>& getCompilerFlags() const { return compilerFlags; }
    
    // Utilities
    std::string generateCompileCommand(const std::string& inputFile, const std::string& outputFile) const;
    bool isCompiling() const { return currentStatus == CompileStatus::Compiling; }
    CompileStatus getStatus() const { return currentStatus; }
    
    // Static utilities
    static std::vector<std::string> detectCompilers();
    static bool isCompilerAvailable(const std::string& compiler);
    static std::string getCompilerVersion(const std::string& compiler);
    
private:
    std::string compilerPath;
    std::string cppStandard;
    int optimizationLevel;
    bool debugSymbols;
    bool warningsAsErrors;
    
    std::vector<std::string> includePaths;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    std::vector<std::string> defines;
    std::vector<std::string> compilerFlags;
    
    CompileStatus currentStatus;
    
    CompileResult executeCompile(const std::string& command, const std::string& inputFile);
    std::vector<CompileMessage> parseCompilerOutput(const std::string& output);
    std::string createTemporaryFile(const std::string& content, const std::string& extension = ".cpp");
};