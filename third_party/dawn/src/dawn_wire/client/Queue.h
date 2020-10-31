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

#ifndef DAWNWIRE_CLIENT_QUEUE_H_
#define DAWNWIRE_CLIENT_QUEUE_H_

#include <dawn/webgpu.h>

#include "dawn_wire/WireClient.h"
#include "dawn_wire/client/ObjectBase.h"

#include <map>

namespace dawn_wire { namespace client {

    class Queue : public ObjectBase {
      public:
        using ObjectBase::ObjectBase;

        WGPUFence CreateFence(const WGPUFenceDescriptor* descriptor);
        void Signal(WGPUFence fence, uint64_t signalValue);
        void WriteBuffer(WGPUBuffer cBuffer, uint64_t bufferOffset, const void* data, size_t size);
        void WriteTexture(const WGPUTextureCopyView* destination,
                          const void* data,
                          size_t dataSize,
                          const WGPUTextureDataLayout* dataLayout,
                          const WGPUExtent3D* writeSize);
    };

}}  // namespace dawn_wire::client

#endif  // DAWNWIRE_CLIENT_QUEUE_H_
