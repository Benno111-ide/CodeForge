#include "UIManager.h"
#include "../include/CodeEditor.h"
#include "../include/Application.h"
#include "../include/CompilerEngine.h"
#include "../include/CodexManager.h"
#include "../include/GitManager.h"
#include "FileManager.h"
#include "ProjectManager.h"
#include "SettingsManager.h"

#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <memory>
#include <fstream>
#include <ctime>
#include <exception>
#include <cfloat>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <chrono>

namespace {

std::string toLowerCopy(const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return lowered;
}

bool isSupportedWorkspaceFile(const std::string& filename, const std::string& extension) {
    const std::string normalizedName = toLowerCopy(filename);
    const std::string normalizedExtension = toLowerCopy(extension);

    if (normalizedName == "cmakelists.txt" || normalizedName == "makefile" ||
        normalizedName == ".gitignore" || normalizedName == "agents.md" ||
        normalizedName == "skill.md" || normalizedName == "harness.json" ||
        normalizedName == "package.json" || normalizedName == "package-lock.json" ||
        normalizedName == "pyproject.toml" || normalizedName == "requirements.txt") {
        return true;
    }

    return normalizedExtension == ".cpp" || normalizedExtension == ".h" ||
           normalizedExtension == ".hpp" || normalizedExtension == ".c" ||
           normalizedExtension == ".cc" || normalizedExtension == ".cxx" ||
           normalizedExtension == ".txt" || normalizedExtension == ".md" ||
           normalizedExtension == ".cmake" || normalizedExtension == ".py" ||
           normalizedExtension == ".js" || normalizedExtension == ".ts" ||
           normalizedExtension == ".tsx" || normalizedExtension == ".jsx" ||
           normalizedExtension == ".json" || normalizedExtension == ".yaml" ||
           normalizedExtension == ".yml" || normalizedExtension == ".toml" ||
           normalizedExtension == ".sh" || normalizedExtension == ".ps1" ||
           normalizedExtension == ".cfproj" ||
           normalizedExtension == ".java" || normalizedExtension == ".cs";
}

std::string getWorkspaceFileIcon(const std::string& filename, const std::string& extension) {
    const std::string normalizedName = toLowerCopy(filename);
    const std::string normalizedExtension = toLowerCopy(extension);

    if (normalizedName == "cmakelists.txt" || normalizedName == "makefile" ||
        normalizedExtension == ".cfproj" ||
        normalizedName == "harness.json") {
        return "CFG";
    }

    if (normalizedName == "agents.md" || normalizedName == "skill.md") {
        return "AI";
    }

    if (normalizedName == ".gitignore" || normalizedName == "package.json" ||
        normalizedName == "package-lock.json" || normalizedName == "pyproject.toml" ||
        normalizedName == "requirements.txt" || normalizedExtension == ".json" ||
        normalizedExtension == ".yaml" || normalizedExtension == ".yml" ||
        normalizedExtension == ".toml") {
        return "SET";
    }

    if (normalizedExtension == ".cpp" || normalizedExtension == ".c") {
        return "SRC";
    }

    if (normalizedExtension == ".h" || normalizedExtension == ".hpp") {
        return "HDR";
    }

    if (normalizedExtension == ".md" || normalizedExtension == ".txt") {
        return "DOC";
    }

    if (normalizedExtension == ".py" || normalizedExtension == ".js" ||
        normalizedExtension == ".ts" || normalizedExtension == ".tsx" ||
        normalizedExtension == ".jsx" || normalizedExtension == ".sh" ||
        normalizedExtension == ".ps1") {
        return "RUN";
    }

    return "FILE";
}

}  // namespace

UIManager::UIManager() {
    codeEditor = std::make_unique<CodeEditor>();
    codexManager = std::make_unique<CodexManager>();
    gitManager = std::make_unique<GitManager>();
    codeEditor->setLineNumbers(false);      // Hide line numbers by default
    codeEditor->setAutoScroll(false);       // Disable automatic cursor-driven scrolling
    
    // Initialize file browser
    currentDirectory = std::filesystem::current_path().string();
    refreshDirectoryContents();
    
    // Initialize project manager
    newProjectPath = currentDirectory;
    browserCurrentPath = currentDirectory;
    refreshBrowserDirectories();
    
    // Apply glossy theme (transparency disabled)
    applyGlossyTheme();
    
    // Window blur effects disabled - removed enableWindowBlur()
    
    // Load a default example
    codeEditor->setText(R"(#include <iostream>
#include <vector>
#include <string>

// Simple C++ example program
class Calculator {
private:
    std::vector<double> history;

public:
    double add(double a, double b) {
        double result = a + b;
        history.push_back(result);
        std::cout << a << " + " << b << " = " << result << std::endl;
        return result;
    }
    
    double multiply(double a, double b) {
        double result = a * b;
        history.push_back(result);
        std::cout << a << " * " << b << " = " << result << std::endl;
        return result;
    }
    
    void showHistory() {
        std::cout << "Calculation History:" << std::endl;
        for (size_t i = 0; i < history.size(); ++i) {
            std::cout << i + 1 << ": " << history[i] << std::endl;
        }
    }
};

int main() {
    Calculator calc;
    
    std::cout << "=== C++ Calculator Demo ===" << std::endl;
    
    calc.add(10, 5);
    calc.multiply(3, 7);
    calc.add(2.5, 3.7);
    
    calc.showHistory();
    
    return 0;
})");
}

UIManager::~UIManager() = default;

void UIManager::update(float deltaTime) {
    (void)deltaTime;  // Suppress unused parameter warning
    if (codexRunInProgress &&
        codexFuture.valid() &&
        codexFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        CodexRunResult result = codexFuture.get();
        codexRunInProgress = false;
        codexLastCommand = result.command;
        codexOutput = result.output.empty() ? (result.success ? "Codex completed successfully." : "Codex finished without output.") : result.output;
        codexStatusMessage = result.success ? "Last run succeeded" : "Last run failed";
    }
}

void UIManager::render() {
    try {
        // Handle keyboard shortcuts
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_O)) {
                showOpenFileDialog = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                if (codeEditor) {
                    codeEditor->saveFile(currentFile.empty() ? "untitled.cpp" : currentFile);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_N)) {
                if (codeEditor) {
                    codeEditor->clear();
                    currentFile.clear();
                }
            }
        }
        
        // Backdrop blur disabled - removed renderBackdropBlur()
        
        if (showMainMenuBar) {
            renderMainMenuBar();
        }
        
        if (showEditorWindow) {
            renderCodeEditor();
        }
        
        if (showCompilerWindow) {
            renderCompilerWindow();
        }
        
        if (showOutputWindow) {
            renderOutputWindow();
        }
        
        if (showProjectWindow) {
            renderProjectWindow();
        }
        
        if (showProjectManager) {
            renderProjectManager();
        }

        if (showGitWindow) {
            renderGitWindow();
        }

        if (showCodexWindow) {
            renderCodexWindow();
        }
        
        if (showSettingsWindow) {
            renderSettingsWindow();
        }
        
        if (showAboutWindow) {
            renderAboutWindow();
        }
        
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }
        
        // Render file dialogs
        renderFileDialog();
        renderCreateProjectDialog();
        renderProjectSettings();
        renderDirectoryBrowser();
    } catch (const std::exception& e) {
        // Log UI exception to crash dump
        std::ofstream crashFile("codeforge_crash_dump.txt", std::ios::app);
        
        time_t rawtime;
        struct tm* timeinfo;
        char timestr[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        crashFile << "\n=== CODEFORGE UI EXCEPTION ===" << std::endl;
        crashFile << "Time: " << timestr << std::endl;
        crashFile << "Exception: " << e.what() << std::endl;
        crashFile << "Location: UI Render function" << std::endl;
        crashFile << "=== END UI EXCEPTION ===" << std::endl;
        crashFile.close();
        
        std::cerr << "UI Exception caught: " << e.what() << std::endl;
        std::cerr << "UI exception details saved to codeforge_crash_dump.txt" << std::endl;
        
        // Reset dangerous UI state
        showDemoWindow = false;
        showAboutWindow = false;
    } catch (...) {
        // Log unknown UI exception
        std::ofstream crashFile("codeforge_crash_dump.txt", std::ios::app);
        
        time_t rawtime;
        struct tm* timeinfo;
        char timestr[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        crashFile << "\n=== CODEFORGE UNKNOWN UI EXCEPTION ===" << std::endl;
        crashFile << "Time: " << timestr << std::endl;
        crashFile << "Exception: Unknown exception in UI" << std::endl;
        crashFile << "Location: UI Render function" << std::endl;
        crashFile << "=== END UNKNOWN UI EXCEPTION ===" << std::endl;
        crashFile.close();
        
        std::cerr << "Unknown UI exception caught" << std::endl;
        std::cerr << "UI exception details saved to codeforge_crash_dump.txt" << std::endl;
        
        // Reset dangerous UI state
        showDemoWindow = false;
        showAboutWindow = false;
    }
}

void UIManager::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        ImGui::Text("CodeForge");
        ImGui::Separator();
        
        try {
            handleFileMenu();
        } catch (...) {
            std::cerr << "Exception in File menu" << std::endl;
        }
        
        try {
            handleProjectMenu();
        } catch (...) {
            std::cerr << "Exception in Project menu" << std::endl;
        }
        
        try {
            handleEditMenu();
        } catch (...) {
            std::cerr << "Exception in Edit menu" << std::endl;
        }
        
        try {
            handleBuildMenu();
        } catch (...) {
            std::cerr << "Exception in Build menu" << std::endl;
        }
        
        try {
            handleViewMenu();
        } catch (...) {
            std::cerr << "Exception in View menu" << std::endl;
        }

        try {
            handleGitMenu();
        } catch (...) {
            std::cerr << "Exception in Git menu" << std::endl;
        }

        try {
            handleCodexMenu();
        } catch (...) {
            std::cerr << "Exception in Codex menu" << std::endl;
        }
        
        try {
            handleHelpMenu();
        } catch (...) {
            std::cerr << "Exception in Help menu" << std::endl;
        }
        
        ImGui::EndMainMenuBar();
    }
}

