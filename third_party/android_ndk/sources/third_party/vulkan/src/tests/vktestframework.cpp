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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

#include "vktestframework.h"
#include "vkrenderframework.h"
// TODO FIXME remove this once glslang doesn't define this
#undef BadValue
#include "SPIRV/GlslangToSpv.h"
#include "SPIRV/SPVRemapper.h"
#include <limits.h>
#include <math.h>

#if defined(PATH_MAX) && !defined(MAX_PATH)
#define MAX_PATH PATH_MAX
#endif

#ifdef _WIN32
#define ERR_EXIT(err_msg, err_class)                                                                                               \
    do {                                                                                                                           \
        MessageBox(NULL, err_msg, err_class, MB_OK);                                                                               \
        exit(1);                                                                                                                   \
    } while (0)
#else // _WIN32

#define ERR_EXIT(err_msg, err_class)                                                                                               \
    do {                                                                                                                           \
        printf(err_msg);                                                                                                           \
        fflush(stdout);                                                                                                            \
        exit(1);                                                                                                                   \
    } while (0)
#endif // _WIN32

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                                                                                   \
    {                                                                                                                              \
        m_fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);                                      \
        if (m_fp##entrypoint == NULL) {                                                                                            \
            ERR_EXIT("vkGetInstanceProcAddr failed to find vk" #entrypoint, "vkGetInstanceProcAddr Failure");                      \
        }                                                                                                                          \
    }

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                                                                      \
    {                                                                                                                              \
        m_fp##entrypoint = (PFN_vk##entrypoint)vkGetDeviceProcAddr(dev, "vk" #entrypoint);                                         \
        if (m_fp##entrypoint == NULL) {                                                                                            \
            ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint, "vkGetDeviceProcAddr Failure");                          \
        }                                                                                                                          \
    }

// Command-line options
enum TOptions {
    EOptionNone = 0x000,
    EOptionIntermediate = 0x001,
    EOptionSuppressInfolog = 0x002,
    EOptionMemoryLeakMode = 0x004,
    EOptionRelaxedErrors = 0x008,
    EOptionGiveWarnings = 0x010,
    EOptionLinkProgram = 0x020,
    EOptionMultiThreaded = 0x040,
    EOptionDumpConfig = 0x080,
    EOptionDumpReflection = 0x100,
    EOptionSuppressWarnings = 0x200,
    EOptionDumpVersions = 0x400,
    EOptionSpv = 0x800,
    EOptionDefaultDesktop = 0x1000,
};

struct SwapchainBuffers {
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
};

#ifndef _WIN32

#include <errno.h>

int fopen_s(FILE **pFile, const char *filename, const char *mode) {
    if (!pFile || !filename || !mode) {
        return EINVAL;
    }

    FILE *f = fopen(filename, mode);
    if (!f) {
        if (errno != 0) {
            return errno;
        } else {
            return ENOENT;
        }
    }
    *pFile = f;

    return 0;
}

#endif

// Set up environment for GLSL compiler
// Must be done once per process
void TestEnvironment::SetUp() {
    // Initialize GLSL to SPV compiler utility
    glslang::InitializeProcess();

    vk_testing::set_error_callback(test_error_callback);
}

void TestEnvironment::TearDown() { glslang::FinalizeProcess(); }

VkTestFramework::VkTestFramework() : m_compile_options(0), m_num_shader_strings(0) {}

VkTestFramework::~VkTestFramework() {}

// Define all the static elements
bool VkTestFramework::m_use_glsl = false;
bool VkTestFramework::m_canonicalize_spv = false;
bool VkTestFramework::m_strip_spv = false;
bool VkTestFramework::m_do_everything_spv = false;
int VkTestFramework::m_width = 0;
int VkTestFramework::m_height = 0;

bool VkTestFramework::optionMatch(const char *option, char *optionLine) {
    if (strncmp(option, optionLine, strlen(option)) == 0)
        return true;
    else
        return false;
}

