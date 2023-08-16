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

#include "dawn_native/BindGroupLayout.h"

#include "common/BitSetIterator.h"
#include "common/HashUtils.h"
#include "dawn_native/Device.h"
#include "dawn_native/PerStage.h"
#include "dawn_native/ValidationUtils_autogen.h"

#include <algorithm>
#include <functional>
#include <set>

namespace dawn_native {

    MaybeError ValidateBindingTypeWithShaderStageVisibility(
        wgpu::BindingType bindingType,
        wgpu::ShaderStage shaderStageVisibility) {
        // TODO(jiawei.shao@intel.com): support read-write storage textures.
        switch (bindingType) {
            case wgpu::BindingType::StorageBuffer: {
                if ((shaderStageVisibility & wgpu::ShaderStage::Vertex) != 0) {
                    return DAWN_VALIDATION_ERROR(
                        "storage buffer binding is not supported in vertex shader");
                }
                break;
            }

            case wgpu::BindingType::WriteonlyStorageTexture: {
                if ((shaderStageVisibility & wgpu::ShaderStage::Vertex) != 0) {
                    return DAWN_VALIDATION_ERROR(
                        "write-only storage texture binding is not supported in vertex shader");
                }
                break;
            }

            case wgpu::BindingType::StorageTexture: {
                return DAWN_VALIDATION_ERROR("Read-write storage texture binding is not supported");
            }

            case wgpu::BindingType::UniformBuffer:
            case wgpu::BindingType::ReadonlyStorageBuffer:
            case wgpu::BindingType::Sampler:
            case wgpu::BindingType::ComparisonSampler:
            case wgpu::BindingType::SampledTexture:
            case wgpu::BindingType::ReadonlyStorageTexture:
                break;
        }

        return {};
    }

    MaybeError ValidateStorageTextureFormat(DeviceBase* device,
                                            wgpu::BindingType bindingType,
                                            wgpu::TextureFormat storageTextureFormat) {
        switch (bindingType) {
            case wgpu::BindingType::ReadonlyStorageTexture:
            case wgpu::BindingType::WriteonlyStorageTexture: {
                if (storageTextureFormat == wgpu::TextureFormat::Undefined) {
                    return DAWN_VALIDATION_ERROR("Storage texture format is missing");
                }
                DAWN_TRY(ValidateTextureFormat(storageTextureFormat));

                const Format* format = nullptr;
                DAWN_TRY_ASSIGN(format, device->GetInternalFormat(storageTextureFormat));
                ASSERT(format != nullptr);
                if (!format->supportsStorageUsage) {
                    return DAWN_VALIDATION_ERROR("The storage texture format is not supported");
                }
                break;
            }

            case wgpu::BindingType::StorageBuffer:
            case wgpu::BindingType::UniformBuffer:
            case wgpu::BindingType::ReadonlyStorageBuffer:
            case wgpu::BindingType::Sampler:
            case wgpu::BindingType::ComparisonSampler:
            case wgpu::BindingType::SampledTexture:
                break;
            default:
                UNREACHABLE();
                break;
        }

        return {};
    }

    MaybeError ValidateStorageTextureViewDimension(wgpu::BindingType bindingType,
                                                   wgpu::TextureViewDimension dimension) {
        switch (bindingType) {
            case wgpu::BindingType::ReadonlyStorageTexture:
            case wgpu::BindingType::WriteonlyStorageTexture: {
                break;
            }

            case wgpu::BindingType::StorageBuffer:
            case wgpu::BindingType::UniformBuffer:
            case wgpu::BindingType::ReadonlyStorageBuffer:
            case wgpu::BindingType::Sampler:
            case wgpu::BindingType::ComparisonSampler:
            case wgpu::BindingType::SampledTexture:
                return {};

            case wgpu::BindingType::StorageTexture:
            default:
                UNREACHABLE();
                return {};
        }

        switch (dimension) {
            case wgpu::TextureViewDimension::Cube:
            case wgpu::TextureViewDimension::CubeArray:
                return DAWN_VALIDATION_ERROR(
                    "Cube map and cube map texture views cannot be used as storage textures");

            case wgpu::TextureViewDimension::e1D:
            case wgpu::TextureViewDimension::e2D:
            case wgpu::TextureViewDimension::e2DArray:
            case wgpu::TextureViewDimension::e3D:
                return {};

            case wgpu::TextureViewDimension::Undefined:
            default:
                UNREACHABLE();
                return {};
        }
    }

