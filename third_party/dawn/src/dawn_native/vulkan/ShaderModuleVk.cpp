// Copyright 2018 The Dawn Authors
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

#include "dawn_native/vulkan/ShaderModuleVk.h"

#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/VulkanError.h"

#include <spirv_cross.hpp>

namespace dawn_native { namespace vulkan {

    // static
    ResultOrError<ShaderModule*> ShaderModule::Create(Device* device,
                                                      const ShaderModuleDescriptor* descriptor) {
        Ref<ShaderModule> module = AcquireRef(new ShaderModule(device, descriptor));
        if (!module)
            return DAWN_VALIDATION_ERROR("Unable to create ShaderModule");
        DAWN_TRY(module->Initialize());
        return module.Detach();
    }

    ShaderModule::ShaderModule(Device* device, const ShaderModuleDescriptor* descriptor)
        : ShaderModuleBase(device, descriptor) {
    }

    MaybeError ShaderModule::Initialize() {
        DAWN_TRY(InitializeBase());
        const std::vector<uint32_t>& spirv = GetSpirv();

        // Use SPIRV-Cross to extract info from the SPIRV even if Vulkan consumes SPIRV. We want to
        // have a translation step eventually anyway.
        if (GetDevice()->IsToggleEnabled(Toggle::UseSpvc)) {
            shaderc_spvc::CompileOptions options = GetCompileOptions();

            DAWN_TRY(CheckSpvcSuccess(
                mSpvcContext.InitializeForVulkan(spirv.data(), spirv.size(), options),
                "Unable to initialize instance of spvc"));

            spirv_cross::Compiler* compiler;
            DAWN_TRY(CheckSpvcSuccess(mSpvcContext.GetCompiler(reinterpret_cast<void**>(&compiler)),
                                      "Unable to get cross compiler"));
            DAWN_TRY(ExtractSpirvInfo(*compiler));
        } else {
            spirv_cross::Compiler compiler(spirv);
            DAWN_TRY(ExtractSpirvInfo(compiler));
        }

        VkShaderModuleCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        std::vector<uint32_t> vulkanSource;
        if (GetDevice()->IsToggleEnabled(Toggle::UseSpvc)) {
            shaderc_spvc::CompilationResult result;
            DAWN_TRY(CheckSpvcSuccess(mSpvcContext.CompileShader(&result),
                                      "Unable to generate Vulkan shader"));
            DAWN_TRY(CheckSpvcSuccess(result.GetBinaryOutput(&vulkanSource),
                                      "Unable to get binary output of Vulkan shader"));
            createInfo.codeSize = vulkanSource.size() * sizeof(uint32_t);
            createInfo.pCode = vulkanSource.data();
        } else {
            createInfo.codeSize = spirv.size() * sizeof(uint32_t);
            createInfo.pCode = spirv.data();
        }

        Device* device = ToBackend(GetDevice());
        return CheckVkSuccess(
            device->fn.CreateShaderModule(device->GetVkDevice(), &createInfo, nullptr, &*mHandle),
            "CreateShaderModule");
    }

    ShaderModule::~ShaderModule() {
        Device* device = ToBackend(GetDevice());

        if (mHandle != VK_NULL_HANDLE) {
            device->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkShaderModule ShaderModule::GetHandle() const {
        return mHandle;
    }

}}  // namespace dawn_native::vulkan
