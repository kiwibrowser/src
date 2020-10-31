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

#ifndef DAWNNATIVE_BINDINGINFO_H_
#define DAWNNATIVE_BINDINGINFO_H_

#include "common/Constants.h"
#include "common/TypedInteger.h"
#include "common/ityp_array.h"
#include "dawn_native/Error.h"
#include "dawn_native/Format.h"
#include "dawn_native/PerStage.h"

#include "dawn_native/dawn_platform.h"

#include <cstdint>

namespace dawn_native {

    // Binding numbers in the shader and BindGroup/BindGroupLayoutDescriptors
    using BindingNumber = TypedInteger<struct BindingNumberT, uint32_t>;

    // Binding numbers get mapped to a packed range of indices
    using BindingIndex = TypedInteger<struct BindingIndexT, uint32_t>;

    using BindGroupIndex = TypedInteger<struct BindGroupIndexT, uint32_t>;

    static constexpr BindGroupIndex kMaxBindGroupsTyped = BindGroupIndex(kMaxBindGroups);

    // Not a real WebGPU limit, but the sum of the two limits is useful for internal optimizations.
    static constexpr uint32_t kMaxDynamicBuffersPerPipelineLayout =
        kMaxDynamicUniformBuffersPerPipelineLayout + kMaxDynamicStorageBuffersPerPipelineLayout;

    static constexpr BindingIndex kMaxDynamicBuffersPerPipelineLayoutTyped =
        BindingIndex(kMaxDynamicBuffersPerPipelineLayout);

    // Not a real WebGPU limit, but used to optimize parts of Dawn which expect valid usage of the
    // API. There should never be more bindings than the max per stage, for each stage.
    static constexpr uint32_t kMaxBindingsPerPipelineLayout =
        3 * (kMaxSampledTexturesPerShaderStage + kMaxSamplersPerShaderStage +
             kMaxStorageBuffersPerShaderStage + kMaxStorageTexturesPerShaderStage +
             kMaxUniformBuffersPerShaderStage);

    static constexpr BindingIndex kMaxBindingsPerPipelineLayoutTyped =
        BindingIndex(kMaxBindingsPerPipelineLayout);

    // TODO(enga): Figure out a good number for this.
    static constexpr uint32_t kMaxOptimalBindingsPerGroup = 32;

    struct BindingInfo {
        BindingNumber binding;
        wgpu::ShaderStage visibility;
        wgpu::BindingType type;
        Format::Type textureComponentType = Format::Type::Float;
        wgpu::TextureViewDimension viewDimension = wgpu::TextureViewDimension::Undefined;
        wgpu::TextureFormat storageTextureFormat = wgpu::TextureFormat::Undefined;
        bool hasDynamicOffset = false;
        bool multisampled = false;
        uint64_t minBufferBindingSize = 0;
    };

    struct PerStageBindingCounts {
        uint32_t sampledTextureCount;
        uint32_t samplerCount;
        uint32_t storageBufferCount;
        uint32_t storageTextureCount;
        uint32_t uniformBufferCount;
    };

    struct BindingCounts {
        uint32_t totalCount;
        uint32_t bufferCount;
        uint32_t unverifiedBufferCount;  // Buffers with minimum buffer size unspecified
        uint32_t dynamicUniformBufferCount;
        uint32_t dynamicStorageBufferCount;
        PerStage<PerStageBindingCounts> perStage;
    };

    void IncrementBindingCounts(BindingCounts* bindingCounts, const BindGroupLayoutEntry& entry);
    void AccumulateBindingCounts(BindingCounts* bindingCounts, const BindingCounts& rhs);
    MaybeError ValidateBindingCounts(const BindingCounts& bindingCounts);

    // For buffer size validation
    using RequiredBufferSizes = ityp::array<BindGroupIndex, std::vector<uint64_t>, kMaxBindGroups>;

}  // namespace dawn_native

#endif  // DAWNNATIVE_BINDINGINFO_H_