    MaybeError ValidateBindingCanBeMultisampled(wgpu::BindingType bindingType,
                                                wgpu::TextureViewDimension viewDimension) {
        switch (bindingType) {
            case wgpu::BindingType::SampledTexture:
                break;

            case wgpu::BindingType::ReadonlyStorageTexture:
            case wgpu::BindingType::WriteonlyStorageTexture:
                return DAWN_VALIDATION_ERROR("Storage texture bindings may not be multisampled");

            case wgpu::BindingType::StorageBuffer:
            case wgpu::BindingType::UniformBuffer:
            case wgpu::BindingType::ReadonlyStorageBuffer:
                return DAWN_VALIDATION_ERROR("Buffer bindings may not be multisampled");

            case wgpu::BindingType::Sampler:
            case wgpu::BindingType::ComparisonSampler:
                return DAWN_VALIDATION_ERROR("Sampler bindings may not be multisampled");

            case wgpu::BindingType::StorageTexture:
            default:
                UNREACHABLE();
                return {};
        }

        switch (viewDimension) {
            case wgpu::TextureViewDimension::e2D:
                break;

            case wgpu::TextureViewDimension::e2DArray:
                return DAWN_VALIDATION_ERROR("2D array texture bindings may not be multisampled");

            case wgpu::TextureViewDimension::Cube:
            case wgpu::TextureViewDimension::CubeArray:
                return DAWN_VALIDATION_ERROR("Cube texture bindings may not be multisampled");

            case wgpu::TextureViewDimension::e3D:
                return DAWN_VALIDATION_ERROR("3D texture bindings may not be multisampled");

            case wgpu::TextureViewDimension::e1D:
                return DAWN_VALIDATION_ERROR("1D texture bindings may not be multisampled");

            case wgpu::TextureViewDimension::Undefined:
            default:
                UNREACHABLE();
                return {};
        }

        return {};
    }

    MaybeError ValidateBindGroupLayoutDescriptor(DeviceBase* device,
                                                 const BindGroupLayoutDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        std::set<BindingNumber> bindingsSet;
        BindingCounts bindingCounts = {};
        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const BindGroupLayoutEntry& entry = descriptor->entries[i];
            BindingNumber bindingNumber = BindingNumber(entry.binding);

            DAWN_TRY(ValidateShaderStage(entry.visibility));
            DAWN_TRY(ValidateBindingType(entry.type));
            DAWN_TRY(ValidateTextureComponentType(entry.textureComponentType));

            wgpu::TextureViewDimension viewDimension = wgpu::TextureViewDimension::e2D;
            if (entry.viewDimension != wgpu::TextureViewDimension::Undefined) {
                DAWN_TRY(ValidateTextureViewDimension(entry.viewDimension));
                viewDimension = entry.viewDimension;
            }

            if (bindingsSet.count(bindingNumber) != 0) {
                return DAWN_VALIDATION_ERROR("some binding index was specified more than once");
            }

            DAWN_TRY(ValidateBindingTypeWithShaderStageVisibility(entry.type, entry.visibility));

            DAWN_TRY(ValidateStorageTextureFormat(device, entry.type, entry.storageTextureFormat));

            DAWN_TRY(ValidateStorageTextureViewDimension(entry.type, viewDimension));

            if (entry.multisampled) {
                DAWN_TRY(ValidateBindingCanBeMultisampled(entry.type, viewDimension));
            }

            switch (entry.type) {
                case wgpu::BindingType::UniformBuffer:
                case wgpu::BindingType::StorageBuffer:
                case wgpu::BindingType::ReadonlyStorageBuffer:
                    break;
                case wgpu::BindingType::SampledTexture:
                    if (entry.hasDynamicOffset) {
                        return DAWN_VALIDATION_ERROR("Sampled textures cannot be dynamic");
                    }
                    break;
                case wgpu::BindingType::Sampler:
                case wgpu::BindingType::ComparisonSampler:
                    if (entry.hasDynamicOffset) {
                        return DAWN_VALIDATION_ERROR("Samplers cannot be dynamic");
                    }
                    break;
                case wgpu::BindingType::ReadonlyStorageTexture:
                case wgpu::BindingType::WriteonlyStorageTexture:
                    if (entry.hasDynamicOffset) {
                        return DAWN_VALIDATION_ERROR("Storage textures cannot be dynamic");
                    }
                    break;
                case wgpu::BindingType::StorageTexture:
                    return DAWN_VALIDATION_ERROR("storage textures aren't supported (yet)");
                default:
                    UNREACHABLE();
                    break;
            }

            IncrementBindingCounts(&bindingCounts, entry);

            bindingsSet.insert(bindingNumber);
        }