void VkTestFramework::InitArgs(int *argc, char *argv[]) {
    int i, n;

    for (i = 1, n = 1; i < *argc; i++) {
        if (optionMatch("--no-SPV", argv[i]))
            m_use_glsl = true;
        else if (optionMatch("--strip-SPV", argv[i]))
            m_strip_spv = true;
        else if (optionMatch("--canonicalize-SPV", argv[i]))
            m_canonicalize_spv = true;
        else if (optionMatch("--help", argv[i]) || optionMatch("-h", argv[i])) {
            printf("\nOther options:\n");
            printf("\t--show-images\n"
                   "\t\tDisplay test images in viewer after tests complete.\n");
            printf("\t--save-images\n"
                   "\t\tSave tests images as ppm files in current working "
                   "directory.\n"
                   "\t\tUsed to generate golden images for compare-images.\n");
            printf("\t--compare-images\n"
                   "\t\tCompare test images to 'golden' image in golden folder.\n"
                   "\t\tAlso saves the generated test image in current working\n"
                   "\t\t\tdirectory but only if the image is different from the "
                   "golden\n"
                   "\t\tSetting RENDERTEST_GOLDEN_DIR environment variable can "
                   "specify\n"
                   "\t\t\tdifferent directory for golden images\n"
                   "\t\tSignal test failure if different.\n");
            printf("\t--no-SPV\n"
                   "\t\tUse built-in GLSL compiler rather than SPV code path.\n");
            printf("\t--strip-SPV\n"
                   "\t\tStrip SPIR-V debug information (line numbers, names, "
                   "etc).\n");
            printf("\t--canonicalize-SPV\n"
                   "\t\tRemap SPIR-V ids before submission to aid compression.\n");
            exit(0);
        } else {
            printf("\nUnrecognized option: %s\n", argv[i]);
            printf("\nUse --help or -h for option list.\n");
            exit(0);
        }

        /*
         * Since the above "consume" inputs, update argv
         * so that it contains the trimmed list of args for glutInit
         */

        argv[n] = argv[i];
        n++;
    }
}

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
    printf("Error - device does not support VK_FORMAT_B8G8R8A8_UNORM nor "
           "VK_FORMAT_R8G8B8A8_UNORM - exiting\n");
    exit(1);
}

void VkTestFramework::Finish() {}

