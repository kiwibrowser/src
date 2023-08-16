// Copyright 2017 The Dawn Authors
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

#include "dawn_native/metal/PipelineLayoutMTL.h"

#include "common/BitSetIterator.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/metal/DeviceMTL.h"

namespace dawn_native { namespace metal {

    PipelineLayout::PipelineLayout(Device* device, const PipelineLayoutDescriptor* descriptor)
        : PipelineLayoutBase(device, descriptor) {
        // Each stage has its own numbering namespace in CompilerMSL.
        for (auto stage : IterateStages(kAllStages)) {
            uint32_t bufferIndex = 0;
            uint32_t samplerIndex = 0;
            uint32_t textureIndex = 0;

            for (BindGroupIndex group : IterateBitSet(GetBindGroupLayoutsMask())) {
                mIndexInfo[stage][group].resize(GetBindGroupLayout(group)->GetBindingCount());

                for (BindingIndex bindingIndex{0};
                     bindingIndex < GetBindGroupLayout(group)->GetBindingCount(); ++bindingIndex) {
                    const BindingInfo& bindingInfo =
                        GetBindGroupLayout(group)->GetBindingInfo(bindingIndex);
                    if (!(bindingInfo.visibility & StageBit(stage))) {
                        continue;
                    }

                    switch (bindingInfo.type) {
                        case wgpu::BindingType::UniformBuffer:
                        case wgpu::BindingType::StorageBuffer:
                        case wgpu::BindingType::ReadonlyStorageBuffer:
                            mIndexInfo[stage][group][bindingIndex] = bufferIndex;
                            bufferIndex++;
                            break;
                        case wgpu::BindingType::Sampler:
                        case wgpu::BindingType::ComparisonSampler:
                            mIndexInfo[stage][group][bindingIndex] = samplerIndex;
                            samplerIndex++;
                            break;
                        case wgpu::BindingType::SampledTexture:
                        case wgpu::BindingType::ReadonlyStorageTexture:
                        case wgpu::BindingType::WriteonlyStorageTexture:
                            mIndexInfo[stage][group][bindingIndex] = textureIndex;
                            textureIndex++;
                            break;
                        case wgpu::BindingType::StorageTexture:
                            UNREACHABLE();
                            break;
                    }
                }
            }

            mBufferBindingCount[stage] = bufferIndex;
        }
    }

    const PipelineLayout::BindingIndexInfo& PipelineLayout::GetBindingIndexInfo(
        SingleShaderStage stage) const {
        return mIndexInfo[stage];
    }

    uint32_t PipelineLayout::GetBufferBindingCount(SingleShaderStage stage) {
        return mBufferBindingCount[stage];
    }

}}  // namespace dawn_native::metal
