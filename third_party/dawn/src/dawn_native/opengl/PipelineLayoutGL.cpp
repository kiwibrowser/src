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

#include "dawn_native/opengl/PipelineLayoutGL.h"

#include "common/BitSetIterator.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/opengl/DeviceGL.h"

namespace dawn_native { namespace opengl {

    PipelineLayout::PipelineLayout(Device* device, const PipelineLayoutDescriptor* descriptor)
        : PipelineLayoutBase(device, descriptor) {
        GLuint uboIndex = 0;
        GLuint samplerIndex = 0;
        GLuint sampledTextureIndex = 0;
        GLuint ssboIndex = 0;
        GLuint storageTextureIndex = 0;

        for (BindGroupIndex group : IterateBitSet(GetBindGroupLayoutsMask())) {
            const BindGroupLayoutBase* bgl = GetBindGroupLayout(group);
            mIndexInfo[group].resize(bgl->GetBindingCount());

            for (BindingIndex bindingIndex{0}; bindingIndex < bgl->GetBindingCount();
                 ++bindingIndex) {
                switch (bgl->GetBindingInfo(bindingIndex).type) {
                    case wgpu::BindingType::UniformBuffer:
                        mIndexInfo[group][bindingIndex] = uboIndex;
                        uboIndex++;
                        break;
                    case wgpu::BindingType::Sampler:
                    case wgpu::BindingType::ComparisonSampler:
                        mIndexInfo[group][bindingIndex] = samplerIndex;
                        samplerIndex++;
                        break;
                    case wgpu::BindingType::SampledTexture:
                        mIndexInfo[group][bindingIndex] = sampledTextureIndex;
                        sampledTextureIndex++;
                        break;

                    case wgpu::BindingType::StorageBuffer:
                    case wgpu::BindingType::ReadonlyStorageBuffer:
                        mIndexInfo[group][bindingIndex] = ssboIndex;
                        ssboIndex++;
                        break;

                    case wgpu::BindingType::ReadonlyStorageTexture:
                    case wgpu::BindingType::WriteonlyStorageTexture:
                        mIndexInfo[group][bindingIndex] = storageTextureIndex;
                        storageTextureIndex++;
                        break;

                    case wgpu::BindingType::StorageTexture:
                        UNREACHABLE();
                        break;

                        // TODO(shaobo.yan@intel.com): Implement dynamic buffer offset
                }
            }
        }

        mNumSamplers = samplerIndex;
        mNumSampledTextures = sampledTextureIndex;
    }

    const PipelineLayout::BindingIndexInfo& PipelineLayout::GetBindingIndexInfo() const {
        return mIndexInfo;
    }

    GLuint PipelineLayout::GetTextureUnitsUsed() const {
        return 0;
    }

    size_t PipelineLayout::GetNumSamplers() const {
        return mNumSamplers;
    }

    size_t PipelineLayout::GetNumSampledTextures() const {
        return mNumSampledTextures;
    }

}}  // namespace dawn_native::opengl