//
// These are the default resources for TBuiltInResources, used for both
//  - parsing this string for the case where the user didn't supply one
//  - dumping out a template for user construction of a config file
//
static const char *DefaultConfig = "MaxLights 32\n"
                                   "MaxClipPlanes 6\n"
                                   "MaxTextureUnits 32\n"
                                   "MaxTextureCoords 32\n"
                                   "MaxVertexAttribs 64\n"
                                   "MaxVertexUniformComponents 4096\n"
                                   "MaxVaryingFloats 64\n"
                                   "MaxVertexTextureImageUnits 32\n"
                                   "MaxCombinedTextureImageUnits 80\n"
                                   "MaxTextureImageUnits 32\n"
                                   "MaxFragmentUniformComponents 4096\n"
                                   "MaxDrawBuffers 32\n"
                                   "MaxVertexUniformVectors 128\n"
                                   "MaxVaryingVectors 8\n"
                                   "MaxFragmentUniformVectors 16\n"
                                   "MaxVertexOutputVectors 16\n"
                                   "MaxFragmentInputVectors 15\n"
                                   "MinProgramTexelOffset -8\n"
                                   "MaxProgramTexelOffset 7\n"
                                   "MaxClipDistances 8\n"
                                   "MaxComputeWorkGroupCountX 65535\n"
                                   "MaxComputeWorkGroupCountY 65535\n"
                                   "MaxComputeWorkGroupCountZ 65535\n"
                                   "MaxComputeWorkGroupSizeX 1024\n"
                                   "MaxComputeWorkGroupSizeY 1024\n"
                                   "MaxComputeWorkGroupSizeZ 64\n"
                                   "MaxComputeUniformComponents 1024\n"
                                   "MaxComputeTextureImageUnits 16\n"
                                   "MaxComputeImageUniforms 8\n"
                                   "MaxComputeAtomicCounters 8\n"
                                   "MaxComputeAtomicCounterBuffers 1\n"
                                   "MaxVaryingComponents 60\n"
                                   "MaxVertexOutputComponents 64\n"
                                   "MaxGeometryInputComponents 64\n"
                                   "MaxGeometryOutputComponents 128\n"
                                   "MaxFragmentInputComponents 128\n"
                                   "MaxImageUnits 8\n"
                                   "MaxCombinedImageUnitsAndFragmentOutputs 8\n"
                                   "MaxCombinedShaderOutputResources 8\n"
                                   "MaxImageSamples 0\n"
                                   "MaxVertexImageUniforms 0\n"
                                   "MaxTessControlImageUniforms 0\n"
                                   "MaxTessEvaluationImageUniforms 0\n"
                                   "MaxGeometryImageUniforms 0\n"
                                   "MaxFragmentImageUniforms 8\n"
                                   "MaxCombinedImageUniforms 8\n"
                                   "MaxGeometryTextureImageUnits 16\n"
                                   "MaxGeometryOutputVertices 256\n"
                                   "MaxGeometryTotalOutputComponents 1024\n"
                                   "MaxGeometryUniformComponents 1024\n"
                                   "MaxGeometryVaryingComponents 64\n"
                                   "MaxTessControlInputComponents 128\n"
                                   "MaxTessControlOutputComponents 128\n"
                                   "MaxTessControlTextureImageUnits 16\n"
                                   "MaxTessControlUniformComponents 1024\n"
                                   "MaxTessControlTotalOutputComponents 4096\n"
                                   "MaxTessEvaluationInputComponents 128\n"
                                   "MaxTessEvaluationOutputComponents 128\n"
                                   "MaxTessEvaluationTextureImageUnits 16\n"
                                   "MaxTessEvaluationUniformComponents 1024\n"
                                   "MaxTessPatchComponents 120\n"
                                   "MaxPatchVertices 32\n"
                                   "MaxTessGenLevel 64\n"
                                   "MaxViewports 16\n"
                                   "MaxVertexAtomicCounters 0\n"
                                   "MaxTessControlAtomicCounters 0\n"
                                   "MaxTessEvaluationAtomicCounters 0\n"
                                   "MaxGeometryAtomicCounters 0\n"
                                   "MaxFragmentAtomicCounters 8\n"
                                   "MaxCombinedAtomicCounters 8\n"
                                   "MaxAtomicCounterBindings 1\n"
                                   "MaxVertexAtomicCounterBuffers 0\n"
                                   "MaxTessControlAtomicCounterBuffers 0\n"
                                   "MaxTessEvaluationAtomicCounterBuffers 0\n"
                                   "MaxGeometryAtomicCounterBuffers 0\n"
                                   "MaxFragmentAtomicCounterBuffers 1\n"
                                   "MaxCombinedAtomicCounterBuffers 1\n"
                                   "MaxAtomicCounterBufferSize 16384\n"
                                   "MaxTransformFeedbackBuffers 4\n"
                                   "MaxTransformFeedbackInterleavedComponents 64\n"
                                   "MaxCullDistances 8\n"
                                   "MaxCombinedClipAndCullDistances 8\n"
                                   "MaxSamples 4\n"

                                   "nonInductiveForLoops 1\n"
                                   "whileLoops 1\n"
                                   "doWhileLoops 1\n"
                                   "generalUniformIndexing 1\n"
                                   "generalAttributeMatrixVectorIndexing 1\n"
                                   "generalVaryingIndexing 1\n"
                                   "generalSamplerIndexing 1\n"
                                   "generalVariableIndexing 1\n"
                                   "generalConstantMatrixVectorIndexing 1\n";

//
// *.conf => this is a config file that can set limits/resources
//
bool VkTestFramework::SetConfigFile(const std::string &name) {
    if (name.size() < 5)
        return false;

    if (name.compare(name.size() - 5, 5, ".conf") == 0) {
        ConfigFile = name;
        return true;
    }

    return false;
}

