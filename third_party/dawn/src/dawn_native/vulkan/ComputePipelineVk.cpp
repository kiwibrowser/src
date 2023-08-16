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

#include "dawn_native/vulkan/ComputePipelineVk.h"

#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/PipelineLayoutVk.h"
#include "dawn_native/vulkan/ShaderModuleVk.h"
#include "dawn_native/vulkan/UtilsVulkan.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    // static
    ResultOrError<ComputePipeline*> ComputePipeline::Create(
        Device* device,
        const ComputePipelineDescriptor* descriptor) {
        Ref<ComputePipeline> pipeline = AcquireRef(new ComputePipeline(device, descriptor));
        DAWN_TRY(pipeline->Initialize(descriptor));
        return pipeline.Detach();
    }

    MaybeError ComputePipeline::Initialize(const ComputePipelineDescriptor* descriptor) {
        VkComputePipelineCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.layout = ToBackend(descriptor->layout)->GetHandle();
        createInfo.basePipelineHandle = ::VK_NULL_HANDLE;
        createInfo.basePipelineIndex = -1;

        createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        createInfo.stage.pNext = nullptr;
        createInfo.stage.flags = 0;
        createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        createInfo.stage.module = ToBackend(descriptor->computeStage.module)->GetHandle();
        createInfo.stage.pName = descriptor->computeStage.entryPoint;
        createInfo.stage.pSpecializationInfo = nullptr;

        Device* device = ToBackend(GetDevice());

        PNextChainBuilder extChain(&createInfo);

        VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroupSizeInfo = {};
        uint32_t computeSubgroupSize = device->GetComputeSubgroupSize();
        if (computeSubgroupSize != 0u) {
            ASSERT(device->GetDeviceInfo().HasExt(DeviceExt::SubgroupSizeControl));
            subgroupSizeInfo.requiredSubgroupSize = computeSubgroupSize;
            extChain.Add(
                &subgroupSizeInfo,
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT);
        }

        return CheckVkSuccess(
            device->fn.CreateComputePipelines(device->GetVkDevice(), ::VK_NULL_HANDLE, 1,
                                              &createInfo, nullptr, &*mHandle),
            "CreateComputePipeline");
    }

    ComputePipeline::~ComputePipeline() {
        if (mHandle != VK_NULL_HANDLE) {
            ToBackend(GetDevice())->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkPipeline ComputePipeline::GetHandle() const {
        return mHandle;
    }

}}  // namespace dawn_native::vulkan