void UIManager::renderDockSpace() {
    // Simplified layout without advanced docking
    // This function is now unused but kept for future compatibility
}

void UIManager::renderCodeEditor() {
    codeEditor->render("Code Editor", &showEditorWindow);
}

void UIManager::renderCompilerWindow() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(420, 320), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Compiler", &showCompilerWindow)) {
        Application* app = Application::getInstance();
        CompilerEngine* compiler = app ? app->getCompilerEngine() : nullptr;
        
        if (compiler) {
            // Compiler settings
            ImGui::Text("Compiler: %s", compiler->getCompiler().c_str());
            ImGui::Text("C++ Standard: C++%s", compiler->getCppStandard().c_str());
            ImGui::Text("Optimization: O%d", compiler->getOptimizationLevel());
            ImGui::Text("Debug Symbols: %s", compiler->getDebugSymbols() ? "Yes" : "No");
            
            ImGui::Separator();
            
            // Compile button
            bool isCompiling = compiler->isCompiling();
            if (isCompiling) {
                ImGui::Text("Compiling...");
            } else {
                if (ImGui::Button("Compile Current File")) {
                    std::string sourceCode = codeEditor->getText();
                    compiler->compileAsync(sourceCode, [this](const CompileResult& result) {
                        // Update output with compilation results
                        compilerOutput = result.output;
                        for (const auto& msg : result.messages) {
                            compilerOutput += msg.message + "\n";
                        }
                    });
                }
                ImGui::SameLine();
                if (ImGui::Button("Build Project")) {
                    // Implement project building
                }
            }
            
            // Status
            ImGui::Separator();
            ImGui::Text("Status: ");
            ImGui::SameLine();
            switch (compiler->getStatus()) {
                case CompileStatus::Ready:
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready");
                    break;
                case CompileStatus::Compiling:
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Compiling...");
                    break;
                case CompileStatus::Success:
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Success");
                    break;
                case CompileStatus::Error:
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error");
                    break;
                case CompileStatus::Warning:
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning");
                    break;
            }
        }
    }
    ImGui::End();
}

void UIManager::renderOutputWindow() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(400, 200), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Output", &showOutputWindow)) {
        ImGui::TextWrapped("%s", compilerOutput.c_str());
        
        if (ImGui::Button("Clear")) {
            compilerOutput.clear();
        }
    }
    ImGui::End();
}

void UIManager::renderProjectWindow() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Project", &showProjectWindow)) {
        ImGui::Text("Project Explorer");
        ImGui::Separator();
        
        if (ImGui::TreeNode("Current Project")) {
            if (ImGui::TreeNode("Source Files")) {
                ImGui::Text("main.cpp");
                ImGui::Text("utils.cpp");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Header Files")) {
                ImGui::Text("utils.h");
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void UIManager::renderProjectManager() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(360, 320), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Project Manager", &showProjectManager)) {
        // Project controls
        if (ImGui::Button("New Project")) {
            showCreateProjectDialog = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Project")) {
            showOpenFileDialog = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Project") && currentProject) {
            saveProject();
        }
        ImGui::SameLine();
        if (ImGui::Button("Settings") && currentProject) {
            showProjectSettings = true;
        }
        
        ImGui::Separator();
        
        if (currentProject) {
            // Project info header
            ImGui::Text("Project: %s", currentProject->name.c_str());
            ImGui::Text("Type: %s", currentProject->projectType.c_str());
            ImGui::Text("Path: %s", currentProject->rootPath.c_str());
            ImGui::Text("Build Target: %s", currentProject->buildTarget.c_str());
            ImGui::Text("Git: %s",
                        currentProject->gitEnabled ? currentProject->gitBranch.c_str() : "Not initialized");
            
            ImGui::Separator();
            
            // Quick actions
            if (ImGui::Button("Add File")) {
                showOpenFileDialog = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("New File")) {
                if (codeEditor) {
                    codeEditor->clear();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                if (currentProject->rootFolder) {
                    scanProjectFiles(currentProject->rootFolder);
                }
            }
            
            ImGui::Separator();
            
            // Build configuration
            ImGui::Text("Build Configuration:");
            ImGui::SameLine();
            const char* configs[] = { "Debug", "Release", "RelWithDebInfo" };
            static int currentConfig = 0;
            if (ImGui::Combo("##config", &currentConfig, configs, 3)) {
                currentProject->buildConfiguration = configs[currentConfig];
            }
            
            ImGui::Separator();
            
            // Project tree
            ImGui::Text("Project Files:");
            ImGui::BeginChild("ProjectTree", ImVec2(0, 0));
            
            if (currentProject->rootFolder) {
                renderProjectTree(currentProject->rootFolder);
            }
            
            ImGui::EndChild();
        } else {
            // No project loaded
            ImGui::Text("No project loaded");
            ImGui::Separator();
            
            ImGui::Text("Quick Actions:");
            if (ImGui::Button("Create New Project", ImVec2(-1, 0))) {
                showCreateProjectDialog = true;
            }
            if (ImGui::Button("Open Existing Project", ImVec2(-1, 0))) {
                showOpenFileDialog = true;
            }
            
            // Recent projects
            if (!recentProjects.empty()) {
                ImGui::Separator();
                ImGui::Text("Recent Projects:");
                for (auto& project : recentProjects) {
                    if (ImGui::Button(project->name.c_str(), ImVec2(-1, 0))) {
                        currentProject = project;
                        if (currentProject->rootFolder) {
                            scanProjectFiles(currentProject->rootFolder);
                        }
                        refreshCodexTaskFiles();
                        refreshGitState();
                    }
                }
            }
        }
    }
    ImGui::End();
}

void UIManager::renderGitWindow() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(420, 280), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Git", &showGitWindow)) {
        if (!gitManager || !gitManager->isGitAvailable()) {
            ImGui::TextWrapped("Git is not available on PATH.");
            ImGui::End();
            return;
        }

        if (!currentProject) {
            ImGui::TextWrapped("Open or create a project to use Git features.");
            ImGui::End();
            return;
        }

        ImGui::Text("Repository: %s",
                    currentProject->gitEnabled ? currentProject->gitRepositoryRoot.c_str() : "Not initialized");
        ImGui::Text("Branch: %s",
                    currentProject->gitEnabled ? currentProject->gitBranch.c_str() : "-");

        ImGui::Separator();

        if (!currentProject->gitEnabled) {
            if (ImGui::Button("Initialize Git Repository")) {
                initializeProjectGitRepository();
            }
        } else {
            if (ImGui::Button("Refresh Status")) {
                refreshGitState();
            }
            ImGui::SameLine();
            if (ImGui::Button("Stage All")) {
                runGitStageAll();
            }
            ImGui::SameLine();
            if (ImGui::Button("Pull")) {
                runGitPull();
            }
            ImGui::SameLine();
            if (ImGui::Button("Push")) {
                runGitPush();
            }

            char commitBuffer[512] = {};
            strncpy(commitBuffer, gitCommitMessage.c_str(), sizeof(commitBuffer) - 1);
            if (ImGui::InputText("Commit Message", commitBuffer, sizeof(commitBuffer))) {
                gitCommitMessage = commitBuffer;
            }

            if (ImGui::Button("Commit")) {
                runGitCommit();
            }

            ImGui::Separator();
            ImGui::Text("Status");
            ImGui::BeginChild("GitStatusList", ImVec2(0, 120), true);
            if (gitStatusLines.empty()) {
                ImGui::TextUnformatted("Working tree clean.");
            } else {
                for (const auto& line : gitStatusLines) {
                    ImGui::TextWrapped("%s", line.c_str());
                }
            }
            ImGui::EndChild();
        }

        ImGui::Separator();
        ImGui::Text("Output");
        ImGui::BeginChild("GitOutput", ImVec2(0, 0), true);
        ImGui::TextWrapped("%s", gitOutput.empty() ? "No Git operations yet." : gitOutput.c_str());
        ImGui::EndChild();
    }
    ImGui::End();
}

