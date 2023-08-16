// Copyright 2020 The Dawn Authors
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

#include "dawn_native/opengl/BindGroupGL.h"

#include "dawn_native/Texture.h"
#include "dawn_native/opengl/BindGroupLayoutGL.h"
#include "dawn_native/opengl/DeviceGL.h"

namespace dawn_native { namespace opengl {

    MaybeError ValidateGLBindGroupDescriptor(const BindGroupDescriptor* descriptor) {
        const BindGroupLayoutBase::BindingMap& bindingMap = descriptor->layout->GetBindingMap();
        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const BindGroupEntry& entry = descriptor->entries[i];

            const auto& it = bindingMap.find(BindingNumber(entry.binding));
            BindingIndex bindingIndex = it->second;
            ASSERT(bindingIndex < descriptor->layout->GetBindingCount());

            const BindingInfo& bindingInfo = descriptor->layout->GetBindingInfo(bindingIndex);
            switch (bindingInfo.type) {
                case wgpu::BindingType::ReadonlyStorageTexture:
                case wgpu::BindingType::WriteonlyStorageTexture: {
                    ASSERT(entry.textureView != nullptr);
                    const uint32_t textureViewLayerCount = entry.textureView->GetLayerCount();
                    if (textureViewLayerCount != 1 &&
                        textureViewLayerCount !=
                            entry.textureView->GetTexture()->GetArrayLayers()) {
                        return DAWN_VALIDATION_ERROR(
                            "Currently the OpenGL backend only supports either binding a layer or "
                            "the entire texture as storage texture.");
                    }
                } break;

                case wgpu::BindingType::UniformBuffer:
                case wgpu::BindingType::StorageBuffer:
                case wgpu::BindingType::ReadonlyStorageBuffer:
                case wgpu::BindingType::SampledTexture:
                case wgpu::BindingType::Sampler:
                case wgpu::BindingType::ComparisonSampler:
                    break;

                case wgpu::BindingType::StorageTexture:
                default:
                    UNREACHABLE();
                    break;
            }
        }

        return {};
    }

    BindGroup::BindGroup(Device* device, const BindGroupDescriptor* descriptor)
        : BindGroupBase(this, device, descriptor) {
    }

    BindGroup::~BindGroup() {
        ToBackend(GetLayout())->DeallocateBindGroup(this);
    }

    // static
    BindGroup* BindGroup::Create(Device* device, const BindGroupDescriptor* descriptor) {
        return ToBackend(descriptor->layout)->AllocateBindGroup(device, descriptor);
    }

}}  // namespace dawn_native::opengl
