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

#ifndef DAWNNATIVE_BINDGROUP_H_
#define DAWNNATIVE_BINDGROUP_H_

#include "common/Constants.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn_native/dawn_platform.h"

#include <array>

namespace dawn_native {

    class DeviceBase;

    MaybeError ValidateBindGroupDescriptor(DeviceBase* device,
                                           const BindGroupDescriptor* descriptor);

    struct BufferBinding {
        BufferBase* buffer;
        uint64_t offset;
        uint64_t size;
    };

    class BindGroupBase : public ObjectBase {
      public:
        BindGroupBase(DeviceBase* device, const BindGroupDescriptor* descriptor);

        static BindGroupBase* MakeError(DeviceBase* device);

        const BindGroupLayoutBase* GetLayout() const;
        BufferBinding GetBindingAsBufferBinding(size_t binding);
        SamplerBase* GetBindingAsSampler(size_t binding);
        TextureViewBase* GetBindingAsTextureView(size_t binding);

      private:
        BindGroupBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        Ref<BindGroupLayoutBase> mLayout;
        std::array<Ref<ObjectBase>, kMaxBindingsPerGroup> mBindings;
        std::array<uint32_t, kMaxBindingsPerGroup> mOffsets;
        std::array<uint32_t, kMaxBindingsPerGroup> mSizes;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BINDGROUP_H_
