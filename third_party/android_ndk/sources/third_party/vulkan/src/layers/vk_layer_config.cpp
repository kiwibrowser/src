/**************************************************************************
 *
 * Copyright 2014 Valve Software
 * Copyright 2015 Google Inc.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 **************************************************************************/
#include "vk_layer_config.h"
#include "vulkan/vk_sdk_platform.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <vulkan/vk_layer.h>

#define MAX_CHARS_PER_LINE 4096

class ConfigFile {
  public:
    ConfigFile();
    ~ConfigFile();

    const char *getOption(const std::string &_option);
    void setOption(const std::string &_option, const std::string &_val);

  private:
    bool m_fileIsParsed;
    std::map<std::string, std::string> m_valueMap;

    void parseFile(const char *filename);
};

static ConfigFile g_configFileObj;

std::string getEnvironment(const char *variable) {
#if !defined(__ANDROID__) && !defined(_WIN32)
    const char *output = getenv(variable);
    return output == NULL ? "" : output;
#elif defined(_WIN32)
    int size = GetEnvironmentVariable(variable, NULL, 0);
    if (size == 0) {
        return "";
    }
    char *buffer = new char[size];
    GetEnvironmentVariable(variable, buffer, size);
    std::string output = buffer;
    delete[] buffer;
    return output;
#else
    return "";
#endif
}

const char *getLayerOption(const char *_option) { return g_configFileObj.getOption(_option); }

// If option is NULL or stdout, return stdout, otherwise try to open option
// as a filename. If successful, return file handle, otherwise stdout
FILE *getLayerLogOutput(const char *_option, const char *layerName) {
    FILE *log_output = NULL;
    if (!_option || !strcmp("stdout", _option))
        log_output = stdout;
    else {
        log_output = fopen(_option, "w");
        if (log_output == NULL) {
            if (_option)
                std::cout << std::endl
                          << layerName << " ERROR: Bad output filename specified: " << _option << ". Writing to STDOUT instead"
                          << std::endl
                          << std::endl;
            log_output = stdout;
        }
    }
    return log_output;
}

// Map option strings to flag enum values
VkFlags GetLayerOptionFlags(std::string _option, std::unordered_map<std::string, VkFlags> const &enum_data,
                            uint32_t option_default) {
    VkDebugReportFlagsEXT flags = option_default;
    std::string option_list = g_configFileObj.getOption(_option.c_str());

    while (option_list.length() != 0) {

        // Find length of option string
        std::size_t option_length = option_list.find(",");
        if (option_length == option_list.npos) {
            option_length = option_list.size();
        }

        // Get first option in list
        const std::string option = option_list.substr(0, option_length);

        auto enum_value = enum_data.find(option);
        if (enum_value != enum_data.end()) {
            flags |= enum_value->second;
        }

        // Remove first option from option_list
        option_list.erase(0, option_length);
        // Remove possible comma separator
        std::size_t char_position = option_list.find(",");
        if (char_position == 0) {
            option_list.erase(char_position, 1);
        }
        // Remove possible space
        char_position = option_list.find(" ");
        if (char_position == 0) {
            option_list.erase(char_position, 1);
        }
    }
    return flags;
}

void setLayerOption(const char *_option, const char *_val) { g_configFileObj.setOption(_option, _val); }

