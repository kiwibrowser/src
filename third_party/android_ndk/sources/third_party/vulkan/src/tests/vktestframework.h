/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

#ifndef VKTESTFRAMEWORK_H
#define VKTESTFRAMEWORK_H

//#include "gtest-1.7.0/include/gtest/gtest.h"
#include "SPIRV/GLSL.std.450.h"
#include "glslang/Public/ShaderLang.h"
#include "icd-spv.h"
#include "test_common.h"
#include "test_environment.h"
#include "vktestbinding.h"

#include <fstream>
#include <iostream>
#include <list>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

// Can be used by tests to record additional details / description of test
#define TEST_DESCRIPTION(desc) RecordProperty("description", desc)

using namespace std;

class VkImageObj;

class VkTestFramework : public ::testing::Test {
  public:
    VkTestFramework();
    ~VkTestFramework();

    VkFormat GetFormat(VkInstance instance, vk_testing::Device *device);
    static bool optionMatch(const char *option, char *optionLine);
    static void InitArgs(int *argc, char *argv[]);
    static void Finish();

    bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spv);
    static bool m_use_glsl;
    static bool m_canonicalize_spv;
    static bool m_strip_spv;
    static bool m_do_everything_spv;

    char **ReadFileData(const char *fileName);
    void FreeFileData(char **data);

  private:
    int m_compile_options;
    int m_num_shader_strings;
    TBuiltInResource Resources;
    void SetMessageOptions(EShMessages &messages);
    void ProcessConfigFile();
    EShLanguage FindLanguage(const std::string &name);
    EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
    std::string ConfigFile;
    bool SetConfigFile(const std::string &name);
    static int m_width;
    static int m_height;
    string m_testName;
};

class TestEnvironment : public ::testing::Environment {
  public:
    void SetUp();

    void TearDown();
};

#endif // VKTESTFRAMEWORK_H