void UIManager::renderCodexWindow() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(480, 320), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Codex", &showCodexWindow)) {
        const std::string workingRoot = currentProject ? currentProject->rootPath : currentDirectory;
        const bool codexAvailable = codexManager && codexManager->isCodexAvailable();

        ImGui::Text("Status: %s", codexStatusMessage.c_str());
        ImGui::TextWrapped("Workspace: %s", workingRoot.c_str());

        if (!codexAvailable) {
            ImGui::Separator();
            ImGui::TextWrapped("Codex CLI is not available on PATH.");
            ImGui::End();
            return;
        }

        char modelBuffer[128] = {};
        strncpy(modelBuffer, codexModel.c_str(), sizeof(modelBuffer) - 1);
        if (ImGui::InputText("Model", modelBuffer, sizeof(modelBuffer))) {
            codexModel = modelBuffer;
        }

        const char* sandboxModes[] = {
            "Read Only",
            "Workspace Write",
            "Danger Full Access"
        };
        ImGui::Checkbox("Full Auto", &codexUseFullAuto);
        if (!codexUseFullAuto) {
            ImGui::Combo("Sandbox", &codexSandboxMode, sandboxModes, 3);
        }
        ImGui::Checkbox("Ephemeral Session", &codexEphemeral);
        ImGui::SameLine();
        if (ImGui::Button("Refresh Tasks")) {
            refreshCodexTaskFiles();
        }

        if (!codexTaskFiles.empty()) {
            std::vector<const char*> taskItems;
            taskItems.reserve(codexTaskFiles.size());
            for (const auto& task : codexTaskFiles) {
                taskItems.push_back(task.c_str());
            }
            if (codexSelectedTaskFile < 0 || codexSelectedTaskFile >= static_cast<int>(codexTaskFiles.size())) {
                codexSelectedTaskFile = 0;
            }
            ImGui::Combo("Harness Task", &codexSelectedTaskFile, taskItems.data(), static_cast<int>(taskItems.size()));
        }

        char promptBuffer[4096] = {};
        strncpy(promptBuffer, codexPrompt.c_str(), sizeof(promptBuffer) - 1);
        if (ImGui::InputTextMultiline("Prompt", promptBuffer, sizeof(promptBuffer), ImVec2(-1, 120))) {
            codexPrompt = promptBuffer;
        }

        const bool canRunPrompt = !codexRunInProgress && (!codexPrompt.empty() || !codexTaskFiles.empty());
        if (ImGui::Button("Run Prompt") && canRunPrompt) {
            CodexRunOptions options;
            options.workingDirectory = workingRoot;
            options.model = codexModel;
            options.useFullAuto = codexUseFullAuto;
            options.ephemeral = codexEphemeral;
            options.skipGitRepoCheck = true;
            static const char* sandboxValues[] = { "read-only", "workspace-write", "danger-full-access" };
            options.sandboxMode = sandboxValues[codexSandboxMode];

            std::string prompt = codexPrompt;
            if (!codexTaskFiles.empty() &&
                codexSelectedTaskFile >= 0 &&
                codexSelectedTaskFile < static_cast<int>(codexTaskFiles.size())) {
                prompt += "\n\nPlease follow the task file `" + codexTaskFiles[codexSelectedTaskFile] + "`.";
            }
            if (!currentFile.empty()) {
                prompt += "\nCurrent file in editor: `" + currentFile + "`.";
            }
            if (prompt.empty()) {
                prompt = "Review the current workspace and summarize the next best change to make.";
            }
            options.prompt = prompt;

            codexOutput = "Running Codex...";
            codexStatusMessage = "Running prompt";
            codexRunInProgress = true;
            codexFuture = std::async(std::launch::async, [this, options]() {
                return codexManager->runPrompt(options);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Review Repo") && !codexRunInProgress) {
            CodexRunOptions options;
            options.workingDirectory = workingRoot;
            options.model = codexModel;
            options.useFullAuto = codexUseFullAuto;
            options.ephemeral = codexEphemeral;
            options.skipGitRepoCheck = true;
            static const char* sandboxValues[] = { "read-only", "workspace-write", "danger-full-access" };
            options.sandboxMode = sandboxValues[codexSandboxMode];
            options.prompt = codexPrompt;

            codexOutput = "Running repository review...";
            codexStatusMessage = "Reviewing repository";
            codexRunInProgress = true;
            codexFuture = std::async(std::launch::async, [this, options]() {
                return codexManager->reviewRepository(options);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            codexPrompt.clear();
            codexOutput.clear();
            codexLastCommand.clear();
            codexStatusMessage = "Idle";
        }

        if (codexRunInProgress) {
            ImGui::TextWrapped("Codex is running. Results will appear when the command finishes.");
        }

        if (!codexLastCommand.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("Last command: %s", codexLastCommand.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Output");
        ImGui::BeginChild("CodexOutput", ImVec2(0, 0), true);
        ImGui::TextWrapped("%s", codexOutput.empty() ? "No Codex activity yet." : codexOutput.c_str());
        ImGui::EndChild();
    }
    ImGui::End();
}

void UIManager::renderSettingsWindow() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(420, 360), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Settings", &showSettingsWindow)) {
        if (ImGui::CollapsingHeader("Editor Settings")) {
            ImGui::Checkbox("Syntax Highlighting", &syntaxHighlighting);
            ImGui::Checkbox("Line Numbers", &lineNumbers);
            ImGui::Checkbox("Auto Completion", &autoCompletion);
        }
        
        if (ImGui::CollapsingHeader("Compiler Settings")) {
            ImGui::Text("Compiler Path: /usr/bin/g++");
            ImGui::SliderInt("Optimization Level", &optimizationLevel, 0, 3);
            ImGui::Checkbox("Debug Symbols", &debugSymbols);
            ImGui::Checkbox("Warnings as Errors", &warningsAsErrors);
        }
    }
    ImGui::End();
}

void UIManager::renderAboutWindow() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 220), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("About", &showAboutWindow)) {
        ImGui::Text("CodeForge v1.0");
        ImGui::Text("Professional C++ Integrated Development Environment");
        ImGui::Separator();
        ImGui::Text("Features:");
        ImGui::BulletText("Advanced syntax highlighting");
        ImGui::BulletText("Real-time compilation with error detection");
        ImGui::BulletText("Intelligent code completion");
        ImGui::BulletText("Integrated project management");
        ImGui::BulletText("Multi-compiler support");
        ImGui::BulletText("Cross-platform compatibility");
        ImGui::Separator();
        ImGui::Text("Built with modern C++ and ImGui");
        ImGui::Text("Copyright 2025 - CodeForge Development Team");
    }
    ImGui::End();
}

void UIManager::handleFileMenu() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", "Ctrl+N")) {
            codeEditor->clear();
            currentFile.clear();
        }
        if (ImGui::MenuItem("Open", "Ctrl+O")) {
            showOpenFileDialog = true;
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            if (codeEditor) {
                codeEditor->saveFile(currentFile.empty() ? "untitled.cpp" : currentFile);
            }
        }
        if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
            // Implement save as
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            // Exit application
        }
        ImGui::EndMenu();
    }
}

void UIManager::handleProjectMenu() {
    if (ImGui::BeginMenu("Project")) {
        if (ImGui::MenuItem("New Project", "Ctrl+Shift+N")) {
            showCreateProjectDialog = true;
        }
        if (ImGui::MenuItem("Open Project", "Ctrl+Shift+O")) {
            showOpenFileDialog = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Project", "Ctrl+Shift+S", false, currentProject != nullptr)) {
            saveProject();
        }
        if (ImGui::MenuItem("Close Project", nullptr, false, currentProject != nullptr)) {
            currentProject = nullptr;
            gitStatusLines.clear();
            gitOutput.clear();
            gitCommitMessage.clear();
            codexTaskFiles.clear();
            codexSelectedTaskFile = 0;
            codexStatusMessage = "Idle";
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Project Settings", nullptr, false, currentProject != nullptr)) {
            showProjectSettings = true;
        }
        if (ImGui::MenuItem("Add File to Project", nullptr, false, currentProject != nullptr)) {
            showOpenFileDialog = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Build Project", "F7", false, currentProject != nullptr)) {
            // TODO: Implement build functionality
        }
        if (ImGui::MenuItem("Clean Project", nullptr, false, currentProject != nullptr)) {
            // TODO: Implement clean functionality  
        }
        ImGui::EndMenu();
    }
}