//
// Parse either a .conf file provided by the user or the default string above.
//
void VkTestFramework::ProcessConfigFile() {
    char **configStrings = 0;
    char *config = 0;
    if (ConfigFile.size() > 0) {
        configStrings = ReadFileData(ConfigFile.c_str());
        if (configStrings)
            config = *configStrings;
        else {
            printf("Error opening configuration file; will instead use the "
                   "default configuration\n");
        }
    }

    if (config == 0) {
        config = (char *)alloca(strlen(DefaultConfig) + 1);
        strcpy(config, DefaultConfig);
    }

    const char *delims = " \t\n\r";
    const char *token = strtok(config, delims);
    while (token) {
        const char *valueStr = strtok(0, delims);
        if (valueStr == 0 || !(valueStr[0] == '-' || (valueStr[0] >= '0' && valueStr[0] <= '9'))) {
            printf("Error: '%s' bad .conf file.  Each name must be followed by "
                   "one number.\n",
                   valueStr ? valueStr : "");
            return;
        }
        int value = atoi(valueStr);

        if (strcmp(token, "MaxLights") == 0)
            Resources.maxLights = value;
        else if (strcmp(token, "MaxClipPlanes") == 0)
            Resources.maxClipPlanes = value;
        else if (strcmp(token, "MaxTextureUnits") == 0)
            Resources.maxTextureUnits = value;
        else if (strcmp(token, "MaxTextureCoords") == 0)
            Resources.maxTextureCoords = value;
        else if (strcmp(token, "MaxVertexAttribs") == 0)
            Resources.maxVertexAttribs = value;
        else if (strcmp(token, "MaxVertexUniformComponents") == 0)
            Resources.maxVertexUniformComponents = value;
        else if (strcmp(token, "MaxVaryingFloats") == 0)
            Resources.maxVaryingFloats = value;
        else if (strcmp(token, "MaxVertexTextureImageUnits") == 0)
            Resources.maxVertexTextureImageUnits = value;
        else if (strcmp(token, "MaxCombinedTextureImageUnits") == 0)
            Resources.maxCombinedTextureImageUnits = value;
        else if (strcmp(token, "MaxTextureImageUnits") == 0)
            Resources.maxTextureImageUnits = value;
        else if (strcmp(token, "MaxFragmentUniformComponents") == 0)
            Resources.maxFragmentUniformComponents = value;
        else if (strcmp(token, "MaxDrawBuffers") == 0)
            Resources.maxDrawBuffers = value;
        else if (strcmp(token, "MaxVertexUniformVectors") == 0)
            Resources.maxVertexUniformVectors = value;
        else if (strcmp(token, "MaxVaryingVectors") == 0)
            Resources.maxVaryingVectors = value;
        else if (strcmp(token, "MaxFragmentUniformVectors") == 0)
            Resources.maxFragmentUniformVectors = value;
        else if (strcmp(token, "MaxVertexOutputVectors") == 0)
            Resources.maxVertexOutputVectors = value;
        else if (strcmp(token, "MaxFragmentInputVectors") == 0)
            Resources.maxFragmentInputVectors = value;
        else if (strcmp(token, "MinProgramTexelOffset") == 0)
            Resources.minProgramTexelOffset = value;
        else if (strcmp(token, "MaxProgramTexelOffset") == 0)
            Resources.maxProgramTexelOffset = value;
        else if (strcmp(token, "MaxClipDistances") == 0)
            Resources.maxClipDistances = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountX") == 0)
            Resources.maxComputeWorkGroupCountX = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountY") == 0)
            Resources.maxComputeWorkGroupCountY = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountZ") == 0)
            Resources.maxComputeWorkGroupCountZ = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeX") == 0)
            Resources.maxComputeWorkGroupSizeX = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeY") == 0)
            Resources.maxComputeWorkGroupSizeY = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeZ") == 0)
            Resources.maxComputeWorkGroupSizeZ = value;
        else if (strcmp(token, "MaxComputeUniformComponents") == 0)
            Resources.maxComputeUniformComponents = value;
        else if (strcmp(token, "MaxComputeTextureImageUnits") == 0)
            Resources.maxComputeTextureImageUnits = value;
        else if (strcmp(token, "MaxComputeImageUniforms") == 0)
            Resources.maxComputeImageUniforms = value;
        else if (strcmp(token, "MaxComputeAtomicCounters") == 0)
            Resources.maxComputeAtomicCounters = value;
        else if (strcmp(token, "MaxComputeAtomicCounterBuffers") == 0)
            Resources.maxComputeAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxVaryingComponents") == 0)
            Resources.maxVaryingComponents = value;
        else if (strcmp(token, "MaxVertexOutputComponents") == 0)
            Resources.maxVertexOutputComponents = value;
        else if (strcmp(token, "MaxGeometryInputComponents") == 0)
            Resources.maxGeometryInputComponents = value;
        else if (strcmp(token, "MaxGeometryOutputComponents") == 0)
            Resources.maxGeometryOutputComponents = value;
        else if (strcmp(token, "MaxFragmentInputComponents") == 0)
            Resources.maxFragmentInputComponents = value;
        else if (strcmp(token, "MaxImageUnits") == 0)
            Resources.maxImageUnits = value;
        else if (strcmp(token, "MaxCombinedImageUnitsAndFragmentOutputs") == 0)
            Resources.maxCombinedImageUnitsAndFragmentOutputs = value;
        else if (strcmp(token, "MaxCombinedShaderOutputResources") == 0)
            Resources.maxCombinedShaderOutputResources = value;
        else if (strcmp(token, "MaxImageSamples") == 0)
            Resources.maxImageSamples = value;
        else if (strcmp(token, "MaxVertexImageUniforms") == 0)
            Resources.maxVertexImageUniforms = value;
        else if (strcmp(token, "MaxTessControlImageUniforms") == 0)
            Resources.maxTessControlImageUniforms = value;
        else if (strcmp(token, "MaxTessEvaluationImageUniforms") == 0)
            Resources.maxTessEvaluationImageUniforms = value;
        else if (strcmp(token, "MaxGeometryImageUniforms") == 0)
            Resources.maxGeometryImageUniforms = value;
        else if (strcmp(token, "MaxFragmentImageUniforms") == 0)
            Resources.maxFragmentImageUniforms = value;
        else if (strcmp(token, "MaxCombinedImageUniforms") == 0)
            Resources.maxCombinedImageUniforms = value;
        else if (strcmp(token, "MaxGeometryTextureImageUnits") == 0)
            Resources.maxGeometryTextureImageUnits = value;
        else if (strcmp(token, "MaxGeometryOutputVertices") == 0)
            Resources.maxGeometryOutputVertices = value;
        else if (strcmp(token, "MaxGeometryTotalOutputComponents") == 0)
            Resources.maxGeometryTotalOutputComponents = value;
        else if (strcmp(token, "MaxGeometryUniformComponents") == 0)
            Resources.maxGeometryUniformComponents = value;
        else if (strcmp(token, "MaxGeometryVaryingComponents") == 0)
            Resources.maxGeometryVaryingComponents = value;
        else if (strcmp(token, "MaxTessControlInputComponents") == 0)
            Resources.maxTessControlInputComponents = value;
        else if (strcmp(token, "MaxTessControlOutputComponents") == 0)
            Resources.maxTessControlOutputComponents = value;
        else if (strcmp(token, "MaxTessControlTextureImageUnits") == 0)
            Resources.maxTessControlTextureImageUnits = value;
        else if (strcmp(token, "MaxTessControlUniformComponents") == 0)
            Resources.maxTessControlUniformComponents = value;
        else if (strcmp(token, "MaxTessControlTotalOutputComponents") == 0)
            Resources.maxTessControlTotalOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationInputComponents") == 0)
            Resources.maxTessEvaluationInputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationOutputComponents") == 0)
            Resources.maxTessEvaluationOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationTextureImageUnits") == 0)
            Resources.maxTessEvaluationTextureImageUnits = value;
        else if (strcmp(token, "MaxTessEvaluationUniformComponents") == 0)
            Resources.maxTessEvaluationUniformComponents = value;
        else if (strcmp(token, "MaxTessPatchComponents") == 0)
            Resources.maxTessPatchComponents = value;
        else if (strcmp(token, "MaxPatchVertices") == 0)
            Resources.maxPatchVertices = value;
        else if (strcmp(token, "MaxTessGenLevel") == 0)
            Resources.maxTessGenLevel = value;
        else if (strcmp(token, "MaxViewports") == 0)
            Resources.maxViewports = value;
        else if (strcmp(token, "MaxVertexAtomicCounters") == 0)
            Resources.maxVertexAtomicCounters = value;
        else if (strcmp(token, "MaxTessControlAtomicCounters") == 0)
            Resources.maxTessControlAtomicCounters = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounters") == 0)
            Resources.maxTessEvaluationAtomicCounters = value;
        else if (strcmp(token, "MaxGeometryAtomicCounters") == 0)
            Resources.maxGeometryAtomicCounters = value;
        else if (strcmp(token, "MaxFragmentAtomicCounters") == 0)
            Resources.maxFragmentAtomicCounters = value;
        else if (strcmp(token, "MaxCombinedAtomicCounters") == 0)
            Resources.maxCombinedAtomicCounters = value;
        else if (strcmp(token, "MaxAtomicCounterBindings") == 0)
            Resources.maxAtomicCounterBindings = value;
        else if (strcmp(token, "MaxVertexAtomicCounterBuffers") == 0)
            Resources.maxVertexAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessControlAtomicCounterBuffers") == 0)
            Resources.maxTessControlAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounterBuffers") == 0)
            Resources.maxTessEvaluationAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxGeometryAtomicCounterBuffers") == 0)
            Resources.maxGeometryAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxFragmentAtomicCounterBuffers") == 0)
            Resources.maxFragmentAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxCombinedAtomicCounterBuffers") == 0)
            Resources.maxCombinedAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxAtomicCounterBufferSize") == 0)
            Resources.maxAtomicCounterBufferSize = value;
        else if (strcmp(token, "MaxTransformFeedbackBuffers") == 0)
            Resources.maxTransformFeedbackBuffers = value;
        else if (strcmp(token, "MaxTransformFeedbackInterleavedComponents") == 0)
            Resources.maxTransformFeedbackInterleavedComponents = value;
        else if (strcmp(token, "MaxCullDistances") == 0)
            Resources.maxCullDistances = value;
        else if (strcmp(token, "MaxCombinedClipAndCullDistances") == 0)
            Resources.maxCombinedClipAndCullDistances = value;
        else if (strcmp(token, "MaxSamples") == 0)
            Resources.maxSamples = value;

        else if (strcmp(token, "nonInductiveForLoops") == 0)
            Resources.limits.nonInductiveForLoops = (value != 0);
        else if (strcmp(token, "whileLoops") == 0)
            Resources.limits.whileLoops = (value != 0);
        else if (strcmp(token, "doWhileLoops") == 0)
            Resources.limits.doWhileLoops = (value != 0);
        else if (strcmp(token, "generalUniformIndexing") == 0)
            Resources.limits.generalUniformIndexing = (value != 0);
        else if (strcmp(token, "generalAttributeMatrixVectorIndexing") == 0)
            Resources.limits.generalAttributeMatrixVectorIndexing = (value != 0);
        else if (strcmp(token, "generalVaryingIndexing") == 0)
            Resources.limits.generalVaryingIndexing = (value != 0);
        else if (strcmp(token, "generalSamplerIndexing") == 0)
            Resources.limits.generalSamplerIndexing = (value != 0);
        else if (strcmp(token, "generalVariableIndexing") == 0)
            Resources.limits.generalVariableIndexing = (value != 0);
        else if (strcmp(token, "generalConstantMatrixVectorIndexing") == 0)
            Resources.limits.generalConstantMatrixVectorIndexing = (value != 0);
        else
            printf("Warning: unrecognized limit (%s) in configuration file.\n", token);

        token = strtok(0, delims);
    }
    if (configStrings)
        FreeFileData(configStrings);
}

