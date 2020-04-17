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

#include "dawn_native/BindGroup.h"

#include "common/Assert.h"
#include "common/Math.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/Device.h"
#include "dawn_native/Sampler.h"
#include "dawn_native/Texture.h"

namespace dawn_native {

    namespace {

        // Helper functions to perform binding-type specific validation

        MaybeError ValidateBufferBinding(const DeviceBase* device,
                                         const BindGroupBinding& binding,
                                         dawn::BufferUsageBit requiredUsage) {
            if (binding.buffer == nullptr || binding.sampler != nullptr ||
                binding.textureView != nullptr) {
                return DAWN_VALIDATION_ERROR("expected buffer binding");
            }
            DAWN_TRY(device->ValidateObject(binding.buffer));

            uint64_t bufferSize = binding.buffer->GetSize();
            if (binding.size > bufferSize) {
                return DAWN_VALIDATION_ERROR("Buffer binding size larger than the buffer");
            }

            // Note that no overflow can happen because we already checked that
            // bufferSize >= binding.size
            if (binding.offset > bufferSize - binding.size) {
                return DAWN_VALIDATION_ERROR("Buffer binding doesn't fit in the buffer");
            }

            if (!IsAligned(binding.offset, 256)) {
                return DAWN_VALIDATION_ERROR(
                    "Buffer offset for bind group needs to be 256-byte aligned");
            }

            if (!(binding.buffer->GetUsage() & requiredUsage)) {
                return DAWN_VALIDATION_ERROR("buffer binding usage mismatch");
            }

            return {};
        }

        MaybeError ValidateTextureBinding(const DeviceBase* device,
                                          const BindGroupBinding& binding,
                                          dawn::TextureUsageBit requiredUsage) {
            if (binding.textureView == nullptr || binding.sampler != nullptr ||
                binding.buffer != nullptr) {
                return DAWN_VALIDATION_ERROR("expected texture binding");
            }
            DAWN_TRY(device->ValidateObject(binding.textureView));

            if (!(binding.textureView->GetTexture()->GetUsage() & requiredUsage)) {
                return DAWN_VALIDATION_ERROR("texture binding usage mismatch");
            }

            return {};
        }

        MaybeError ValidateSamplerBinding(const DeviceBase* device,
                                          const BindGroupBinding& binding) {
            if (binding.sampler == nullptr || binding.textureView != nullptr ||
                binding.buffer != nullptr) {
                return DAWN_VALIDATION_ERROR("expected sampler binding");
            }
            DAWN_TRY(device->ValidateObject(binding.sampler));

            return {};
        }

    }  // anonymous namespace

    MaybeError ValidateBindGroupDescriptor(DeviceBase* device,
                                           const BindGroupDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        DAWN_TRY(device->ValidateObject(descriptor->layout));

        const BindGroupLayoutBase::LayoutBindingInfo& layoutInfo =
            descriptor->layout->GetBindingInfo();

        if (descriptor->bindingCount != layoutInfo.mask.count()) {
            return DAWN_VALIDATION_ERROR("numBindings mismatch");
        }

        std::bitset<kMaxBindingsPerGroup> bindingsSet;
        for (uint32_t i = 0; i < descriptor->bindingCount; ++i) {
            const BindGroupBinding& binding = descriptor->bindings[i];
            uint32_t bindingIndex = binding.binding;

            // Check that we can set this binding.
            if (bindingIndex >= kMaxBindingsPerGroup) {
                return DAWN_VALIDATION_ERROR("binding index too high");
            }

            if (!layoutInfo.mask[bindingIndex]) {
                return DAWN_VALIDATION_ERROR("setting non-existent binding");
            }

            if (bindingsSet[bindingIndex]) {
                return DAWN_VALIDATION_ERROR("binding set twice");
            }
            bindingsSet.set(bindingIndex);

            // Perform binding-type specific validation.
            switch (layoutInfo.types[bindingIndex]) {
                case dawn::BindingType::UniformBuffer:
                case dawn::BindingType::DynamicUniformBuffer:
                    DAWN_TRY(ValidateBufferBinding(device, binding, dawn::BufferUsageBit::Uniform));
                    break;
                case dawn::BindingType::StorageBuffer:
                case dawn::BindingType::DynamicStorageBuffer:
                    DAWN_TRY(ValidateBufferBinding(device, binding, dawn::BufferUsageBit::Storage));
                    break;
                case dawn::BindingType::SampledTexture:
                    DAWN_TRY(
                        ValidateTextureBinding(device, binding, dawn::TextureUsageBit::Sampled));
                    break;
                case dawn::BindingType::Sampler:
                    DAWN_TRY(ValidateSamplerBinding(device, binding));
                    break;
            }
        }

        // This should always be true because
        //  - numBindings has to match between the bind group and its layout.
        //  - Each binding must be set at most once
        //
        // We don't validate the equality because it wouldn't be possible to cover it with a test.
        ASSERT(bindingsSet == layoutInfo.mask);

        return {};
    }