void UIManager::handleEditMenu() {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            if (codeEditor) {
                codeEditor->undo();
            }
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
            if (codeEditor) {
                codeEditor->redo();
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            if (codeEditor) {
                codeEditor->cut();
            }
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            if (codeEditor) {
                codeEditor->copy();
            }
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            if (codeEditor) {
                codeEditor->paste();
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select All", "Ctrl+A")) {
            if (codeEditor) {
                codeEditor->selectAll();
            }
        }
        if (ImGui::MenuItem("Find", "Ctrl+F")) {
            // TODO: Implement find functionality
        }
        if (ImGui::MenuItem("Replace", "Ctrl+H")) {
            // TODO: Implement replace functionality
        }
        ImGui::EndMenu();
    }
}

void UIManager::handleBuildMenu() {
    if (ImGui::BeginMenu("Build")) {
        if (ImGui::MenuItem("Compile", "F7")) {
            // Trigger compilation
        }
        if (ImGui::MenuItem("Build Project", "Ctrl+F7")) {
            // Build entire project
        }
        if (ImGui::MenuItem("Rebuild", "Ctrl+Shift+F7")) {
            // Rebuild project
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Run", "F5")) {
            // Run compiled program
        }
        if (ImGui::MenuItem("Debug", "F10")) {
            // Start debugging
        }
        ImGui::EndMenu();
    }
}

void UIManager::handleViewMenu() {
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Code Editor", nullptr, &showEditorWindow);
        ImGui::MenuItem("Compiler", nullptr, &showCompilerWindow);
        ImGui::MenuItem("Output", nullptr, &showOutputWindow);
        ImGui::MenuItem("Project", nullptr, &showProjectWindow);
        ImGui::MenuItem("Project Manager", nullptr, &showProjectManager);
        ImGui::MenuItem("Git", nullptr, &showGitWindow);
        ImGui::MenuItem("Codex", nullptr, &showCodexWindow);
        if (ImGui::MenuItem("Toggle Fullscreen", "F11")) {
            if (Application::getInstance()) {
                Application::getInstance()->toggleFullscreen();
            }
        }
        ImGui::Separator();
        ImGui::MenuItem("Settings", nullptr, &showSettingsWindow);
        ImGui::EndMenu();
    }
}

void UIManager::handleGitMenu() {
    if (ImGui::BeginMenu("Git")) {
        bool hasProject = currentProject != nullptr;
        bool hasRepo = hasProject && currentProject->gitEnabled;

        if (ImGui::MenuItem("Git Window", nullptr, &showGitWindow)) {
        }
        if (ImGui::MenuItem("Refresh Status", nullptr, false, hasRepo)) {
            refreshGitState();
        }
        if (ImGui::MenuItem("Initialize Repository", nullptr, false, hasProject && !hasRepo)) {
            initializeProjectGitRepository();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Stage All", nullptr, false, hasRepo)) {
            runGitStageAll();
        }
        if (ImGui::MenuItem("Commit", nullptr, false, hasRepo)) {
            runGitCommit();
        }
        if (ImGui::MenuItem("Pull", nullptr, false, hasRepo)) {
            runGitPull();
        }
        if (ImGui::MenuItem("Push", nullptr, false, hasRepo)) {
            runGitPush();
        }
        ImGui::EndMenu();
    }
}

void UIManager::handleCodexMenu() {
    if (ImGui::BeginMenu("Codex")) {
        ImGui::MenuItem("Codex Window", nullptr, &showCodexWindow);
        if (ImGui::MenuItem("Clear Output")) {
            codexOutput.clear();
            codexLastCommand.clear();
            codexStatusMessage = "Idle";
        }
        if (ImGui::MenuItem("Refresh Harness Tasks", nullptr, false, currentProject != nullptr)) {
            refreshCodexTaskFiles();
        }
        ImGui::EndMenu();
    }
}

void UIManager::handleHelpMenu() {
    if (ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow);
        ImGui::MenuItem("About", nullptr, &showAboutWindow);
        ImGui::EndMenu();
    }
}

void UIManager::applyGlossyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Main colors - Dark blue/gray theme with NO transparency
    ImVec4 darkBg = ImVec4(0.08f, 0.10f, 0.15f, 1.00f);      // Solid dark blue-gray
    ImVec4 mediumBg = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);    // Solid medium blue-gray  
    ImVec4 lightBg = ImVec4(0.18f, 0.22f, 0.28f, 1.00f);     // Solid light blue-gray
    ImVec4 accent = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);      // Solid bright blue accent
    ImVec4 accentHover = ImVec4(0.36f, 0.69f, 1.00f, 1.00f); // Solid lighter blue hover
    ImVec4 accentActive = ImVec4(0.16f, 0.49f, 0.88f, 1.00f); // Solid darker blue active
    ImVec4 text = ImVec4(0.90f, 0.92f, 0.95f, 1.00f);        // Solid light gray text
    ImVec4 textDisabled = ImVec4(0.50f, 0.55f, 0.60f, 1.00f); // Solid muted gray
    ImVec4 border = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);      // Solid subtle border
    ImVec4 borderShadow = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // No border shadow
    
    // Solid colors - NO transparency
    ImVec4 frostBg = ImVec4(0.15f, 0.18f, 0.25f, 1.00f);     // Solid background
    ImVec4 frostLight = ImVec4(0.22f, 0.26f, 0.32f, 1.00f);  // Solid light
    ImVec4 frostDark = ImVec4(0.06f, 0.08f, 0.12f, 1.00f);   // Solid dark
    
    // Suppress unused variable warnings
    (void)frostBg;
    (void)frostLight;
    (void)frostDark;
    
    // Apply solid color scheme - NO transparency
    colors[ImGuiCol_Text]                   = text;
    colors[ImGuiCol_TextDisabled]           = textDisabled;
    colors[ImGuiCol_WindowBg]               = darkBg;
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = mediumBg;
    colors[ImGuiCol_Border]                 = border;
    colors[ImGuiCol_BorderShadow]           = borderShadow;
    colors[ImGuiCol_FrameBg]                = mediumBg;
    colors[ImGuiCol_FrameBgHovered]         = lightBg;
    colors[ImGuiCol_FrameBgActive]          = accent;
    colors[ImGuiCol_TitleBg]                = darkBg;
    colors[ImGuiCol_TitleBgActive]          = mediumBg;
    colors[ImGuiCol_TitleBgCollapsed]       = darkBg;
    colors[ImGuiCol_MenuBarBg]              = mediumBg;
    colors[ImGuiCol_ScrollbarBg]            = darkBg;
    colors[ImGuiCol_ScrollbarGrab]          = lightBg;
    colors[ImGuiCol_ScrollbarGrabHovered]   = accent;
    colors[ImGuiCol_ScrollbarGrabActive]    = accentActive;
    colors[ImGuiCol_CheckMark]              = accent;
    colors[ImGuiCol_SliderGrab]             = accent;
    colors[ImGuiCol_SliderGrabActive]       = accentActive;
    colors[ImGuiCol_Button]                 = lightBg;
    colors[ImGuiCol_ButtonHovered]          = accentHover;
    colors[ImGuiCol_ButtonActive]           = accentActive;
    colors[ImGuiCol_Header]                 = lightBg;
    colors[ImGuiCol_HeaderHovered]          = accent;
    colors[ImGuiCol_HeaderActive]           = accentActive;
    colors[ImGuiCol_Separator]              = border;
    colors[ImGuiCol_SeparatorHovered]       = accent;
    colors[ImGuiCol_SeparatorActive]        = accentActive;
    colors[ImGuiCol_ResizeGrip]             = lightBg;
    colors[ImGuiCol_ResizeGripHovered]      = accent;
    colors[ImGuiCol_ResizeGripActive]       = accentActive;
    colors[ImGuiCol_Tab]                    = mediumBg;
    colors[ImGuiCol_TabHovered]             = accentHover;
    colors[ImGuiCol_TabActive]              = accent;
    colors[ImGuiCol_TabUnfocused]           = darkBg;
    colors[ImGuiCol_TabUnfocusedActive]     = mediumBg;
    colors[ImGuiCol_PlotLines]              = accent;
    colors[ImGuiCol_PlotLinesHovered]       = accentHover;
    colors[ImGuiCol_PlotHistogram]          = accent;
    colors[ImGuiCol_PlotHistogramHovered]   = accentHover;
    colors[ImGuiCol_TableHeaderBg]          = mediumBg;
    colors[ImGuiCol_TableBorderStrong]      = border;
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.28f, 0.33f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = accent;
    colors[ImGuiCol_DragDropTarget]         = accentHover;
    colors[ImGuiCol_NavHighlight]           = accent;
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    
    // Style settings for frosted glass blur appearance
    style.WindowPadding     = ImVec2(15.0f, 15.0f);
    style.FramePadding      = ImVec2(10.0f, 6.0f);
    style.CellPadding       = ImVec2(8.0f, 8.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 8.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing     = 30.0f;
    style.ScrollbarSize     = 16.0f;
    style.GrabMinSize       = 12.0f;
    
    // Enhanced rounding for glass blur effect
    style.WindowRounding     = 12.0f;
    style.ChildRounding      = 10.0f;
    style.FrameRounding      = 8.0f;
    style.PopupRounding      = 10.0f;
    style.ScrollbarRounding  = 16.0f;
    style.GrabRounding       = 8.0f;
    style.LogSliderDeadzone  = 6.0f;
    style.TabRounding        = 8.0f;
    
    // Border settings for glass effect
    style.WindowBorderSize = 1.5f;
    style.ChildBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.5f;
    style.FrameBorderSize  = 1.0f;
    style.TabBorderSize    = 0.0f;
    
    // Other style tweaks
    style.WindowTitleAlign       = ImVec2(0.00f, 0.50f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ColorButtonPosition     = ImGuiDir_Right;
    style.ButtonTextAlign        = ImVec2(0.50f, 0.50f);
    style.SelectableTextAlign    = ImVec2(0.00f, 0.00f);
    
    // Alpha settings - NO transparency
    style.Alpha                = 1.00f;  // Fully opaque
    style.DisabledAlpha        = 0.60f;  // Standard disabled transparency
}

void UIManager::enableWindowBlur() {
    // Additional settings to enhance the backdrop blur effect
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Set transparency for backdrop blur effect
    ImVec4* colors = style.Colors;
    
    // Make windows semi-transparent for backdrop blur
    colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.18f, 0.25f, 0.50f);  // Semi-transparent main windows
    
    // Make child windows transparent for layered glass effect
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.12f, 0.18f, 0.35f);   // Transparent child windows
    
    // Make popup windows transparent for backdrop blur
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.18f, 0.25f, 0.55f);   // Semi-transparent popups
    
    // Make menu bar semi-transparent
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.15f, 0.20f, 0.45f); // Semi-transparent menu bar
    
    // Make title bars semi-transparent
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.08f, 0.12f, 0.50f);          // Semi-transparent title
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.15f, 0.20f, 0.60f);    // More opaque when active
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.08f, 0.12f, 0.45f); // Semi-transparent when collapsed
    
    // Enhance separator visibility
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.35f, 0.40f, 0.35f);
    
    // Make modal window dimming subtle for backdrop blur
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.10f);
    
    // Make navigation dimming subtle
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.08f);
    
    // Add window shadow effect through border for depth
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    
    // Make text selection semi-transparent
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.30f);
    
    // Set overall transparency for backdrop blur effect
    style.Alpha = 0.85f;  // Balanced transparency overall
}