void VkTestFramework::SetMessageOptions(EShMessages &messages) {
    if (m_compile_options & EOptionRelaxedErrors)
        messages = (EShMessages)(messages | EShMsgRelaxedErrors);
    if (m_compile_options & EOptionIntermediate)
        messages = (EShMessages)(messages | EShMsgAST);
    if (m_compile_options & EOptionSuppressWarnings)
        messages = (EShMessages)(messages | EShMsgSuppressWarnings);
}

//
//   Malloc a string of sufficient size and read a string into it.
//
char **VkTestFramework::ReadFileData(const char *fileName) {
    FILE *in;
#if defined(_WIN32) && defined(__GNUC__)
    in = fopen(fileName, "r");
    int errorCode = in ? 0 : 1;
#else
    int errorCode = fopen_s(&in, fileName, "r");
#endif

    char *fdata;
    size_t count = 0;
    const int maxSourceStrings = 5;
    char **return_data = (char **)malloc(sizeof(char *) * (maxSourceStrings + 1));

    if (errorCode) {
        printf("Error: unable to open input file: %s\n", fileName);
        return 0;
    }

    while (fgetc(in) != EOF)
        count++;

    fseek(in, 0, SEEK_SET);

    if (!(fdata = (char *)malloc(count + 2))) {
        printf("Error allocating memory\n");
        return 0;
    }
    if (fread(fdata, 1, count, in) != count) {
        printf("Error reading input file: %s\n", fileName);
        return 0;
    }
    fdata[count] = '\0';
    fclose(in);
    if (count == 0) {
        return_data[0] = (char *)malloc(count + 2);
        return_data[0][0] = '\0';
        m_num_shader_strings = 0;
        return return_data;
    } else
        m_num_shader_strings = 1;

    size_t len = (int)(ceil)((float)count / (float)m_num_shader_strings);
    size_t ptr_len = 0, i = 0;
    while (count > 0) {
        return_data[i] = (char *)malloc(len + 2);
        memcpy(return_data[i], fdata + ptr_len, len);
        return_data[i][len] = '\0';
        count -= (len);
        ptr_len += (len);
        if (count < len) {
            if (count == 0) {
                m_num_shader_strings = (i + 1);
                break;
            }
            len = count;
        }
        ++i;
    }
    return return_data;
}

