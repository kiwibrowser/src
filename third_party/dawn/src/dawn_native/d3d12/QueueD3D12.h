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

#ifndef DAWNNATIVE_D3D12_QUEUED3D12_H_
#define DAWNNATIVE_D3D12_QUEUED3D12_H_

#include "dawn_native/Queue.h"

#include "dawn_native/d3d12/CommandRecordingContext.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class Device;
    class CommandBuffer;

    class Queue final : public QueueBase {
      public:
        Queue(Device* device);

      private:
        MaybeError SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) override;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_QUEUED3D12_H_