// Constructor for ConfigFile. Initialize layers to log error messages to stdout by default. If a vk_layer_settings file is present,
// its settings will override the defaults.
ConfigFile::ConfigFile() : m_fileIsParsed(false) {
    m_valueMap["lunarg_core_validation.report_flags"] = "error";
    m_valueMap["lunarg_image.report_flags"] = "error";
    m_valueMap["lunarg_object_tracker.report_flags"] = "error";
    m_valueMap["lunarg_parameter_validation.report_flags"] = "error";
    m_valueMap["lunarg_swapchain.report_flags"] = "error";
    m_valueMap["google_threading.report_flags"] = "error";
    m_valueMap["google_unique_objects.report_flags"] = "error";

#ifdef WIN32
    // For Windows, enable message logging AND OutputDebugString
    m_valueMap["lunarg_core_validation.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG,VK_DBG_LAYER_ACTION_DEBUG_OUTPUT";
    m_valueMap["lunarg_image.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG,VK_DBG_LAYER_ACTION_DEBUG_OUTPUT";
    m_valueMap["lunarg_object_tracker.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG,VK_DBG_LAYER_ACTION_DEBUG_OUTPUT";
    m_valueMap["lunarg_parameter_validation.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG,VK_DBG_LAYER_ACTION_DEBUG_OUTPUT";
    m_valueMap["lunarg_swapchain.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG,VK_DBG_LAYER_ACTION_DEBUG_OUTPUT";
    m_valueMap["google_threading.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG,VK_DBG_LAYER_ACTION_DEBUG_OUTPUT";
    m_valueMap["google_unique_objects.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG,VK_DBG_LAYER_ACTION_DEBUG_OUTPUT";
#else  // WIN32
    m_valueMap["lunarg_core_validation.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG";
    m_valueMap["lunarg_image.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG";
    m_valueMap["lunarg_object_tracker.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG";
    m_valueMap["lunarg_parameter_validation.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG";
    m_valueMap["lunarg_swapchain.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG";
    m_valueMap["google_threading.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG";
    m_valueMap["google_unique_objects.debug_action"] = "VK_DBG_LAYER_ACTION_DEFAULT,VK_DBG_LAYER_ACTION_LOG_MSG";
#endif // WIN32

    m_valueMap["lunarg_core_validation.log_filename"] = "stdout";
    m_valueMap["lunarg_image.log_filename"] = "stdout";
    m_valueMap["lunarg_object_tracker.log_filename"] = "stdout";
    m_valueMap["lunarg_parameter_validation.log_filename"] = "stdout";
    m_valueMap["lunarg_swapchain.log_filename"] = "stdout";
    m_valueMap["google_threading.log_filename"] = "stdout";
    m_valueMap["google_unique_objects.log_filename"] = "stdout";
}

ConfigFile::~ConfigFile() {}

const char *ConfigFile::getOption(const std::string &_option) {
    std::map<std::string, std::string>::const_iterator it;
    if (!m_fileIsParsed) {
        std::string envPath = getEnvironment("VK_LAYER_SETTINGS_PATH");

        // If the path exists use it, else use vk_layer_settings
        struct stat info;
        if (stat(envPath.c_str(), &info) == 0) {
            // If this is a directory, look for vk_layer_settings within the directory
            if (info.st_mode & S_IFDIR) {
                envPath += "/vk_layer_settings.txt";
            }
            parseFile(envPath.c_str());
        } else {
            parseFile("vk_layer_settings.txt");
        }
    }

    if ((it = m_valueMap.find(_option)) == m_valueMap.end())
        return "";
    else
        return it->second.c_str();
}

void ConfigFile::setOption(const std::string &_option, const std::string &_val) {
    if (!m_fileIsParsed) {
        std::string envPath = getEnvironment("VK_LAYER_SETTINGS_PATH");

        // If the path exists use it, else use vk_layer_settings
        struct stat info;
        if (stat(envPath.c_str(), &info) == 0) {
            // If this is a directory, look for vk_layer_settings within the directory
            if (info.st_mode & S_IFDIR) {
                envPath += "/vk_layer_settings.txt";
            }
            parseFile(envPath.c_str());
        } else {
            parseFile("vk_layer_settings.txt");
        }
    }

    m_valueMap[_option] = _val;
}

void ConfigFile::parseFile(const char *filename) {
    std::ifstream file;
    char buf[MAX_CHARS_PER_LINE];

    m_fileIsParsed = true;

    file.open(filename);
    if (!file.good()) {
        return;
    }


    // read tokens from the file and form option, value pairs
    file.getline(buf, MAX_CHARS_PER_LINE);
    while (!file.eof()) {
        char option[512];
        char value[512];

        char *pComment;

        // discard any comments delimited by '#' in the line
        pComment = strchr(buf, '#');
        if (pComment)
            *pComment = '\0';

        if (sscanf(buf, " %511[^\n\t =] = %511[^\n \t]", option, value) == 2) {
            std::string optStr(option);
            std::string valStr(value);
            m_valueMap[optStr] = valStr;
        }
        file.getline(buf, MAX_CHARS_PER_LINE);
    }
}

void print_msg_flags(VkFlags msgFlags, char *msg_flags) {
    bool separator = false;

    msg_flags[0] = 0;
    if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        strcat(msg_flags, "DEBUG");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        if (separator)
            strcat(msg_flags, ",");
        strcat(msg_flags, "INFO");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        if (separator)
            strcat(msg_flags, ",");
        strcat(msg_flags, "WARN");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        if (separator)
            strcat(msg_flags, ",");
        strcat(msg_flags, "PERF");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        if (separator)
            strcat(msg_flags, ",");
        strcat(msg_flags, "ERROR");
    }
}
