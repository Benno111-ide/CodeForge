#include "UIManager.h"
#include "../include/CodeEditor.h"
#include "../include/Application.h"
#include "../include/CompilerEngine.h"
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

UIManager::UIManager() {
    codeEditor = std::make_unique<CodeEditor>();
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
    // Update UI state
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
                    codeEditor->saveFile("untitled.cpp");
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_N)) {
                if (codeEditor) {
                    codeEditor->clear();
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
            ImGui::Text("Path: %s", currentProject->rootPath.c_str());
            ImGui::Text("Build Target: %s", currentProject->buildTarget.c_str());
            
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
                    }
                }
            }
        }
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
        }
        if (ImGui::MenuItem("Open", "Ctrl+O")) {
            showOpenFileDialog = true;
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            // TODO: Implement save to current file or show save dialog
            if (codeEditor) {
                // For now, save to a default file
                codeEditor->saveFile("untitled.cpp");
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
                    if (extension == ".cpp" || extension == ".h" || extension == ".hpp" || 
                        extension == ".c" || extension == ".cc" || extension == ".cxx" ||
                        extension == ".txt" || extension == ".md" || extension == ".py" ||
                        extension == ".js" || extension == ".java" || extension == ".cs") {
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
                        // Open the file
                        std::string fullPath = (std::filesystem::path(currentDirectory) / file).string();
                        if (codeEditor && codeEditor->loadFile(fullPath)) {
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
                if (codeEditor && codeEditor->loadFile(fullPath)) {
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
        // Create project directory
        std::filesystem::path projectPath = std::filesystem::path(path) / name;
        std::filesystem::create_directories(projectPath);
        
        // Create subdirectories
        std::filesystem::create_directories(projectPath / "src");
        std::filesystem::create_directories(projectPath / "include");
        std::filesystem::create_directories(projectPath / "build");
        std::filesystem::create_directories(projectPath / "docs");
        
        // Create a new project
        auto project = std::make_shared<Project>();
        project->name = name;
        project->rootPath = projectPath.string();
        project->configFile = (projectPath / (name + ".cfproj")).string();
        project->buildTarget = name;
        project->buildConfiguration = "Debug";
        
        // Create root folder structure
        project->rootFolder = std::make_shared<ProjectFolder>();
        project->rootFolder->name = name;
        project->rootFolder->path = project->rootPath;
        
        // Create basic project files
        std::string mainCppPath = (projectPath / "src" / "main.cpp").string();
        std::ofstream mainFile(mainCppPath);
        mainFile << "#include <iostream>\n\nint main() {\n";
        mainFile << "    std::cout << \"Hello, " << name << "!\" << std::endl;\n";
        mainFile << "    return 0;\n}\n";
        mainFile.close();
        
        // Create CMakeLists.txt
        std::string cmakeListsPath = (projectPath / "CMakeLists.txt").string();
        std::ofstream cmakeFile(cmakeListsPath);
        cmakeFile << "cmake_minimum_required(VERSION 3.10)\n";
        cmakeFile << "project(" << name << ")\n\n";
        cmakeFile << "set(CMAKE_CXX_STANDARD 17)\n";
        cmakeFile << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
        cmakeFile << "file(GLOB SOURCES \"src/*.cpp\")\n";
        cmakeFile << "add_executable(" << name << " ${SOURCES})\n\n";
        cmakeFile << "target_include_directories(" << name << " PRIVATE include)\n";
        cmakeFile.close();
        
        currentProject = project;
        scanProjectFiles(project->rootFolder);
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
            } else if (line.substr(0, 13) == "build_target=") {
                project->buildTarget = line.substr(13);
            } else if (line.substr(0, 13) == "build_config=") {
                project->buildConfiguration = line.substr(13);
            }
        }
        
        project->configFile = configPath;
        
        // Create root folder
        project->rootFolder = std::make_shared<ProjectFolder>();
        project->rootFolder->name = project->name;
        project->rootFolder->path = project->rootPath;
        
        currentProject = project;
        scanProjectFiles(project->rootFolder);
        
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
        configFile << "build_target=" << currentProject->buildTarget << "\n";
        configFile << "build_config=" << currentProject->buildConfiguration << "\n";
        
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

void UIManager::scanProjectFiles(std::shared_ptr<ProjectFolder> folder) {
    if (!folder) return;
    
    try {
        folder->files.clear();
        folder->subFolders.clear();
        
        if (std::filesystem::exists(folder->path)) {
            for (const auto& entry : std::filesystem::directory_iterator(folder->path)) {
                if (entry.is_directory()) {
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
                    if (file.extension == ".cpp" || file.extension == ".h" || file.extension == ".hpp" ||
                        file.extension == ".c" || file.extension == ".cc" || file.extension == ".cxx" ||
                        file.extension == ".txt" || file.extension == ".md" || file.extension == ".cmake" ||
                        file.name == "CMakeLists.txt" || file.name == "Makefile") {
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
        
        std::string displayName = icon + " " + file.name;
        if (file.isModified) displayName += " *";
        
        if (ImGui::Selectable(displayName.c_str(), file.isOpen, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                // Open file in editor
                if (codeEditor && codeEditor->loadFile(file.fullPath)) {
                    file.isOpen = true;
                }
            }
        }
        
        // Context menu for files
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Open")) {
                if (codeEditor) {
                    codeEditor->loadFile(file.fullPath);
                    file.isOpen = true;
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
            static int selectedTemplate = 0;
            const char* templates[] = { "Console Application", "Static Library", "Shared Library" };
            ImGui::Combo("Template", &selectedTemplate, templates, 3);
            
            ImGui::Separator();
            
            // Buttons
            if (ImGui::Button("Create") && !newProjectName.empty() && !newProjectPath.empty()) {
                createNewProject(newProjectName, newProjectPath);
                showCreateProjectDialog = false;
                newProjectName.clear();
                newProjectPath.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                showCreateProjectDialog = false;
                newProjectName.clear();
                newProjectPath.clear();
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
