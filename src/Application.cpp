#include "../include/Application.h"
#include "../include/CompilerEngine.h"
#include "UIManager.h"
#include "FileManager.h"
#include "ProjectManager.h"
#include "SettingsManager.h"
#include "BuildSystem.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <chrono>
#include <csignal>
#include <fstream>
#include <cstring>
#include <ctime>
#include <fcntl.h>

#ifdef __linux__
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#ifdef __linux__
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

Application* Application::instance = nullptr;

// Global flag to prevent recursive crash handling
static bool crashHandlerActive = false;

// Crash dump functionality
void generateCrashDump(const char* signal_name, int sig) {
    // Prevent recursive crash handling
    if (crashHandlerActive) {
        std::cerr << "Recursive crash detected! Exiting immediately." << std::endl;
        _exit(1);
    }
    crashHandlerActive = true;
    
    try {
        std::ofstream crashFile("codeforge_crash_dump.txt", std::ios::app);
        
        // Get current time
        time_t rawtime;
        struct tm* timeinfo;
        char timestr[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        crashFile << "\n=== CODEFORGE CRASH DUMP ===" << std::endl;
        crashFile << "Time: " << timestr << std::endl;
        crashFile << "Signal: " << signal_name << " (" << sig << ")" << std::endl;
        crashFile << "PID: " << getpid() << std::endl;
        
#ifdef __linux__
        // Get stack trace with error handling
        void* array[20];
        size_t size = backtrace(array, 20);
        
        crashFile << "\nStack trace:" << std::endl;
        if (size > 0) {
            // Use backtrace_symbols_fd which is safer than backtrace_symbols
            int fd = open("temp_backtrace.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd != -1) {
                backtrace_symbols_fd(array, size, fd);
                close(fd);
                
                std::ifstream tempFile("temp_backtrace.txt");
                std::string line;
                size_t i = 0;
                while (std::getline(tempFile, line) && i < size) {
                    crashFile << "  " << i << ": " << line << std::endl;
                    i++;
                }
                tempFile.close();
                remove("temp_backtrace.txt");
            } else {
                crashFile << "  Unable to write stack trace" << std::endl;
            }
        } else {
            crashFile << "  No stack trace available" << std::endl;
        }
#endif
        
        // Application state information
        if (Application::getInstance()) {
            crashFile << "\nApplication State:" << std::endl;
            crashFile << "  Window: " << (Application::getInstance()->getWindow() ? "Valid" : "NULL") << std::endl;
            crashFile << "  Running: " << (Application::getInstance()->isRunning() ? "Yes" : "No") << std::endl;
        }
        
        crashFile << "\n=== END CRASH DUMP ===" << std::endl;
        crashFile.close();
        
        std::cerr << "\nCODEFORGE CRASHED!" << std::endl;
        std::cerr << "Signal: " << signal_name << " (" << sig << ")" << std::endl;
        std::cerr << "Crash dump saved to: codeforge_crash_dump.txt" << std::endl;
        std::cerr << "Please report this issue with the crash dump file." << std::endl;
        
    } catch (...) {
        std::cerr << "Error while generating crash dump!" << std::endl;
    }
    
    // Cleanup and exit
    if (Application::getInstance()) {
        Application::getInstance()->shutdown();
    }
    _exit(1);  // Use _exit to avoid atexit handlers
}

// Signal handlers
void crashHandler(int sig) {
    switch(sig) {
        case SIGSEGV:
            generateCrashDump("SIGSEGV (Segmentation fault)", sig);
            break;
        case SIGABRT:
            generateCrashDump("SIGABRT (Abort signal)", sig);
            break;
        case SIGFPE:
            generateCrashDump("SIGFPE (Floating point exception)", sig);
            break;
        case SIGILL:
            generateCrashDump("SIGILL (Illegal instruction)", sig);
            break;
        default:
            generateCrashDump("Unknown signal", sig);
            break;
    }
}

void setupCrashHandlers() {
    signal(SIGSEGV, crashHandler);
    signal(SIGABRT, crashHandler);
    signal(SIGFPE, crashHandler);
    signal(SIGILL, crashHandler);
}

Application::Application() 
    : window(nullptr), running(false) {
    instance = this;
    setupCrashHandlers();
    
    std::cout << "CodeForge v1.0 - Professional C++ IDE" << std::endl;
    std::cout << "Crash reporting enabled" << std::endl;
}

Application::~Application() {
    shutdown();
    instance = nullptr;
}

bool Application::initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwSetErrorCallback(errorCallback);
    
    initializeWindow();
    
    if (!window) {
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1); // Enable vsync
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Note: Docking and Viewports might not be available in this ImGui version
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    initializeManagers();
    
    running = true;
    return true;
}

