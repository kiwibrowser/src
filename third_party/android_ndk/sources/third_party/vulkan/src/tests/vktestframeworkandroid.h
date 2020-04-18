//  VK tests
//
//  Copyright (c) 2015-2016 The Khronos Group Inc.
//  Copyright (c) 2015-2016 Valve Corporation
//  Copyright (c) 2015-2016 LunarG, Inc.
//  Copyright (c) 2015-2016 Google, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VKTESTFRAMEWORKANDROID_H
#define VKTESTFRAMEWORKANDROID_H

#include "test_common.h"
#include "vktestbinding.h"

#if defined(NDEBUG)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

// Can be used by tests to record additional details / description of test
#define TEST_DESCRIPTION(desc) RecordProperty("description", desc)

#define ICD_SPV_MAGIC 0x07230203

class VkTestFramework : public ::testing::Test {
  public:
    VkTestFramework();
    ~VkTestFramework();

    static void InitArgs(int *argc, char *argv[]);
    static void Finish();

    VkFormat GetFormat(VkInstance instance, vk_testing::Device *device);
    bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spv);

    static bool m_use_glsl;
};

class TestEnvironment : public ::testing::Environment {
  public:
    void SetUp();

    void TearDown();
};

#endif