void UIManager::renderBackdropBlur() {
    // Create a simulated blur effect using layered transparent overlays
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;
    ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
    
    // Create multiple overlays with slight offsets to simulate blur
    ImU32 blur_color = ImGui::GetColorU32(ImVec4(0.10f, 0.12f, 0.18f, 0.08f));
    
    for (int i = 0; i < 5; ++i) {
        float offset = (float)(i * 2);
        ImVec2 blur_min = ImVec2(viewport_pos.x - offset, viewport_pos.y - offset);
        ImVec2 blur_max = ImVec2(viewport_pos.x + viewport_size.x + offset, 
                                viewport_pos.y + viewport_size.y + offset);
        
        // Draw layered semi-transparent rectangles
        drawList->AddRectFilled(blur_min, blur_max, blur_color);
    }
    
    // Add a final overlay for depth
    ImU32 depth_color = ImGui::GetColorU32(ImVec4(0.05f, 0.08f, 0.12f, 0.15f));
    drawList->AddRectFilled(viewport_pos, 
                           ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y), 
                           depth_color);
}

// File browser implementation
void UIManager::refreshDirectoryContents() {
    directoryFiles.clear();
    directoryFolders.clear();
    
    try {
        if (std::filesystem::exists(currentDirectory)) {
            for (const auto& entry : std::filesystem::directory_iterator(currentDirectory)) {
                if (entry.is_directory()) {
                    directoryFolders.push_back(entry.path().filename().string());
                } else if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    std::string extension = entry.path().extension().string();
                    
                    // Filter for code files
                    if (isSupportedWorkspaceFile(filename, extension)) {
                        directoryFiles.push_back(filename);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
    
    // Sort folders and files alphabetically
    std::sort(directoryFolders.begin(), directoryFolders.end());
    std::sort(directoryFiles.begin(), directoryFiles.end());
}

void UIManager::navigateToDirectory(const std::string& path) {
    try {
        std::filesystem::path newPath = std::filesystem::absolute(path);
        if (std::filesystem::exists(newPath) && std::filesystem::is_directory(newPath)) {
            currentDirectory = newPath.string();
            refreshDirectoryContents();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error navigating to directory: " << e.what() << std::endl;
    }
}

void UIManager::renderFileDialog() {
    if (showOpenFileDialog) {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(500, 360), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::Begin("Open File", &showOpenFileDialog)) {
            // Current directory display
            ImGui::Text("Current Directory: %s", currentDirectory.c_str());
            ImGui::Separator();
            
            // Navigation buttons
            if (ImGui::Button("Parent Directory")) {
                std::filesystem::path parentPath = std::filesystem::path(currentDirectory).parent_path();
                if (!parentPath.empty()) {
                    navigateToDirectory(parentPath.string());
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                refreshDirectoryContents();
            }
            
            ImGui::Separator();
            
            // File and folder listing
            ImGui::BeginChild("FileList", ImVec2(0, -40));
            
            // Folders first
            for (const auto& folder : directoryFolders) {
                if (ImGui::Selectable(("📁 " + folder).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        navigateToDirectory((std::filesystem::path(currentDirectory) / folder).string());
                    }
                }
            }
            
            // Then files
            for (const auto& file : directoryFiles) {
                bool isSelected = (selectedFileName == file);
                if (ImGui::Selectable(("📄 " + file).c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    selectedFileName = file;
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        std::string fullPath = (std::filesystem::path(currentDirectory) / file).string();
                        if (std::filesystem::path(fullPath).extension() == ".cfproj") {
                            loadProject(fullPath);
                            showOpenFileDialog = false;
                            selectedFileName.clear();
                        } else if (codeEditor && codeEditor->loadFile(fullPath)) {
                            currentFile = fullPath;
                            showOpenFileDialog = false;
                            selectedFileName.clear();
                        }
                    }
                }
            }
            
            ImGui::EndChild();
            
            // Bottom buttons
            ImGui::Separator();
            if (ImGui::Button("Open") && !selectedFileName.empty()) {
                std::string fullPath = (std::filesystem::path(currentDirectory) / selectedFileName).string();
                if (std::filesystem::path(fullPath).extension() == ".cfproj") {
                    loadProject(fullPath);
                    showOpenFileDialog = false;
                    selectedFileName.clear();
                } else if (codeEditor && codeEditor->loadFile(fullPath)) {
                    currentFile = fullPath;
                    showOpenFileDialog = false;
                    selectedFileName.clear();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                showOpenFileDialog = false;
                selectedFileName.clear();
            }
        }
        ImGui::End();
    }
}

// Advanced Project Manager Implementation
void UIManager::createNewProject(const std::string& name, const std::string& path) {
    try {
        static const char* templates[] = {
            "Console Application",
            "Static Library",
            "Shared Library",
            "Codex Harness"
        };

        // Create project directory
        std::filesystem::path projectPath = std::filesystem::path(path) / name;
        std::filesystem::create_directories(projectPath);

        std::filesystem::create_directories(projectPath / "docs");
        if (newProjectTemplate == 3) {
            std::filesystem::create_directories(projectPath / "prompts");
            std::filesystem::create_directories(projectPath / "scripts");
            std::filesystem::create_directories(projectPath / "tasks");
            std::filesystem::create_directories(projectPath / ".codex");
        } else {
            std::filesystem::create_directories(projectPath / "src");
            std::filesystem::create_directories(projectPath / "include");
            std::filesystem::create_directories(projectPath / "build");
        }
        
        // Create a new project
        auto project = std::make_shared<Project>();
        project->name = name;
        project->rootPath = projectPath.string();
        project->configFile = (projectPath / (name + ".cfproj")).string();
        project->projectType = templates[newProjectTemplate];
        project->buildTarget = name;
        project->buildConfiguration = "Debug";
        project->autoInitializeGit = initializeGitForNewProjects;
        
        // Create root folder structure
        project->rootFolder = std::make_shared<ProjectFolder>();
        project->rootFolder->name = name;
        project->rootFolder->path = project->rootPath;
        
        if (newProjectTemplate == 3) {
            std::ofstream readmeFile((projectPath / "README.md").string());
            readmeFile << "# " << name << "\n\n";
            readmeFile << "Codex harness workspace for prompts, scripts, and repeatable tasks.\n";
            readmeFile.close();

            std::ofstream agentsFile((projectPath / "AGENTS.md").string());
            agentsFile << "# Agent Guidelines\n\n";
            agentsFile << "- Keep prompts task-focused and reproducible.\n";
            agentsFile << "- Store reusable helpers in `scripts/`.\n";
            agentsFile << "- Capture task inputs and expected outputs in `tasks/`.\n";
            agentsFile.close();

            std::ofstream harnessFile((projectPath / "harness.json").string());
            harnessFile << "{\n";
            harnessFile << "  \"name\": \"" << name << "\",\n";
            harnessFile << "  \"runner\": \"codex\",\n";
            harnessFile << "  \"tasksDir\": \"tasks\",\n";
            harnessFile << "  \"promptsDir\": \"prompts\",\n";
            harnessFile << "  \"scriptsDir\": \"scripts\"\n";
            harnessFile << "}\n";
            harnessFile.close();

            std::ofstream taskFile((projectPath / "tasks" / "example-task.md").string());
            taskFile << "# Example Task\n\n";
            taskFile << "Describe the change to make, the files in scope, and the validation steps.\n";
            taskFile.close();

            std::ofstream promptFile((projectPath / "prompts" / "system.md").string());
            promptFile << "You are working inside the " << name << " Codex harness workspace.\n";
            promptFile.close();

            std::ofstream scriptFile((projectPath / "scripts" / "run_harness.ps1").string());
            scriptFile << "param(\n";
            scriptFile << "    [string]$Task = \"tasks/example-task.md\"\n";
            scriptFile << ")\n\n";
            scriptFile << "Write-Host \"Run Codex against $Task using the workspace prompt files.\"\n";
            scriptFile.close();
        } else {
            std::string mainCppPath = (projectPath / "src" / "main.cpp").string();
            std::ofstream mainFile(mainCppPath);
            mainFile << "#include <iostream>\n\nint main() {\n";
            mainFile << "    std::cout << \"Hello, " << name << "!\" << std::endl;\n";
            mainFile << "    return 0;\n}\n";
            mainFile.close();

            std::string cmakeListsPath = (projectPath / "CMakeLists.txt").string();
            std::ofstream cmakeFile(cmakeListsPath);
            cmakeFile << "cmake_minimum_required(VERSION 3.10)\n";
            cmakeFile << "project(" << name << ")\n\n";
            cmakeFile << "set(CMAKE_CXX_STANDARD 17)\n";
            cmakeFile << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
            cmakeFile << "file(GLOB SOURCES \"src/*.cpp\")\n";
            if (newProjectTemplate == 1) {
                cmakeFile << "add_library(" << name << " STATIC ${SOURCES})\n\n";
            } else if (newProjectTemplate == 2) {
                cmakeFile << "add_library(" << name << " SHARED ${SOURCES})\n\n";
            } else {
                cmakeFile << "add_executable(" << name << " ${SOURCES})\n\n";
            }
            cmakeFile << "target_include_directories(" << name << " PRIVATE include)\n";
            cmakeFile.close();
        }

        if (initializeGitForNewProjects) {
            std::ofstream gitignoreFile((projectPath / ".gitignore").string());
            gitignoreFile << "build/\n";
            gitignoreFile << "*.o\n";
            gitignoreFile << "*.obj\n";
            gitignoreFile << "*.exe\n";
            gitignoreFile << "*.out\n";
            gitignoreFile << "*.pdb\n";
            gitignoreFile.close();
        }
        
        currentProject = project;
        if (currentProject->autoInitializeGit) {
            initializeProjectGitRepository();
        }
        scanProjectFiles(project->rootFolder);
        refreshCodexTaskFiles();
        saveProject();
        
        // Add to recent projects
        recentProjects.insert(recentProjects.begin(), project);
        if (recentProjects.size() > 10) {
            recentProjects.resize(10);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating project: " << e.what() << std::endl;
    }
}

void UIManager::loadProject(const std::string& configPath) {
    try {
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            std::cerr << "Cannot open project file: " << configPath << std::endl;
            return;
        }
        
        auto project = std::make_shared<Project>();
        std::string line;
        
        // Simple config file parser (could be enhanced to use JSON)
        while (std::getline(configFile, line)) {
            if (line.substr(0, 5) == "name=") {
                project->name = line.substr(5);
            } else if (line.substr(0, 10) == "root_path=") {
                project->rootPath = line.substr(10);
            } else if (line.substr(0, 13) == "project_type=") {
                project->projectType = line.substr(13);
            } else if (line.substr(0, 13) == "build_target=") {
                project->buildTarget = line.substr(13);
            } else if (line.substr(0, 13) == "build_config=") {
                project->buildConfiguration = line.substr(13);
            } else if (line.substr(0, 13) == "include_path=") {
                project->includePaths.push_back(line.substr(13));
            } else if (line.substr(0, 13) == "library_path=") {
                project->libraryPaths.push_back(line.substr(13));
            } else if (line.substr(0, 8) == "library=") {
                project->libraries.push_back(line.substr(8));
            } else if (line.substr(0, 12) == "git_enabled=") {
                project->gitEnabled = (line.substr(12) == "true");
            } else if (line.substr(0, 9) == "git_root=") {
                project->gitRepositoryRoot = line.substr(9);
            } else if (line.substr(0, 11) == "git_branch=") {
                project->gitBranch = line.substr(11);
            } else if (line.substr(0, 14) == "git_auto_init=") {
                project->autoInitializeGit = (line.substr(14) == "true");
            }
        }
        
        project->configFile = configPath;
        
        // Create root folder
        project->rootFolder = std::make_shared<ProjectFolder>();
        project->rootFolder->name = project->name;
        project->rootFolder->path = project->rootPath;
        
        currentProject = project;
        scanProjectFiles(project->rootFolder);
        refreshCodexTaskFiles();
        refreshGitState();
        recentProjects.insert(recentProjects.begin(), project);
        if (recentProjects.size() > 10) {
            recentProjects.resize(10);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading project: " << e.what() << std::endl;
    }
}

void UIManager::saveProject() {
    if (!currentProject) return;
    
    try {
        std::ofstream configFile(currentProject->configFile);
        configFile << "name=" << currentProject->name << "\n";
        configFile << "root_path=" << currentProject->rootPath << "\n";
        configFile << "project_type=" << currentProject->projectType << "\n";
        configFile << "build_target=" << currentProject->buildTarget << "\n";
        configFile << "build_config=" << currentProject->buildConfiguration << "\n";
        configFile << "git_enabled=" << (currentProject->gitEnabled ? "true" : "false") << "\n";
        configFile << "git_root=" << currentProject->gitRepositoryRoot << "\n";
        configFile << "git_branch=" << currentProject->gitBranch << "\n";
        configFile << "git_auto_init=" << (currentProject->autoInitializeGit ? "true" : "false") << "\n";
        
        for (const auto& includePath : currentProject->includePaths) {
            configFile << "include_path=" << includePath << "\n";
        }
        
        for (const auto& libPath : currentProject->libraryPaths) {
            configFile << "library_path=" << libPath << "\n";
        }
        
        for (const auto& lib : currentProject->libraries) {
            configFile << "library=" << lib << "\n";
        }
        
        configFile.close();
    } catch (const std::exception& e) {
        std::cerr << "Error saving project: " << e.what() << std::endl;
    }
}

void UIManager::refreshCodexTaskFiles() {
    codexTaskFiles.clear();

    if (!currentProject) {
        codexSelectedTaskFile = 0;
        return;
    }

    std::filesystem::path taskRoot = std::filesystem::path(currentProject->rootPath) / "tasks";
    if (!std::filesystem::exists(taskRoot) || !std::filesystem::is_directory(taskRoot)) {
        codexSelectedTaskFile = 0;
        return;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(taskRoot)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string extension = toLowerCopy(entry.path().extension().string());
            if (extension == ".md" || extension == ".txt") {
                codexTaskFiles.push_back(std::filesystem::relative(entry.path(), currentProject->rootPath).string());
            }
        }
    } catch (const std::exception& e) {
        codexOutput = std::string("Failed to scan Codex task files: ") + e.what();
    }

    std::sort(codexTaskFiles.begin(), codexTaskFiles.end());
    if (codexSelectedTaskFile >= static_cast<int>(codexTaskFiles.size())) {
        codexSelectedTaskFile = codexTaskFiles.empty() ? 0 : static_cast<int>(codexTaskFiles.size()) - 1;
    }
}

void UIManager::refreshGitState() {
    gitStatusLines.clear();

    if (!currentProject || !gitManager || !gitManager->isGitAvailable()) {
        return;
    }

    std::string repoRoot = gitManager->findRepositoryRoot(currentProject->rootPath);
    currentProject->gitEnabled = !repoRoot.empty();
    currentProject->gitRepositoryRoot = repoRoot;
    currentProject->gitBranch = currentProject->gitEnabled ? gitManager->getCurrentBranch(repoRoot) : "";

    if (!currentProject->gitEnabled) {
        return;
    }

    std::vector<GitStatusEntry> entries = gitManager->getStatus(currentProject->gitRepositoryRoot);
    for (const auto& entry : entries) {
        gitStatusLines.push_back(entry.indexStatus + entry.workTreeStatus + " " + entry.path);
    }
}

void UIManager::appendGitOutput(const std::string& message) {
    gitOutput = message;
}

void UIManager::runGitStageAll() {
    if (!currentProject || !currentProject->gitEnabled) {
        return;
    }

    GitOperationResult result = gitManager->stageAll(currentProject->gitRepositoryRoot);
    appendGitOutput(result.output.empty() ? (result.success ? "Staged all files." : "Failed to stage files.") : result.output);
    refreshGitState();
    saveProject();
}

void UIManager::runGitCommit() {
    if (!currentProject || !currentProject->gitEnabled || gitCommitMessage.empty()) {
        return;
    }

    GitOperationResult result = gitManager->commit(currentProject->gitRepositoryRoot, gitCommitMessage);
    appendGitOutput(result.output.empty() ? (result.success ? "Commit created." : "Commit failed.") : result.output);
    if (result.success) {
        gitCommitMessage.clear();
    }
    refreshGitState();
    saveProject();
}

void UIManager::runGitPull() {
    if (!currentProject || !currentProject->gitEnabled) {
        return;
    }

    GitOperationResult result = gitManager->pull(currentProject->gitRepositoryRoot);
    appendGitOutput(result.output.empty() ? (result.success ? "Pull completed." : "Pull failed.") : result.output);
    refreshGitState();
}

void UIManager::runGitPush() {
    if (!currentProject || !currentProject->gitEnabled) {
        return;
    }

    GitOperationResult result = gitManager->push(currentProject->gitRepositoryRoot);
    appendGitOutput(result.output.empty() ? (result.success ? "Push completed." : "Push failed.") : result.output);
    refreshGitState();
}

void UIManager::initializeProjectGitRepository() {
    if (!currentProject || !gitManager) {
        return;
    }

    GitOperationResult result = gitManager->initRepository(currentProject->rootPath);
    appendGitOutput(result.output.empty() ? (result.success ? "Repository initialized." : "Repository initialization failed.") : result.output);
    refreshGitState();
    saveProject();
}

void UIManager::scanProjectFiles(std::shared_ptr<ProjectFolder> folder) {
    if (!folder) return;
    
    try {
        folder->files.clear();
        folder->subFolders.clear();
        
        if (std::filesystem::exists(folder->path)) {
            for (const auto& entry : std::filesystem::directory_iterator(folder->path)) {
                if (entry.is_directory()) {
                    if (entry.path().filename() == ".git") {
                        continue;
                    }
                    auto subFolder = std::make_shared<ProjectFolder>();
                    subFolder->name = entry.path().filename().string();
                    subFolder->path = entry.path().string();
                    subFolder->isExpanded = false;
                    folder->subFolders.push_back(subFolder);
                    scanProjectFiles(subFolder);
                } else if (entry.is_regular_file()) {
                    ProjectFile file;
                    file.name = entry.path().filename().string();
                    file.fullPath = entry.path().string();
                    file.relativePath = std::filesystem::relative(entry.path(), currentProject->rootPath).string();
                    file.extension = entry.path().extension().string();
                    
                    // Only include relevant file types
                    if (isSupportedWorkspaceFile(file.name, file.extension)) {
                        folder->files.push_back(file);
                    }
                }
            }
        }
        
        // Sort files and folders
        std::sort(folder->files.begin(), folder->files.end(), 
                 [](const ProjectFile& a, const ProjectFile& b) {
                     return a.name < b.name;
                 });
        
        std::sort(folder->subFolders.begin(), folder->subFolders.end(),
                 [](const std::shared_ptr<ProjectFolder>& a, const std::shared_ptr<ProjectFolder>& b) {
                     return a->name < b->name;
                 });
                 
    } catch (const std::exception& e) {
        std::cerr << "Error scanning project files: " << e.what() << std::endl;
    }
}

void UIManager::renderProjectTree(std::shared_ptr<ProjectFolder> folder) {
    if (!folder) return;
    
    // Render folders
    for (auto& subFolder : folder->subFolders) {
        bool nodeOpen = ImGui::TreeNodeEx(("📁 " + subFolder->name).c_str(), 
                                         subFolder->isExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        if (nodeOpen) {
            subFolder->isExpanded = true;
            renderProjectTree(subFolder);
            ImGui::TreePop();
        } else {
            subFolder->isExpanded = false;
        }
    }
    
    // Render files
    for (auto& file : folder->files) {
        std::string icon = "📄";
        if (file.extension == ".cpp" || file.extension == ".c") icon = "📝";
        else if (file.extension == ".h" || file.extension == ".hpp") icon = "📋";
        else if (file.extension == ".cmake" || file.name == "CMakeLists.txt") icon = "⚙️";
        else if (file.extension == ".md") icon = "📖";
        
        icon = getWorkspaceFileIcon(file.name, file.extension);
        std::string displayName = icon + " " + file.name;
        if (file.isModified) displayName += " *";
        
        if (ImGui::Selectable(displayName.c_str(), file.isOpen, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                // Open file in editor
                if (codeEditor && codeEditor->loadFile(file.fullPath)) {
                    file.isOpen = true;
                    currentFile = file.fullPath;
                }
            }
        }
        
        // Context menu for files
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Open")) {
                if (codeEditor) {
                    codeEditor->loadFile(file.fullPath);
                    file.isOpen = true;
                    currentFile = file.fullPath;
                }
            }
            if (ImGui::MenuItem("Remove from Project")) {
                removeFileFromProject(file.fullPath);
            }
            if (ImGui::MenuItem("Delete File")) {
                if (std::filesystem::exists(file.fullPath)) {
                    std::filesystem::remove(file.fullPath);
                    scanProjectFiles(currentProject->rootFolder);
                }
            }
            ImGui::EndPopup();
        }
    }
}

void UIManager::renderCreateProjectDialog() {
    if (showCreateProjectDialog) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(420, 320), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::Begin("Create New Project", &showCreateProjectDialog)) {
            ImGui::Text("Create a new C++ project");
            ImGui::Separator();
            
            // Project name input
            char nameBuffer[256] = {};
            strncpy(nameBuffer, newProjectName.c_str(), sizeof(nameBuffer) - 1);
            if (ImGui::InputText("Project Name", nameBuffer, sizeof(nameBuffer))) {
                newProjectName = nameBuffer;
            }
            
            // Project path input
            char pathBuffer[512] = {};
            strncpy(pathBuffer, newProjectPath.c_str(), sizeof(pathBuffer) - 1);
            if (ImGui::InputText("Project Path", pathBuffer, sizeof(pathBuffer))) {
                newProjectPath = pathBuffer;
            }
            ImGui::SameLine();
            if (ImGui::Button("Browse")) {
                browserCurrentPath = newProjectPath.empty() ? currentDirectory : newProjectPath;
                refreshBrowserDirectories();
                browserPurpose = "project_path";
                showDirectoryBrowser = true;
            }
            
            // Project template
            const char* templates[] = {
                "Console Application",
                "Static Library",
                "Shared Library",
                "Codex Harness"
            };
            ImGui::Combo("Template", &newProjectTemplate, templates, 4);
            ImGui::Checkbox("Initialize Git repository", &initializeGitForNewProjects);
            
            ImGui::Separator();
            
            // Buttons
            if (ImGui::Button("Create") && !newProjectName.empty() && !newProjectPath.empty()) {
                createNewProject(newProjectName, newProjectPath);
                showCreateProjectDialog = false;
                newProjectName.clear();
                newProjectPath.clear();
                newProjectTemplate = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                showCreateProjectDialog = false;
                newProjectName.clear();
                newProjectPath.clear();
                newProjectTemplate = 0;
            }
        }
        ImGui::End();
    }
}

void UIManager::renderProjectSettings() {
    if (showProjectSettings && currentProject) {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(420, 320), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::Begin("Project Settings", &showProjectSettings)) {
            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
                char nameBuffer[256] = {};
                strncpy(nameBuffer, currentProject->name.c_str(), sizeof(nameBuffer) - 1);
                if (ImGui::InputText("Project Name", nameBuffer, sizeof(nameBuffer))) {
                    currentProject->name = nameBuffer;
                }

                ImGui::Text("Project Type: %s", currentProject->projectType.c_str());
                
                char targetBuffer[256] = {};
                strncpy(targetBuffer, currentProject->buildTarget.c_str(), sizeof(targetBuffer) - 1);
                if (ImGui::InputText("Build Target", targetBuffer, sizeof(targetBuffer))) {
                    currentProject->buildTarget = targetBuffer;
                }
            }
            
            if (ImGui::CollapsingHeader("Build Settings")) {
                const char* configs[] = { "Debug", "Release", "RelWithDebInfo", "MinSizeRel" };
                static int currentConfig = 0;
                for (int i = 0; i < 4; i++) {
                    if (currentProject->buildConfiguration == configs[i]) {
                        currentConfig = i;
                        break;
                    }
                }
                if (ImGui::Combo("Configuration", &currentConfig, configs, 4)) {
                    currentProject->buildConfiguration = configs[currentConfig];
                }
            }

            if (ImGui::CollapsingHeader("Git", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Enable Git for this project", &currentProject->autoInitializeGit);
                ImGui::Text("Repository: %s",
                            currentProject->gitEnabled ? currentProject->gitRepositoryRoot.c_str() : "Not initialized");
                ImGui::Text("Branch: %s",
                            currentProject->gitEnabled ? currentProject->gitBranch.c_str() : "-");

                if (!currentProject->gitEnabled) {
                    if (ImGui::Button("Initialize Repository")) {
                        initializeProjectGitRepository();
                    }
                } else {
                    if (ImGui::Button("Refresh Git Status")) {
                        refreshGitState();
                    }
                }
            }
            
            if (ImGui::CollapsingHeader("Include Paths")) {
                for (size_t i = 0; i < currentProject->includePaths.size(); i++) {
                    ImGui::Text("%s", currentProject->includePaths[i].c_str());
                    ImGui::SameLine();
                    if (ImGui::Button(("Remove##include" + std::to_string(i)).c_str())) {
                        currentProject->includePaths.erase(currentProject->includePaths.begin() + i);
                        break;
                    }
                }
                if (ImGui::Button("Add Include Path")) {
                    browserCurrentPath = currentProject->rootPath;
                    refreshBrowserDirectories();
                    browserPurpose = "include_path";
                    showDirectoryBrowser = true;
                }
            }
            
            if (ImGui::CollapsingHeader("Library Paths")) {
                for (size_t i = 0; i < currentProject->libraryPaths.size(); i++) {
                    ImGui::Text("%s", currentProject->libraryPaths[i].c_str());
                    ImGui::SameLine();
                    if (ImGui::Button(("Remove##libpath" + std::to_string(i)).c_str())) {
                        currentProject->libraryPaths.erase(currentProject->libraryPaths.begin() + i);
                        break;
                    }
                }
                if (ImGui::Button("Add Library Path")) {
                    browserCurrentPath = currentProject->rootPath;
                    refreshBrowserDirectories();
                    browserPurpose = "library_path";
                    showDirectoryBrowser = true;
                }
            }
            
            if (ImGui::CollapsingHeader("Libraries")) {
                for (size_t i = 0; i < currentProject->libraries.size(); i++) {
                    ImGui::Text("%s", currentProject->libraries[i].c_str());
                    ImGui::SameLine();
                    if (ImGui::Button(("Remove##lib" + std::to_string(i)).c_str())) {
                        currentProject->libraries.erase(currentProject->libraries.begin() + i);
                        break;
                    }
                }
                if (ImGui::Button("Add Library")) {
                    currentProject->libraries.push_back("pthread");
                }
            }
            
            ImGui::Separator();
            if (ImGui::Button("Save")) {
                saveProject();
                showProjectSettings = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                showProjectSettings = false;
            }
        }
        ImGui::End();
    }
}

void UIManager::addFileToProject(const std::string& filepath) {
    if (!currentProject) return;
    
    try {
        // Copy file to project if it's outside the project directory
        std::filesystem::path filePath(filepath);
        std::filesystem::path projectPath(currentProject->rootPath);
        
        if (filepath.substr(0, currentProject->rootPath.length()) != currentProject->rootPath) {
            std::filesystem::path destPath = projectPath / "src" / filePath.filename();
            std::filesystem::copy_file(filePath, destPath);
        }
        
        // Refresh project files
        scanProjectFiles(currentProject->rootFolder);
        
    } catch (const std::exception& e) {
        std::cerr << "Error adding file to project: " << e.what() << std::endl;
    }
}

void UIManager::removeFileFromProject(const std::string& filepath) {
    (void)filepath;  // Suppress unused parameter warning
    if (!currentProject) return;
    
    // This would remove the file reference from project, not delete the actual file
    // For now, just refresh the project files
    scanProjectFiles(currentProject->rootFolder);
}

void UIManager::refreshBrowserDirectories() {
    browserDirectories.clear();
    
    try {
        if (std::filesystem::exists(browserCurrentPath) && std::filesystem::is_directory(browserCurrentPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(browserCurrentPath)) {
                if (entry.is_directory()) {
                    browserDirectories.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory for browser: " << e.what() << std::endl;
    }
    
    // Sort directories alphabetically
    std::sort(browserDirectories.begin(), browserDirectories.end());
}

void UIManager::renderDirectoryBrowser() {
    if (showDirectoryBrowser) {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        
        std::string windowTitle = "Select Directory";
        if (browserPurpose == "project_path") {
            windowTitle = "Select Project Directory";
        } else if (browserPurpose == "include_path") {
            windowTitle = "Select Include Directory";
        } else if (browserPurpose == "library_path") {
            windowTitle = "Select Library Directory";
        }
        
        ImGui::SetNextWindowSizeConstraints(ImVec2(500, 360), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::Begin(windowTitle.c_str(), &showDirectoryBrowser)) {
            // Current path display
            ImGui::Text("Current Path:");
            ImGui::SameLine();
            ImGui::TextWrapped("%s", browserCurrentPath.c_str());
            ImGui::Separator();
            
            // Navigation buttons
            if (ImGui::Button("Parent Directory")) {
                std::filesystem::path parentPath = std::filesystem::path(browserCurrentPath).parent_path();
                if (!parentPath.empty()) {
                    browserCurrentPath = parentPath.string();
                    refreshBrowserDirectories();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                refreshBrowserDirectories();
            }
            ImGui::SameLine();
            if (ImGui::Button("Home")) {
                const char* homeDir = getenv("HOME");
                if (homeDir) {
                    browserCurrentPath = homeDir;
                    refreshBrowserDirectories();
                }
            }
            
            ImGui::Separator();
            
            // Directory listing
            ImGui::BeginChild("DirectoryList", ImVec2(0, -40));
            
            // Handle keyboard navigation
            if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                // Enter key - select first directory if any
                if (!browserDirectories.empty()) {
                    browserCurrentPath = (std::filesystem::path(browserCurrentPath) / browserDirectories[0]).string();
                    refreshBrowserDirectories();
                }
            }
            
            // Show directories
            for (const auto& dir : browserDirectories) {
                if (ImGui::Selectable(("📁 " + dir).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        browserCurrentPath = (std::filesystem::path(browserCurrentPath) / dir).string();
                        refreshBrowserDirectories();
                    }
                }
            }
            
            ImGui::EndChild();
            
            // Bottom buttons
            ImGui::Separator();
            if (ImGui::Button("Select This Directory")) {
                if (browserPurpose == "project_path") {
                    newProjectPath = browserCurrentPath;
                } else if (browserPurpose == "include_path" && currentProject) {
                    currentProject->includePaths.push_back(browserCurrentPath);
                } else if (browserPurpose == "library_path" && currentProject) {
                    currentProject->libraryPaths.push_back(browserCurrentPath);
                }
                showDirectoryBrowser = false;
                browserPurpose = "";
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                showDirectoryBrowser = false;
                browserPurpose = "";
            }
            
            // Show selected path
            ImGui::Text("Selected: %s", browserCurrentPath.c_str());
        }
        ImGui::End();
    }
}