void Application::initializeWindow() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(1280, 720, "CodeForge - C++ IDE", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return;
    }
}

void Application::initializeManagers() {
    // Initialize managers in dependency order
    settingsManager = std::make_unique<SettingsManager>();
    fileManager = std::make_unique<FileManager>();
    compilerEngine = std::make_unique<CompilerEngine>();
    projectManager = std::make_unique<ProjectManager>();
    buildSystem = std::make_unique<BuildSystem>();
    uiManager = std::make_unique<UIManager>();
}

void Application::toggleFullscreen() {
    if (!window) return;

    if (!fullscreen) {
        // Switch to fullscreen: save current windowed position/size and set monitor
        glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (mode) {
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                fullscreen = true;
            }
        }
    } else {
        // Restore windowed mode
        glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
        fullscreen = false;
    }
}

bool Application::isFullscreen() const {
    return fullscreen;
}

void Application::run() {
    mainLoop();
}

void Application::mainLoop() {
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    try {
        while (running && !glfwWindowShouldClose(window)) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            handleEvents();
            update(deltaTime);
            render();
            
            glfwSwapBuffers(window);
        }
    } catch (const std::exception& e) {
        std::ofstream crashFile("codeforge_crash_dump.txt", std::ios::app);
        
        time_t rawtime;
        struct tm* timeinfo;
        char timestr[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        crashFile << "\n=== CODEFORGE C++ EXCEPTION ===" << std::endl;
        crashFile << "Time: " << timestr << std::endl;
        crashFile << "Exception: " << e.what() << std::endl;
        crashFile << "Location: Main loop" << std::endl;
        crashFile << "=== END EXCEPTION DUMP ===" << std::endl;
        crashFile.close();
        
        std::cerr << "CodeForge caught C++ exception: " << e.what() << std::endl;
        std::cerr << "Exception details saved to codeforge_crash_dump.txt" << std::endl;
        
        shutdown();
        throw; // Re-throw to allow normal exception handling
    } catch (...) {
        std::ofstream crashFile("codeforge_crash_dump.txt", std::ios::app);
        
        time_t rawtime;
        struct tm* timeinfo;
        char timestr[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        crashFile << "\n=== CODEFORGE UNKNOWN EXCEPTION ===" << std::endl;
        crashFile << "Time: " << timestr << std::endl;
        crashFile << "Exception: Unknown C++ exception" << std::endl;
        crashFile << "Location: Main loop" << std::endl;
        crashFile << "=== END EXCEPTION DUMP ===" << std::endl;
        crashFile.close();
        
        std::cerr << "CodeForge caught unknown exception in main loop" << std::endl;
        std::cerr << "Exception details saved to codeforge_crash_dump.txt" << std::endl;
        
        shutdown();
        throw; // Re-throw
    }
}

void Application::handleEvents() {
    glfwPollEvents();
}

void Application::update(float deltaTime) {
    // Update managers
    if (uiManager) {
        uiManager->update(deltaTime);
    }
    if (compilerEngine) {
        // Update compiler status, handle async operations
    }
}

void Application::render() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    try {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render UI
        if (uiManager) {
            uiManager->render();
        }
        
        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Note: Advanced viewport features removed for compatibility
    } catch (const std::exception& e) {
        std::ofstream crashFile("codeforge_crash_dump.txt", std::ios::app);
        
        time_t rawtime;
        struct tm* timeinfo;
        char timestr[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        crashFile << "\n=== CODEFORGE RENDER EXCEPTION ===" << std::endl;
        crashFile << "Time: " << timestr << std::endl;
        crashFile << "Exception: " << e.what() << std::endl;
        crashFile << "Location: Render function" << std::endl;
        crashFile << "=== END RENDER EXCEPTION ===" << std::endl;
        crashFile.close();
        
        std::cerr << "CodeForge render exception: " << e.what() << std::endl;
        std::cerr << "Continuing without UI for this frame..." << std::endl;
        
        // Try to complete the frame anyway
        ImGui::Render();
    }
}

void Application::shutdown() {
    if (window) {
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        // Cleanup managers
        uiManager.reset();
        buildSystem.reset();
        projectManager.reset();
        compilerEngine.reset();
        fileManager.reset();
        settingsManager.reset();
        
        glfwDestroyWindow(window);
        window = nullptr;
    }
    
    glfwTerminate();
    running = false;
}

// Static callbacks
void Application::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;  // Suppress unused parameter warning
    glViewport(0, 0, width, height);
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;  // Suppress unused parameter warning
    (void)mods;      // Suppress unused parameter warning
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        if (Application::getInstance()) {
            Application::getInstance()->toggleFullscreen();
        }
    }
}