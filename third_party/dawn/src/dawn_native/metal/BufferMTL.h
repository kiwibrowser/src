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

#ifndef DAWNNATIVE_METAL_BUFFERMTL_H_
#define DAWNNATIVE_METAL_BUFFERMTL_H_

#include "common/SerialQueue.h"
#include "dawn_native/Buffer.h"

#import <Metal/Metal.h>

namespace dawn_native { namespace metal {

    class Device;

    class Buffer : public BufferBase {
      public:
        Buffer(Device* device, const BufferDescriptor* descriptor);
        ~Buffer();

        id<MTLBuffer> GetMTLBuffer();

        void OnMapCommandSerialFinished(uint32_t mapSerial, bool isWrite);

      private:
        // Dawn API
        void MapReadAsyncImpl(uint32_t serial) override;
        void MapWriteAsyncImpl(uint32_t serial) override;
        void UnmapImpl() override;
        void DestroyImpl() override;

        bool IsMapWritable() const override;
        MaybeError MapAtCreationImpl(uint8_t** mappedPointer) override;

        id<MTLBuffer> mMtlBuffer = nil;
    };

    class MapRequestTracker {
      public:
        MapRequestTracker(Device* device);
        ~MapRequestTracker();

        void Track(Buffer* buffer, uint32_t mapSerial, bool isWrite);
        void Tick(Serial finishedSerial);

      private:
        Device* mDevice;

        struct Request {
            Ref<Buffer> buffer;
            uint32_t mapSerial;
            bool isWrite;
        };
        SerialQueue<Request> mInflightRequests;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_BUFFERMTL_H_
