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

#ifndef DAWNNATIVE_VULKAN_BUFFERVK_H_
#define DAWNNATIVE_VULKAN_BUFFERVK_H_

#include "dawn_native/Buffer.h"

#include "common/SerialQueue.h"
#include "common/vulkan_platform.h"
#include "dawn_native/vulkan/MemoryAllocator.h"

namespace dawn_native { namespace vulkan {

    class Device;

    class Buffer : public BufferBase {
      public:
        Buffer(Device* device, const BufferDescriptor* descriptor);
        ~Buffer();

        void OnMapReadCommandSerialFinished(uint32_t mapSerial, const void* data);
        void OnMapWriteCommandSerialFinished(uint32_t mapSerial, void* data);

        VkBuffer GetHandle() const;

        // Transitions the buffer to be used as `usage`, recording any necessary barrier in
        // `commands`.
        // TODO(cwallez@chromium.org): coalesce barriers and do them early when possible.
        void TransitionUsageNow(VkCommandBuffer commands, dawn::BufferUsageBit usage);

      private:
        // Dawn API
        void MapReadAsyncImpl(uint32_t serial) override;
        void MapWriteAsyncImpl(uint32_t serial) override;
        void UnmapImpl() override;
        void DestroyImpl() override;

        bool IsMapWritable() const override;
        MaybeError MapAtCreationImpl(uint8_t** mappedPointer) override;

        VkBuffer mHandle = VK_NULL_HANDLE;
        DeviceMemoryAllocation mMemoryAllocation;

        dawn::BufferUsageBit mLastUsage = dawn::BufferUsageBit::None;
    };

    class MapRequestTracker {
      public:
        MapRequestTracker(Device* device);
        ~MapRequestTracker();

        void Track(Buffer* buffer, uint32_t mapSerial, void* data, bool isWrite);
        void Tick(Serial finishedSerial);

      private:
        Device* mDevice;

        struct Request {
            Ref<Buffer> buffer;
            uint32_t mapSerial;
            void* data;
            bool isWrite;
        };
        SerialQueue<Request> mInflightRequests;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_BUFFERVK_H_
