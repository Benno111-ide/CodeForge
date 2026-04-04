#include "SettingsManager.h"
#include <filesystem>

SettingsManager::SettingsManager() {
    settingsFile = (std::filesystem::current_path() / "settings.json").string();
    loadSettings();
}

SettingsManager::~SettingsManager() {
    saveSettings();
}

void SettingsManager::loadSettings() {
    // Stub implementation - would load from JSON file
}

void SettingsManager::saveSettings() {
    // Stub implementation - would save to JSON file
}
