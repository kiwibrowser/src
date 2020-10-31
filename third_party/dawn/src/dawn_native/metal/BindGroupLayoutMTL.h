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

#ifndef DAWNNATIVE_METAL_BINDGROUPLAYOUTMTL_H_
#define DAWNNATIVE_METAL_BINDGROUPLAYOUTMTL_H_

#include "common/SlabAllocator.h"
#include "dawn_native/BindGroupLayout.h"

namespace dawn_native { namespace metal {

    class BindGroup;
    class Device;

    class BindGroupLayout final : public BindGroupLayoutBase {
      public:
        BindGroupLayout(DeviceBase* device, const BindGroupLayoutDescriptor* descriptor);

        BindGroup* AllocateBindGroup(Device* device, const BindGroupDescriptor* descriptor);
        void DeallocateBindGroup(BindGroup* bindGroup);

      private:
        ~BindGroupLayout() override = default;
        SlabAllocator<BindGroup> mBindGroupAllocator;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_BINDGROUPLAYOUTMTL_H_
