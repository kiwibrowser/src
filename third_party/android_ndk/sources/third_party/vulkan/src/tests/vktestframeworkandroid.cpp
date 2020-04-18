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

#include "vktestframeworkandroid.h"
#include "shaderc/shaderc.hpp"
#include <android/log.h>

VkTestFramework::VkTestFramework() {}
VkTestFramework::~VkTestFramework() {}

bool VkTestFramework::m_use_glsl = false;

VkFormat VkTestFramework::GetFormat(VkInstance instance, vk_testing::Device *device) {
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(device->phy().handle(), VK_FORMAT_B8G8R8A8_UNORM, &format_props);
    if (format_props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ||
        format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
        return VK_FORMAT_B8G8R8A8_UNORM;
    }
    vkGetPhysicalDeviceFormatProperties(device->phy().handle(), VK_FORMAT_R8G8B8A8_UNORM, &format_props);
    if (format_props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ||
        format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
    printf("Error - device does not support VK_FORMAT_B8G8R8A8_UNORM nor VK_FORMAT_R8G8B8A8_UNORM - exiting\n");
    exit(0);
}

void VkTestFramework::InitArgs(int *argc, char *argv[]) {}
void VkTestFramework::Finish() {}

void TestEnvironment::SetUp() { vk_testing::set_error_callback(test_error_callback); }

void TestEnvironment::TearDown() {}

// Android specific helper functions for shaderc.
struct shader_type_mapping {
    VkShaderStageFlagBits vkshader_type;
    shaderc_shader_kind shaderc_type;
};

static const shader_type_mapping shader_map_table[] = {
    {VK_SHADER_STAGE_VERTEX_BIT, shaderc_glsl_vertex_shader},
    {VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, shaderc_glsl_tess_control_shader},
    {VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, shaderc_glsl_tess_evaluation_shader},
    {VK_SHADER_STAGE_GEOMETRY_BIT, shaderc_glsl_geometry_shader},
    {VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_glsl_fragment_shader},
    {VK_SHADER_STAGE_COMPUTE_BIT, shaderc_glsl_compute_shader},
};

shaderc_shader_kind MapShadercType(VkShaderStageFlagBits vkShader) {
    for (auto shader : shader_map_table) {
        if (shader.vkshader_type == vkShader) {
            return shader.shaderc_type;
        }
    }
    assert(false);
    return shaderc_glsl_infer_from_source;
}

// Compile a given string containing GLSL into SPIR-V
// Return value of false means an error was encountered
bool VkTestFramework::GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv) {

    // On Android, use shaderc instead.
    shaderc::Compiler compiler;
    shaderc::SpvCompilationResult result =
        compiler.CompileGlslToSpv(pshader, strlen(pshader), MapShadercType(shader_type), "shader");
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        __android_log_print(ANDROID_LOG_ERROR, "VkLayerValidationTest", "GLSLtoSPV compilation failed");
        return false;
    }

    for (auto iter = result.begin(); iter != result.end(); iter++) {
        spirv.push_back(*iter);
    }

    return true;
}
