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

#ifndef DAWNNATIVE_D3D12_RESOURCEALLOCATIONMANAGER_H_
#define DAWNNATIVE_D3D12_RESOURCEALLOCATIONMANAGER_H_

#include "dawn_native/d3d12/d3d12_platform.h"

#include "common/SerialQueue.h"

namespace dawn_native { namespace d3d12 {

    class Device;

    class ResourceAllocator {
      public:
        ResourceAllocator(Device* device);

        ComPtr<ID3D12Resource> Allocate(D3D12_HEAP_TYPE heapType,
                                        const D3D12_RESOURCE_DESC& resourceDescriptor,
                                        D3D12_RESOURCE_STATES initialUsage);
        void Release(ComPtr<ID3D12Resource> resource);
        void Tick(uint64_t lastCompletedSerial);

      private:
        Device* mDevice;

        SerialQueue<ComPtr<ID3D12Resource>> mReleasedResources;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_RESOURCEALLOCATIONMANAGER_H_
