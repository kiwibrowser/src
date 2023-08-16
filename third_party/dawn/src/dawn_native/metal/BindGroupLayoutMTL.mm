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

#include "dawn_native/metal/BindGroupLayoutMTL.h"

#include "dawn_native/metal/BindGroupMTL.h"

namespace dawn_native { namespace metal {

    BindGroupLayout::BindGroupLayout(DeviceBase* device,
                                     const BindGroupLayoutDescriptor* descriptor)
        : BindGroupLayoutBase(device, descriptor),
          mBindGroupAllocator(MakeFrontendBindGroupAllocator<BindGroup>(4096)) {
    }

    BindGroup* BindGroupLayout::AllocateBindGroup(Device* device,
                                                  const BindGroupDescriptor* descriptor) {
        return mBindGroupAllocator.Allocate(device, descriptor);
    }

    void BindGroupLayout::DeallocateBindGroup(BindGroup* bindGroup) {
        mBindGroupAllocator.Deallocate(bindGroup);
    }

}}  // namespace dawn_native::metal