void VkTestFramework::FreeFileData(char **data) {
    for (int i = 0; i < m_num_shader_strings; i++)
        free(data[i]);
}

//
//   Deduce the language from the filename.  Files must end in one of the
//   following extensions:
//
//   .vert = vertex
//   .tesc = tessellation control
//   .tese = tessellation evaluation
//   .geom = geometry
//   .frag = fragment
//   .comp = compute
//
EShLanguage VkTestFramework::FindLanguage(const std::string &name) {
    size_t ext = name.rfind('.');
    if (ext == std::string::npos) {
        return EShLangVertex;
    }

    std::string suffix = name.substr(ext + 1, std::string::npos);
    if (suffix == "vert")
        return EShLangVertex;
    else if (suffix == "tesc")
        return EShLangTessControl;
    else if (suffix == "tese")
        return EShLangTessEvaluation;
    else if (suffix == "geom")
        return EShLangGeometry;
    else if (suffix == "frag")
        return EShLangFragment;
    else if (suffix == "comp")
        return EShLangCompute;

    return EShLangVertex;
}

//
// Convert VK shader type to compiler's
//
EShLanguage VkTestFramework::FindLanguage(const VkShaderStageFlagBits shader_type) {
    switch (shader_type) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return EShLangVertex;

    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return EShLangTessControl;

    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return EShLangTessEvaluation;

    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return EShLangGeometry;

    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return EShLangFragment;

    case VK_SHADER_STAGE_COMPUTE_BIT:
        return EShLangCompute;

    default:
        return EShLangVertex;
    }
}