    // BindGroup

    BindGroupBase::BindGroupBase(DeviceBase* device, const BindGroupDescriptor* descriptor)
        : ObjectBase(device), mLayout(descriptor->layout) {
        for (uint32_t i = 0; i < descriptor->bindingCount; ++i) {
            const BindGroupBinding& binding = descriptor->bindings[i];

            uint32_t bindingIndex = binding.binding;
            ASSERT(bindingIndex < kMaxBindingsPerGroup);

            // Only a single binding type should be set, so once we found it we can skip to the
            // next loop iteration.

            if (binding.buffer != nullptr) {
                ASSERT(mBindings[bindingIndex].Get() == nullptr);
                mBindings[bindingIndex] = binding.buffer;
                mOffsets[bindingIndex] = binding.offset;
                mSizes[bindingIndex] = binding.size;
                continue;
            }

            if (binding.textureView != nullptr) {
                ASSERT(mBindings[bindingIndex].Get() == nullptr);
                mBindings[bindingIndex] = binding.textureView;
                continue;
            }

            if (binding.sampler != nullptr) {
                ASSERT(mBindings[bindingIndex].Get() == nullptr);
                mBindings[bindingIndex] = binding.sampler;
                continue;
            }
        }
    }

    BindGroupBase::BindGroupBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag) {
    }

    // static
    BindGroupBase* BindGroupBase::MakeError(DeviceBase* device) {
        return new BindGroupBase(device, ObjectBase::kError);
    }

    const BindGroupLayoutBase* BindGroupBase::GetLayout() const {
        ASSERT(!IsError());
        return mLayout.Get();
    }

    BufferBinding BindGroupBase::GetBindingAsBufferBinding(size_t binding) {
        ASSERT(!IsError());
        ASSERT(binding < kMaxBindingsPerGroup);
        ASSERT(mLayout->GetBindingInfo().mask[binding]);
        ASSERT(mLayout->GetBindingInfo().types[binding] == dawn::BindingType::UniformBuffer ||
               mLayout->GetBindingInfo().types[binding] == dawn::BindingType::StorageBuffer ||
               mLayout->GetBindingInfo().types[binding] ==
                   dawn::BindingType::DynamicUniformBuffer ||
               mLayout->GetBindingInfo().types[binding] == dawn::BindingType::DynamicStorageBuffer);
        BufferBase* buffer = static_cast<BufferBase*>(mBindings[binding].Get());
        return {buffer, mOffsets[binding], mSizes[binding]};
    }

    SamplerBase* BindGroupBase::GetBindingAsSampler(size_t binding) {
        ASSERT(!IsError());
        ASSERT(binding < kMaxBindingsPerGroup);
        ASSERT(mLayout->GetBindingInfo().mask[binding]);
        ASSERT(mLayout->GetBindingInfo().types[binding] == dawn::BindingType::Sampler);
        return static_cast<SamplerBase*>(mBindings[binding].Get());
    }

    TextureViewBase* BindGroupBase::GetBindingAsTextureView(size_t binding) {
        ASSERT(!IsError());
        ASSERT(binding < kMaxBindingsPerGroup);
        ASSERT(mLayout->GetBindingInfo().mask[binding]);
        ASSERT(mLayout->GetBindingInfo().types[binding] == dawn::BindingType::SampledTexture);
        return static_cast<TextureViewBase*>(mBindings[binding].Get());
    }

}  // namespace dawn_native
