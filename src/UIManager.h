#pragma once
#include <string>
#include <vector>
#include <memory>

class CodeEditor;
class CompilerEngine;
class GitManager;

class UIManager {
public:
    UIManager();
    ~UIManager();
    
    void update(float deltaTime);
    void render();
    
    // UI State
    bool showMainMenuBar = true;
    bool showDemoWindow = false;
    bool showAboutWindow = false;
    bool showSettingsWindow = false;
    bool showCompilerWindow = false;
    bool showEditorWindow = true;
    bool showOutputWindow = false;
    bool showProjectWindow = false;
    bool showProjectManager = true;
    bool showGitWindow = true;
    
private:
    std::unique_ptr<CodeEditor> codeEditor;
    
    // File browser state
    std::string currentDirectory;
    std::vector<std::string> directoryFiles;
    std::vector<std::string> directoryFolders;
    bool showOpenFileDialog = false;
    bool showSaveFileDialog = false;
    std::string selectedFileName;
    
    // Advanced Project Manager state
    struct ProjectFile {
        std::string name;
        std::string fullPath;
        std::string relativePath;
        std::string extension;
        bool isModified = false;
        bool isOpen = false;
    };
    
    struct ProjectFolder {
        std::string name;
        std::string path;
        std::vector<ProjectFile> files;
        std::vector<std::shared_ptr<ProjectFolder>> subFolders;
        bool isExpanded = true;
    };
    
    struct Project {
        std::string name;
        std::string rootPath;
        std::string configFile;
        std::string projectType = "Console Application";
        std::vector<std::string> includePaths;
        std::vector<std::string> libraryPaths;
        std::vector<std::string> libraries;
        std::string buildTarget;
        std::string buildConfiguration;
        bool gitEnabled = false;
        bool autoInitializeGit = true;
        std::string gitRepositoryRoot;
        std::string gitBranch;
        std::shared_ptr<ProjectFolder> rootFolder;
    };
    
    std::shared_ptr<Project> currentProject;
    std::vector<std::shared_ptr<Project>> recentProjects;
    bool showCreateProjectDialog = false;
    bool showProjectSettings = false;
    bool showDirectoryBrowser = false;
    bool initializeGitForNewProjects = true;
    int newProjectTemplate = 0;
    std::string browserPurpose = ""; // "project_path", "include_path", etc.
    std::string newProjectName;
    std::string newProjectPath;
    std::string browserCurrentPath;
    std::vector<std::string> browserDirectories;
    std::unique_ptr<GitManager> gitManager;
    std::vector<std::string> gitStatusLines;
    std::string gitOutput;
    std::string gitCommitMessage;
    
    void refreshDirectoryContents();
    void navigateToDirectory(const std::string& path);
    void renderFileDialog();
    
    // Advanced Project Manager methods
    void createNewProject(const std::string& name, const std::string& path);
    void loadProject(const std::string& configPath);
    void saveProject();
    void scanProjectFiles(std::shared_ptr<ProjectFolder> folder);
    void renderProjectManager();
    void renderCreateProjectDialog();
    void renderProjectSettings();
    void renderDirectoryBrowser();
    void renderGitWindow();
    void refreshBrowserDirectories();
    void addFileToProject(const std::string& filepath);
    void removeFileFromProject(const std::string& filepath);
    void renderProjectTree(std::shared_ptr<ProjectFolder> folder);
    void refreshGitState();
    void appendGitOutput(const std::string& message);
    void runGitStageAll();
    void runGitCommit();
    void runGitPull();
    void runGitPush();
    void initializeProjectGitRepository();
    
    void renderMainMenuBar();
    void renderDockSpace();
    void renderCodeEditor();
    void renderCompilerWindow();
    void renderOutputWindow();
    void renderProjectWindow();
    void renderSettingsWindow();
    void renderAboutWindow();
    
    // Theme management
    void applyGlossyTheme();
    void enableWindowBlur();
    void renderBackdropBlur();
    
    // Helper methods
    void handleFileMenu();
    void handleProjectMenu();
    void handleEditMenu();
    void handleBuildMenu();
    void handleViewMenu();
    void handleGitMenu();
    void handleHelpMenu();
    
    // UI State
    std::string currentFile;
    std::vector<std::string> recentFiles;
    std::string compilerOutput;
    
    // Settings state variables
    bool syntaxHighlighting = true;
    bool lineNumbers = true;
    bool autoCompletion = true;
    int optimizationLevel = 1;
    bool debugSymbols = true;
    bool warningsAsErrors = false;
};