//
// Compile a given string containing GLSL into SPV for use by VK
// Return value of false means an error was encountered.
//
bool VkTestFramework::GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv) {
    glslang::TProgram program;
    const char *shaderStrings[1];

    // TODO: Do we want to load a special config file depending on the
    // shader source? Optional name maybe?
    //    SetConfigFile(fileName);

    ProcessConfigFile();

    EShMessages messages = EShMsgDefault;
    SetMessageOptions(messages);
    messages = static_cast<EShMessages>(messages | EShMsgSpvRules | EShMsgVulkanRules);

    EShLanguage stage = FindLanguage(shader_type);
    glslang::TShader *shader = new glslang::TShader(stage);

    shaderStrings[0] = pshader;
    shader->setStrings(shaderStrings, 1);

    if (!shader->parse(&Resources, (m_compile_options & EOptionDefaultDesktop) ? 110 : 100, false, messages)) {

        if (!(m_compile_options & EOptionSuppressInfolog)) {
            puts(shader->getInfoLog());
            puts(shader->getInfoDebugLog());
        }

        return false; // something didn't work
    }

    program.addShader(shader);

    //
    // Program-level processing...
    //

    if (!program.link(messages)) {

        if (!(m_compile_options & EOptionSuppressInfolog)) {
            puts(shader->getInfoLog());
            puts(shader->getInfoDebugLog());
        }

        return false;
    }

    if (m_compile_options & EOptionDumpReflection) {
        program.buildReflection();
        program.dumpReflection();
    }

    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    //
    // Test the different modes of SPIR-V modification
    //
    if (this->m_canonicalize_spv) {
        spv::spirvbin_t(0).remap(spirv, spv::spirvbin_t::ALL_BUT_STRIP);
    }

    if (this->m_strip_spv) {
        spv::spirvbin_t(0).remap(spirv, spv::spirvbin_t::STRIP);
    }

    if (this->m_do_everything_spv) {
        spv::spirvbin_t(0).remap(spirv, spv::spirvbin_t::DO_EVERYTHING);
    }

    delete shader;

    return true;
}
