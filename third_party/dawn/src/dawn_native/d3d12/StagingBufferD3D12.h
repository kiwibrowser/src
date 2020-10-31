// Copyright 2018 The Dawn Authors
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

#ifndef DAWNNATIVE_STAGINGBUFFERD3D12_H_
#define DAWNNATIVE_STAGINGBUFFERD3D12_H_

#include "dawn_native/StagingBuffer.h"
#include "dawn_native/d3d12/ResourceHeapAllocationD3D12.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class CommandRecordingContext;
    class Device;

    class StagingBuffer : public StagingBufferBase {
      public:
        StagingBuffer(size_t size, Device* device);
        ~StagingBuffer() override;

        ID3D12Resource* GetResource() const;

        MaybeError Initialize() override;

      private:
        Device* mDevice;
        ResourceHeapAllocation mUploadHeap;
    };
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_STAGINGBUFFERD3D12_H_
