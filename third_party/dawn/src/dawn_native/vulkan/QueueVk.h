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

#ifndef DAWNNATIVE_VULKAN_QUEUEVK_H_
#define DAWNNATIVE_VULKAN_QUEUEVK_H_

#include "dawn_native/Queue.h"

namespace dawn_native { namespace vulkan {

    class CommandBuffer;
    class Device;

    class Queue final : public QueueBase {
      public:
        static Queue* Create(Device* device);

      private:
        ~Queue() override;
        using QueueBase::QueueBase;

        MaybeError SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) override;
        MaybeError WriteTextureImpl(const TextureCopyView* destination,
                                    const void* data,
                                    size_t dataSize,
                                    const TextureDataLayout* dataLayout,
                                    const Extent3D* writeSize) override;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_QUEUEVK_H_