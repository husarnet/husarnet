#pragma once
#include "husarnet.h"
#include <unordered_map>

struct Settings {
    explicit Settings(std::string path);

    std::unordered_map<std::string, std::string> environValues;
    std::unordered_map<std::string, std::string> settingsFileValues;
    std::string jsonPath;

    void writeSettingsFile();
    void set(std::string key, std::string value);
    void loadInto(NgSocketOptions* settings);
};