        DAWN_TRY(ValidateBindingCounts(bindingCounts));

        return {};
    }

    namespace {

        void HashCombineBindingInfo(size_t* hash, const BindingInfo& info) {
            HashCombine(hash, info.hasDynamicOffset, info.multisampled, info.visibility, info.type,
                        info.textureComponentType, info.viewDimension, info.storageTextureFormat,
                        info.minBufferBindingSize);
        }

        bool operator!=(const BindingInfo& a, const BindingInfo& b) {
            return a.hasDynamicOffset != b.hasDynamicOffset ||          //
                   a.multisampled != b.multisampled ||                  //
                   a.visibility != b.visibility ||                      //
                   a.type != b.type ||                                  //
                   a.textureComponentType != b.textureComponentType ||  //
                   a.viewDimension != b.viewDimension ||                //
                   a.storageTextureFormat != b.storageTextureFormat ||  //
                   a.minBufferBindingSize != b.minBufferBindingSize;
        }

        bool IsBufferBinding(wgpu::BindingType bindingType) {
            switch (bindingType) {
                case wgpu::BindingType::UniformBuffer:
                case wgpu::BindingType::StorageBuffer:
                case wgpu::BindingType::ReadonlyStorageBuffer:
                    return true;
                case wgpu::BindingType::SampledTexture:
                case wgpu::BindingType::Sampler:
                case wgpu::BindingType::ComparisonSampler:
                case wgpu::BindingType::StorageTexture:
                case wgpu::BindingType::ReadonlyStorageTexture:
                case wgpu::BindingType::WriteonlyStorageTexture:
                    return false;
                default:
                    UNREACHABLE();
                    return false;
            }
        }

        bool SortBindingsCompare(const BindGroupLayoutEntry& a, const BindGroupLayoutEntry& b) {
            const bool aIsBuffer = IsBufferBinding(a.type);
            const bool bIsBuffer = IsBufferBinding(b.type);
            if (aIsBuffer != bIsBuffer) {
                // Always place buffers first.
                return aIsBuffer;
            } else {
                if (aIsBuffer) {
                    ASSERT(bIsBuffer);
                    if (a.hasDynamicOffset != b.hasDynamicOffset) {
                        // Buffers with dynamic offsets should come before those without.
                        // This makes it easy to iterate over the dynamic buffer bindings
                        // [0, dynamicBufferCount) during validation.
                        return a.hasDynamicOffset;
                    }
                    if (a.hasDynamicOffset) {
                        ASSERT(b.hasDynamicOffset);
                        ASSERT(a.binding != b.binding);
                        // Above, we ensured that dynamic buffers are first. Now, ensure that
                        // dynamic buffer bindings are in increasing order. This is because dynamic
                        // buffer offsets are applied in increasing order of binding number.
                        return a.binding < b.binding;
                    }
                }
                // Otherwise, sort by type.
                if (a.type != b.type) {
                    return a.type < b.type;
                }
            }
            if (a.visibility != b.visibility) {
                return a.visibility < b.visibility;
            }
            if (a.multisampled != b.multisampled) {
                return a.multisampled < b.multisampled;
            }
            if (a.viewDimension != b.viewDimension) {
                return a.viewDimension < b.viewDimension;
            }
            if (a.textureComponentType != b.textureComponentType) {
                return a.textureComponentType < b.textureComponentType;
            }
            if (a.storageTextureFormat != b.storageTextureFormat) {
                return a.storageTextureFormat < b.storageTextureFormat;
            }
            if (a.minBufferBindingSize != b.minBufferBindingSize) {
                return a.minBufferBindingSize < b.minBufferBindingSize;
            }
            return false;
        }

        // This is a utility function to help ASSERT that the BGL-binding comparator places buffers
        // first.
        bool CheckBufferBindingsFirst(ityp::span<BindingIndex, const BindingInfo> bindings) {
            BindingIndex lastBufferIndex{0};
            BindingIndex firstNonBufferIndex = std::numeric_limits<BindingIndex>::max();
            for (BindingIndex i{0}; i < bindings.size(); ++i) {
                if (IsBufferBinding(bindings[i].type)) {
                    lastBufferIndex = std::max(i, lastBufferIndex);
                } else {
                    firstNonBufferIndex = std::min(i, firstNonBufferIndex);
                }
            }

            // If there are no buffers, then |lastBufferIndex| is initialized to 0 and
            // |firstNonBufferIndex| gets set to 0.
            return firstNonBufferIndex >= lastBufferIndex;
        }

    }  // namespace

    // BindGroupLayoutBase

    BindGroupLayoutBase::BindGroupLayoutBase(DeviceBase* device,
                                             const BindGroupLayoutDescriptor* descriptor)
        : CachedObject(device), mBindingInfo(BindingIndex(descriptor->entryCount)) {
        std::vector<BindGroupLayoutEntry> sortedBindings(
            descriptor->entries, descriptor->entries + descriptor->entryCount);
        std::sort(sortedBindings.begin(), sortedBindings.end(), SortBindingsCompare);

        for (BindingIndex i{0}; i < mBindingInfo.size(); ++i) {
            const BindGroupLayoutEntry& binding = sortedBindings[static_cast<uint32_t>(i)];
            mBindingInfo[i].binding = BindingNumber(binding.binding);
            mBindingInfo[i].type = binding.type;
            mBindingInfo[i].visibility = binding.visibility;
            mBindingInfo[i].textureComponentType =
                Format::TextureComponentTypeToFormatType(binding.textureComponentType);
            mBindingInfo[i].storageTextureFormat = binding.storageTextureFormat;
            mBindingInfo[i].minBufferBindingSize = binding.minBufferBindingSize;

            if (binding.viewDimension == wgpu::TextureViewDimension::Undefined) {
                mBindingInfo[i].viewDimension = wgpu::TextureViewDimension::e2D;
            } else {
                mBindingInfo[i].viewDimension = binding.viewDimension;
            }

            mBindingInfo[i].multisampled = binding.multisampled;
            mBindingInfo[i].hasDynamicOffset = binding.hasDynamicOffset;

            if (IsBufferBinding(binding.type)) {
                // Buffers must be contiguously packed at the start of the binding info.
                ASSERT(GetBufferCount() == i);
            }
            IncrementBindingCounts(&mBindingCounts, binding);

            const auto& it = mBindingMap.emplace(BindingNumber(binding.binding), i);
            ASSERT(it.second);
        }
        ASSERT(CheckBufferBindingsFirst({mBindingInfo.data(), GetBindingCount()}));
        ASSERT(mBindingInfo.size() <= kMaxBindingsPerPipelineLayoutTyped);
    }

    BindGroupLayoutBase::BindGroupLayoutBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : CachedObject(device, tag) {
    }

    BindGroupLayoutBase::~BindGroupLayoutBase() {
        // Do not uncache the actual cached object if we are a blueprint
        if (IsCachedReference()) {
            GetDevice()->UncacheBindGroupLayout(this);
        }
    }

    // static
    BindGroupLayoutBase* BindGroupLayoutBase::MakeError(DeviceBase* device) {
        return new BindGroupLayoutBase(device, ObjectBase::kError);
    }

    const BindGroupLayoutBase::BindingMap& BindGroupLayoutBase::GetBindingMap() const {
        ASSERT(!IsError());
        return mBindingMap;
    }

    BindingIndex BindGroupLayoutBase::GetBindingIndex(BindingNumber bindingNumber) const {
        ASSERT(!IsError());
        const auto& it = mBindingMap.find(bindingNumber);
        ASSERT(it != mBindingMap.end());
        return it->second;
    }

    size_t BindGroupLayoutBase::HashFunc::operator()(const BindGroupLayoutBase* bgl) const {
        size_t hash = 0;
        // std::map is sorted by key, so two BGLs constructed in different orders
        // will still hash the same.
        for (const auto& it : bgl->mBindingMap) {
            HashCombine(&hash, it.first, it.second);
            HashCombineBindingInfo(&hash, bgl->mBindingInfo[it.second]);
        }
        return hash;
    }

    bool BindGroupLayoutBase::EqualityFunc::operator()(const BindGroupLayoutBase* a,
                                                       const BindGroupLayoutBase* b) const {
        if (a->GetBindingCount() != b->GetBindingCount()) {
            return false;
        }
        for (BindingIndex i{0}; i < a->GetBindingCount(); ++i) {
            if (a->mBindingInfo[i] != b->mBindingInfo[i]) {
                return false;
            }
        }
        return a->mBindingMap == b->mBindingMap;
    }

    BindingIndex BindGroupLayoutBase::GetBindingCount() const {
        return mBindingInfo.size();
    }

    BindingIndex BindGroupLayoutBase::GetBufferCount() const {
        return BindingIndex(mBindingCounts.bufferCount);
    }

    BindingIndex BindGroupLayoutBase::GetDynamicBufferCount() const {
        // This is a binding index because dynamic buffers are packed at the front of the binding
        // info.
        return static_cast<BindingIndex>(mBindingCounts.dynamicStorageBufferCount +
                                         mBindingCounts.dynamicUniformBufferCount);
    }

    uint32_t BindGroupLayoutBase::GetUnverifiedBufferCount() const {
        return mBindingCounts.unverifiedBufferCount;
    }

    const BindingCounts& BindGroupLayoutBase::GetBindingCountInfo() const {
        return mBindingCounts;
    }

    size_t BindGroupLayoutBase::GetBindingDataSize() const {
        // | ------ buffer-specific ----------| ------------ object pointers -------------|
        // | --- offsets + sizes -------------| --------------- Ref<ObjectBase> ----------|
        // Followed by:
        // |---------buffer size array--------|
        // |-uint64_t[mUnverifiedBufferCount]-|
        size_t objectPointerStart = mBindingCounts.bufferCount * sizeof(BufferBindingData);
        ASSERT(IsAligned(objectPointerStart, alignof(Ref<ObjectBase>)));
        size_t bufferSizeArrayStart =
            Align(objectPointerStart + mBindingCounts.totalCount * sizeof(Ref<ObjectBase>),
                  sizeof(uint64_t));
        ASSERT(IsAligned(bufferSizeArrayStart, alignof(uint64_t)));
        return bufferSizeArrayStart + mBindingCounts.unverifiedBufferCount * sizeof(uint64_t);
    }

    BindGroupLayoutBase::BindingDataPointers BindGroupLayoutBase::ComputeBindingDataPointers(
        void* dataStart) const {
        BufferBindingData* bufferData = reinterpret_cast<BufferBindingData*>(dataStart);
        auto bindings = reinterpret_cast<Ref<ObjectBase>*>(bufferData + mBindingCounts.bufferCount);
        uint64_t* unverifiedBufferSizes = AlignPtr(
            reinterpret_cast<uint64_t*>(bindings + mBindingCounts.totalCount), sizeof(uint64_t));

        ASSERT(IsPtrAligned(bufferData, alignof(BufferBindingData)));
        ASSERT(IsPtrAligned(bindings, alignof(Ref<ObjectBase>)));
        ASSERT(IsPtrAligned(unverifiedBufferSizes, alignof(uint64_t)));

        return {{bufferData, GetBufferCount()},
                {bindings, GetBindingCount()},
                {unverifiedBufferSizes, mBindingCounts.unverifiedBufferCount}};
    }

}  // namespace dawn_native
