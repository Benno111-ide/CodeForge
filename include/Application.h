#pragma once
#include <string>
#include <memory>

// Forward declarations
class UIManager;
class CompilerEngine;
class FileManager;
class ProjectManager;
class SettingsManager;
class BuildSystem;
struct GLFWwindow;

class Application {
public:
    Application();
    ~Application();
    
    bool initialize();
    void run();
    void shutdown();
    
    // Getters for managers
    UIManager* getUIManager() const { return uiManager.get(); }
    CompilerEngine* getCompilerEngine() const { return compilerEngine.get(); }
    FileManager* getFileManager() const { return fileManager.get(); }
    ProjectManager* getProjectManager() const { return projectManager.get(); }
    SettingsManager* getSettingsManager() const { return settingsManager.get(); }
    BuildSystem* getBuildSystem() const { return buildSystem.get(); }
    
    GLFWwindow* getWindow() const { return window; }
    bool isRunning() const { return running; }
    
    // Fullscreen control
    void toggleFullscreen();
    bool isFullscreen() const;

    static Application* getInstance() { return instance; }
    
private:
    static Application* instance;
    
    GLFWwindow* window;
    bool running;
    bool fullscreen = false;
    int windowedPosX = 100;
    int windowedPosY = 100;
    int windowedWidth = 1280;
    int windowedHeight = 720;
    
    // Manager instances
    std::unique_ptr<UIManager> uiManager;
    std::unique_ptr<CompilerEngine> compilerEngine;
    std::unique_ptr<FileManager> fileManager;
    std::unique_ptr<ProjectManager> projectManager;
    std::unique_ptr<SettingsManager> settingsManager;
    std::unique_ptr<BuildSystem> buildSystem;
    
    void initializeWindow();
    void initializeManagers();
    void mainLoop();
    void handleEvents();
    void update(float deltaTime);
    void render();
    
    // Callbacks
    static void errorCallback(int error, const char* description);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};