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
#include "common/Math.h"
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
        static BindGroupBase* MakeError(DeviceBase* device);

        BindGroupLayoutBase* GetLayout();
        const BindGroupLayoutBase* GetLayout() const;
        BufferBinding GetBindingAsBufferBinding(BindingIndex bindingIndex);
        SamplerBase* GetBindingAsSampler(BindingIndex bindingIndex) const;
        TextureViewBase* GetBindingAsTextureView(BindingIndex bindingIndex);
        const ityp::span<uint32_t, uint64_t>& GetUnverifiedBufferSizes() const;

      protected:
        // To save memory, the size of a bind group is dynamically determined and the bind group is
        // placement-allocated into memory big enough to hold the bind group with its
        // dynamically-sized bindings after it. The pointer of the memory of the beginning of the
        // binding data should be passed as |bindingDataStart|.
        BindGroupBase(DeviceBase* device,
                      const BindGroupDescriptor* descriptor,
                      void* bindingDataStart);

        // Helper to instantiate BindGroupBase. We pass in |derived| because BindGroupBase may not
        // be first in the allocation. The binding data is stored after the Derived class.
        template <typename Derived>
        BindGroupBase(Derived* derived, DeviceBase* device, const BindGroupDescriptor* descriptor)
            : BindGroupBase(device,
                            descriptor,
                            AlignPtr(reinterpret_cast<char*>(derived) + sizeof(Derived),
                                     descriptor->layout->GetBindingDataAlignment())) {
            static_assert(std::is_base_of<BindGroupBase, Derived>::value, "");
        }

      protected:
        ~BindGroupBase() override;

      private:
        BindGroupBase(DeviceBase* device, ObjectBase::ErrorTag tag);
        void DeleteThis() override;

        Ref<BindGroupLayoutBase> mLayout;
        BindGroupLayoutBase::BindingDataPointers mBindingData;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BINDGROUP_H_
