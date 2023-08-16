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

#ifndef DAWNNATIVE_OPENGL_QUEUEGL_H_
#define DAWNNATIVE_OPENGL_QUEUEGL_H_

#include "dawn_native/Queue.h"

namespace dawn_native { namespace opengl {

    class CommandBuffer;
    class Device;

    class Queue final : public QueueBase {
      public:
        Queue(Device* device);

      private:
        MaybeError SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) override;
        MaybeError WriteBufferImpl(BufferBase* buffer,
                                   uint64_t bufferOffset,
                                   const void* data,
                                   size_t size) override;
    };

}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_QUEUEGL_H_
